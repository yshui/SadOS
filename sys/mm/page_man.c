#include <sys/mm.h>
#include <sys/interrupt.h>
#include <sys/sched.h>
#include <string.h>
static struct obj_pool *page_pool = NULL;
struct binradix_node *phys_pages;

struct page *
manage_phys_page_locked(uint64_t addr) {
	if (!IS_ALIGNED(addr, PAGE_SIZE_BIT))
		return ERR_PTR(-1);
	struct page *p;

	p = binradix_find(phys_pages, addr);

	if (p) {
		p->ref_count++;
		return p;
	}
	p = obj_pool_alloc(page_pool);
	p->phys_addr = addr;
	p->page_size = 0;
	p->snap_count = 0;
	p->ref_count = 1;
	list_head_init(&p->owner);

	binradix_insert(phys_pages, addr, p);

	return p;
}

struct page *
manage_phys_page(uint64_t addr) {
	disable_interrupts();
	struct page *p = manage_phys_page_locked(addr);
	enable_interrupts();
	return p;
}

int page_unref(struct page *p, int hw) {
	p->ref_count--;
	if (!p->ref_count) {
		binradix_delete(phys_pages, p->phys_addr);
		if (!hw) {
			//Don't put hardware page back into pool
			printk("Returning page %p to pool\n", p->phys_addr);
			drop_page(phys_to_virt(p->phys_addr));
		}
		obj_pool_free(page_pool, p);
		return 1;
	}
	return 0;
}

int page_entry_unref(struct page_entry *pe) {
	disable_interrupts();
	struct page *p = pe->p;
	if (pe->flags & PF_SNAPSHOT) {
		p->snap_count--;
		if (p->snap_count == 0) {
			//Restore write permissions
			struct page_entry *o;
			list_for_each(&p->owner, o, owner_of) {
				if (!(o->flags&PF_SHARED))
					panic("snap_count == 0 but has snapshot\n");

				uint64_t *pte = ptable_get_entry_4k(o->as->pml4, o->vaddr);
				if (pte && ((*pte)&PTE_P))
					*pte |= PTE_W;
			}
		}
	}
	page_unref(pe->p, pe->flags&PF_HARDWARE);
	enable_interrupts();
	return 0;
}

void page_ref(struct page *p) {
	atomic_inc(p->ref_count);
}


/* Guarantees:
 *
 * If a page has no snapshots, then it has 'normal' permission
 * according to owner's vma in everyone's page table.
 *
 * If a page has > 0 snapshots, it is mapped as read only in
 * everyone's page table
 */

void share_page(struct page *p) {
	disable_interrupts();
	printk("sharing %p\n", p->phys_addr);
	struct page_entry *pe;
	list_for_each(&p->owner, pe, owner_of) {
		if (pe->flags & PF_HARDWARE)
			panic("Sharing PF_HARDWARE page");
		printk("Mapped in process %d, at %p\n", pe->as->owner->pid, pe->vaddr);
		uint64_t *pte = ptable_get_entry_4k(pe->as->pml4, pe->vaddr);
		if (!pte)
			//Not actually mapped yet
			continue;
		uint64_t entry = *pte;
		if (!(entry&PTE_P))
			continue;
		if (entry&PTE_W) {
			//Remove write permission
			entry &= ~PTE_W;
			*pte = entry;
			if (pe->as == current->as)
				invlpg(pe->vaddr);
		}
	}
	enable_interrupts();
}

void unshare_page(struct page_entry *pe) {
	if (pe->as != current->as)
		panic("Unsharing page from un-current as\n");
	//pe used to be a snapshot of some page
	//Now I want to become the sole owner of
	//this page, then, for example, I can write to it

	//First sanity checks
	disable_interrupts();
	if (pe->flags & PF_HARDWARE)
		panic("Unsharing PF_HARDWARE page??");
	if (pe->p->snap_count == 0) {
		//No snapshots, restore write permission
		disable_interrupts();
		struct page_entry *o;
		list_for_each(&pe->p->owner, o, owner_of) {
			uint64_t *pte = ptable_get_entry_4k(o->as->pml4, o->vaddr);
			if (pte && ((*pte)&PTE_P)) {
				*pte |= PTE_W;
				if (o->as == current->as)
					invlpg(o->vaddr);
			}
		}
		enable_interrupts();
		return;
	}

	if (pe->p->snap_count == pe->p->ref_count &&
	    pe->p->ref_count == 1) {
		//I'm the only snapshot
		pe->p->snap_count = 0;
		unmap(pe->vaddr);
		enable_interrupts();
		return;
	}
	char *page = get_page();
	if (pe->p->ref_count == 1)
		panic("Trying to unshare a page with 1 user");
	//Then create a new page and copy
	memcpy(page, phys_to_virt(pe->p->phys_addr), PAGE_SIZE);

	struct page *p = manage_page((uint64_t)page), *oldp = pe->p;
	list_del(&pe->owner_of);
	if (pe->flags&PF_SHARED) {
	//Unshare all other shared owners
		struct page_entry *oi, *onxt;
		list_for_each_safe(&oldp->owner, oi, onxt, owner_of) {
			if (!(oi->flags&PF_SHARED))
				continue;
			list_del(&oi->owner_of);
			oldp->ref_count--;
			list_add(&p->owner, &oi->owner_of);
			oi->p = p;
			p->ref_count++;

			//Remove old page table entry
			uint64_t *pte = ptable_get_entry_4k(oi->as->pml4, oi->vaddr);
			if (pte)
				*pte = 0;
		}
	} else {
		pe->flags = PF_SHARED; //Now I own this page
		oldp->snap_count--;
		if (oldp->snap_count == 0) {
			//Restore write permissions
			struct page_entry *o;
			list_for_each(&oldp->owner, o, owner_of) {
				if (!(o->flags&PF_SHARED))
					panic("snap_count == 0 but has snapshot\n");

				uint64_t *pte = ptable_get_entry_4k(o->as->pml4, o->vaddr);
				if (pte && ((*pte)&PTE_P))
					*pte |= PTE_W;
			}
		}
	}
	pe->p = p;
	list_add(&p->owner, &pe->owner_of);
	//Update my page table
	unmap(pe->vaddr);
	page_unref(oldp, 0);
	enable_interrupts();
}

void page_man_init(void) {
	page_pool = obj_pool_create(sizeof(struct page));
	phys_pages = binradix_new();
}

struct page_entry *page_entry_new(struct address_space *as, uint64_t vaddr) {
	struct page_entry *pe = obj_pool_alloc(as->pe_pool);
	pe->vaddr = vaddr;
	pe->as = as;
	return pe;
}
