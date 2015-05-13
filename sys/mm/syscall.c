#include <sys/ipc.h>
#include <sys/sched.h>
#include <sys/syscall.h>
#include <sys/interrupt.h>
#include <sys/aspace.h>
#include <sys/lib/binradix.h>
#include <uapi/aspace.h>
static void _page_entry_visitor(void *_pe, void *d) {
	struct address_space *as = d;
	struct page_entry *pe = _pe;
	if (pe->flags & PF_HARDWARE)
		return;
	pe->p->ref_count++;
	address_space_assign_page(as, pe->p, pe->vaddr, PF_SNAPSHOT);
}
static void as_snapshot(struct address_space *new_as) {
	for (struct vm_area *vma = current->as->vma;
	     vma; vma = vma->next) {
		if (vma->vma_flags & VMA_HW)
			continue;
		insert_vma_to_vaddr(new_as, vma->vma_begin, vma->vma_length, vma->vma_flags);
	}
	binradix_for_each(current->as->vaddr_map, new_as, _page_entry_visitor);
}
SYSCALL(1, asnew, int, flags) {
	disable_interrupts();
	struct address_space *new_as = obj_pool_alloc(current->as_pool);
	int fd = fdtable_insert(current->astable, new_as);
	if (!fd) {
		obj_pool_free(current->as_pool, new_as);
		enable_interrupts();
		return 0;
	}

	aspace_init(new_as, get_page(), 0);

	if (flags&AS_SNAPSHOT)
		as_snapshot(new_as);
	enable_interrupts();
	return fd;
}
//Map page start at vaddr into address space numbered 'as'
//Optionally destionation address
SYSCALL(3, sendpage, int, as, uint64_t, vaddr, uint64_t, dst) {
	return 0;
}
