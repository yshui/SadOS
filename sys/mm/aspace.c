#include <sys/lib/binradix.h>
#include <sys/interrupt.h>
#include <sys/lib/splay.h>
#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/kernaddr.h>
#include <sys/panic.h>
#include <sys/printk.h>
#include <string.h>
#include <bitops.h>
#include <sys/list.h>
#include <sys/sched.h>

static struct obj_pool *aspace_pool = NULL;
int early_address_space = 1;

static inline int vma_flags_to_page_flags(int vmaf) {
	uint64_t res = 0;
	if (vmaf & VMA_RW)
		res |= PTE_W;
	return res|PTE_U;
}

uint64_t vma_get_base(struct vm_area *vma) {
	return vma->vma_begin;
}

static long aspace_add_vma(struct address_space *as, struct vm_area *vma) {
	disable_interrupts();
	struct vm_area **now = &as->vma;
	uint64_t vma_end = vma->vma_begin+vma->vma_length;
	while(*now) {
		uint64_t now_end = (*now)->vma_begin+(*now)->vma_length;
		if ((*now)->vma_begin >= vma_end)
			break;
		if (now_end > vma->vma_begin && (*now)->vma_begin < vma_end) {
			enable_interrupts();
			return -1;
		}
		now = &(*now)->next;
	}

	if (*now)
		(*now)->prev = &vma->next;
	vma->next = *now;
	vma->prev = now;
	vma->as = as;
	*now = vma;
	enable_interrupts();
	return 0;
}

void aspace_init(struct address_space *ret, void *pml4, int flags) {
	ret->vma_pool = obj_pool_create(sizeof(struct vm_area));
	ret->vma = NULL;
	ret->low_addr = USER_BASE;
	ret->high_addr = USER_TOP;
	ret->pml4 = pml4;

	ret->vaddr_map = binradix_new();
	ret->as_flags = flags;
}

void address_space_init(void) {
	aspace_pool = obj_pool_create(sizeof(struct address_space));
}

struct address_space *aspace_new(int flags) {
	struct address_space *ret = obj_pool_alloc(aspace_pool);

	uint64_t *pml4 = get_page();
	memset(pml4, 0, PAGE_SIZE);
	//Set self-referencing entry
	uint64_t entry = pte_set_base(0, (uint64_t)pml4-KERN_VMBASE, 0);
	entry |= PTE_P|PTE_W;
	pml4[256] = entry;

	aspace_init(ret, pml4, flags);
	return ret;
}

struct vm_area *vma_find_by_vaddr(struct address_space *as, uint64_t addr) {
	struct vm_area *now = as->vma;
	while(now) {
		if (now->vma_begin > addr)
			return NULL;
		if (now->vma_begin+now->vma_length > addr)
			return now;
		now = now->next;
	}
	return NULL;
}

long
address_space_map(uint64_t vaddr) {
	if (!IS_ALIGNED(vaddr, PAGE_SIZE_BIT)) {
		printk("address not aligned: %p\n", vaddr);
		return -2;
	}

	struct vm_area *cvma = vma_find_by_vaddr(current->as, vaddr);
	if (!cvma) {
		printk("VMA not found\n");
		return -1;
	}

	struct page_entry *pe = get_allocated_page(current->as, vaddr);
	if (!pe)
		return 0;
	int flags = vma_flags_to_page_flags(cvma->vma_flags);
	if (pe->p->snap_count > 0)
		//Has snapshot, map read only
		flags &= ~PTE_W;
	map_page(pe->p->phys_addr, vaddr, cvma->page_size, flags);
	return 1;
}


static inline
void address_space_assign_page_no_addr_check(struct address_space *as,
					struct page *p, uint64_t vaddr,
					int flags) {
	if ((flags&(PF_SHARED|PF_SNAPSHOT)) == (PF_SHARED|PF_SNAPSHOT))
		panic("Insert page with invalid flags, a page can't be both shared and snapshot");
	if ((flags&(PF_HARDWARE|PF_SNAPSHOT)) == (PF_HARDWARE|PF_SNAPSHOT))
		panic("Insert page with invalid flags, a page can't be both hardware and snapshot");

	struct page_entry *pe = get_allocated_page(as, vaddr);
	disable_interrupts();
	if (!pe) {
		pe = page_entry_new(as, vaddr);
		binradix_insert(as->vaddr_map, vaddr, pe);
	} else {
		//Overwrite
		list_del(&pe->owner_of);
		//Remove the old page table entry
		unmap_maybe_current(as, vaddr);
	}
	pe->p = p;
	pe->flags = flags;
	list_add(&p->owner, &pe->owner_of);
	p->ref_count++;

	//Check the sanity of flags
	if (flags&PF_SNAPSHOT) {
		//Can't snapshot a page that has 0 refcount
		if (p->ref_count == 0)
			panic("Invalid snapshot");
		p->snap_count ++;
		if (p->snap_count == 1) {
			//First snapshot
			//Remove everyone's write permission
			share_page(p);
		}
	}
	enable_interrupts();
}

long address_space_assign_page_with_vma(struct address_space *as, struct vm_area *vma,
					struct page *p, uint64_t vaddr, int flags) {
	if (!IS_ALIGNED(vaddr, PAGE_SIZE_BIT)) {
		printk("address not aligned: %p\n", vaddr);
		return -1;
	}
	if (vaddr < vma->vma_begin || vaddr >= vma->vma_begin+vma->vma_length)
		return -2;
	address_space_assign_page_no_addr_check(as, p, vaddr, flags);
	return 0;
}

long address_space_assign_page(struct address_space *as, struct page *p,
			       uint64_t vaddr, int flags) {
	if (!IS_ALIGNED(vaddr, PAGE_SIZE_BIT)) {
		printk("address not aligned: %p\n", vaddr);
		return -1;
	}
	disable_interrupts();
	struct vm_area *vma = vma_find_by_vaddr(as, vaddr);
	enable_interrupts();
	if (!vma)
		return -2;
	address_space_assign_page_no_addr_check(as, p, vaddr, flags);
	return 0;
}


long address_space_assign_addr(struct address_space *as, uint64_t addr, uint64_t vaddr, int flags) {
	if (!IS_ALIGNED(vaddr, PAGE_SIZE_BIT)) {
		printk("address not aligned: %p\n", vaddr);
		return -1;
	}
	disable_interrupts();
	struct vm_area *vma = vma_find_by_vaddr(as, vaddr);
	enable_interrupts();
	if (!vma)
		return -2;
	struct page *p = manage_phys_page(addr);
	address_space_assign_page_no_addr_check(as, p, vaddr, flags);
	return 0;
}

long address_space_assign_addr_with_vma(struct address_space *as, struct vm_area *vma,
					uint64_t addr, uint64_t vaddr, int flags) {
	if (!IS_ALIGNED(vaddr, PAGE_SIZE_BIT)) {
		printk("address not aligned: %p\n", vaddr);
		return -1;
	}
	if (vaddr < vma->vma_begin || vaddr >= vma->vma_begin+vma->vma_length)
		return -2;
	struct page *p = manage_phys_page(addr);
	address_space_assign_page_no_addr_check(as, p, vaddr, flags);
	return 0;
}

//This function is only used in special cases, e.g. mapping device memory to user space
//Therefore all pages are mapped as shared.
long address_space_assign_addr_range(struct address_space *as, uint64_t addr, uint64_t vaddr, uint64_t len) {
	if (!IS_ALIGNED(addr, PAGE_SIZE_BIT) ||
	    !IS_ALIGNED(vaddr, PAGE_SIZE_BIT) ||
	    !IS_ALIGNED(len, PAGE_SIZE_BIT)) {
		printk("address not aligned (range), %p %p %p\n", addr, vaddr, len);
		return -2;
	}

	disable_interrupts();
	struct vm_area *cvma = vma_find_by_vaddr(as, vaddr);
	if (!cvma) {
		printk("VMA not found\n");
		return -1;
	}
	//Sanity check
	uint64_t tmp_len = len;
	struct vm_area *tvma = cvma;
	while(tmp_len > tvma->vma_length) {
		struct vm_area *nvma = tvma->next;
		uint64_t vma_end = tvma->vma_begin+tvma->vma_length;
		if (!(tvma->vma_flags&VMA_HW)) {
			printk("Tring to assign hardware range to non hardware vma");
			return -4;
		}
		if (nvma->vma_begin != vma_end) {
			printk("Trying to map to area without VMA, start: %p", vma_end);
			return -3;
		}
		tmp_len -= tvma->vma_length;
		tvma = nvma;
	}

	uint64_t offset = 0;
	for (int i = 0; i < len; i+=PAGE_SIZE) {
		if (i >= cvma->vma_length+offset) {
			offset += cvma->vma_length;
			cvma = cvma->next;
		}
		long ret = address_space_assign_addr_with_vma(as, cvma, addr+i, vaddr+i, PF_SHARED|PF_HARDWARE);
		if (ret)
			return ret;
	}
	enable_interrupts();
	return 0;
}
struct vm_area *
insert_vma_to_vaddr(struct address_space *as, uint64_t vaddr, uint64_t len, int flags) {
	disable_interrupts();
	struct vm_area *vma = obj_pool_alloc(as->vma_pool);
	enable_interrupts();
	if (vaddr & 0xfff)
		//address not aligned
		return ERR_PTR(-2);

	vma->vma_begin = vaddr;
	vma->vma_length = len;
	vma->vma_flags = flags;
	vma->page_size = 0;

	long ret = aspace_add_vma(as, vma);
	if (ret < 0)
		//Overlap
		return ERR_PTR(ret);

	return vma;
}

struct vm_area *
insert_vma_to_any(struct address_space *as, uint64_t len, int flags) {
	disable_interrupts();
	struct vm_area **now = &as->vma;
	uint64_t now_addr = as->low_addr;
	while(*now) {
		if ((*now)->vma_begin >= now_addr+len)
			break;
		now_addr = (*now)->vma_begin+(*now)->vma_length;
		now = &(*now)->next;
	}
	if (!*now && as->high_addr < now_addr+len)
		return ERR_PTR(-1);
	struct vm_area *nvma = obj_pool_alloc(as->vma_pool);
	nvma->vma_begin = now_addr;
	nvma->vma_length = len;
	nvma->page_size = 0;
	nvma->vma_flags = flags;

	if (*now)
		(*now)->prev = &nvma->next;
	nvma->next = *now;
	nvma->prev = now;
	*now = nvma;
	enable_interrupts();
	printk("Target: %p\n", nvma->vma_begin);

	return nvma;
}

int validate_range(uint64_t base, size_t len) {
	struct vm_area *vma = vma_find_by_vaddr(current->as, base);
	uint64_t xbase = base, xend = base+len;
	do {
		if (!vma || vma->vma_begin > base) {
			printk("No vma mapped at %p, which is inside [%p-%p]\n", base, xbase, xend);
			return 1;
		}
		uint64_t tmp = vma->vma_begin+vma->vma_length;
		if (tmp-base >= len)
			break;
		len -= tmp-base;
		base = tmp;
		vma = vma->next;
	} while (len > 0);
	return 0;
}
