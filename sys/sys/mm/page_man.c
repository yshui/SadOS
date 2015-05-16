#include <sys/mm.h>
#include <sys/interrupt.h>
#include <string.h>
static struct obj_pool *page_pool = NULL, *page_entry_pool = NULL;
struct binradix_node *phys_pages;

struct page *
manage_phys_page_locked(uint64_t addr) {
	if (!IS_ALIGNED(addr, PAGE_SIZE_BIT))
		return ERR_PTR(-1);
	struct page *p;

	p = binradix_find(phys_pages, addr);

	if (p)
		return p;
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

int page_unref(struct page *p) {
	int ref = atomic_dec_post(p->ref_count);
	if (!ref) {
		disable_interrupts();
		binradix_delete(phys_pages, p->phys_addr);
		enable_interrupts();
		drop_page(phys_to_virt(p->phys_addr));
		obj_pool_free(page_pool, p);
		return 1;
	}
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
	struct page_entry *pe;
	list_for_each(&p->owner, pe, owner_of) {
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
		}
	}
	enable_interrupts();
}

void unshare_page(struct page_entry *pe) {
	//pe used to be a snapshot of some page
	//Now I want to become the sole owner of
	//this page, then, for example, I can write to it

	//First sanity checks
	if (pe->p->snap_count == 0)
		panic("Why are you unsharing a page with 0 snapshots?");

	char *page = get_page();
	disable_interrupts();
	if (pe->p->snap_count == pe->p->ref_count &&
	    pe->p->ref_count == 1) {
		//I'm the only snapshot
		pe->p->snap_count = 0;
		unmap(pe->vaddr);
		drop_page(page);
		enable_interrupts();
		return;
	}
	if (pe->p->ref_count == 1)
		panic("Trying to unshare a page with 1 user");
	//Then create a new page and copy
	memcpy(page, phys_to_virt(pe->p->phys_addr), PAGE_SIZE);

	struct page *p = manage_page((uint64_t)page), *oldp = pe->p;
	list_del(&pe->owner_of);
	if (pe->flags&PF_SHARED) {
	//Unshare all other shared owners
		struct page_entry *o;
		list_for_each(&oldp->owner, o, owner_of) {
			if (!(o->flags&PF_SHARED))
				continue;
			list_del(&o->owner_of);
			oldp->ref_count--;
			list_add(&p->owner, &o->owner_of);
			o->p = p;
			p->ref_count++;

			//Remove old page table entry
			uint64_t *pte = ptable_get_entry_4k(o->as->pml4, o->vaddr);
			if (pte)
				*pte = 0;
		}
	} else {
		pe->flags = PF_SHARED; //Now I own this page
		oldp->snap_count--;
	}
	p->ref_count++;
	pe->p = p;
	list_add(&p->owner, &pe->owner_of);
	//Update my page table
	unmap(pe->vaddr);
	enable_interrupts();
	page_unref(oldp);
}

long take_snapshot(struct address_space *as, struct page_entry *pe, uint64_t vaddr) {
	if (pe->flags & PF_HARDWARE)
		panic("Can't take snapshot of hardware page\n");
	return address_space_assign_page(as, pe->p, vaddr, PF_SNAPSHOT);
}

void page_man_init(void) {
	page_pool = obj_pool_create(sizeof(struct page));
	page_entry_pool = obj_pool_create(sizeof(struct page_entry));
	phys_pages = binradix_new();
}

struct page_entry *page_entry_new(struct address_space *as, uint64_t vaddr) {
	struct page_entry *pe = obj_pool_alloc(page_entry_pool);
	pe->vaddr = vaddr;
	pe->as = as;
	return pe;
}
