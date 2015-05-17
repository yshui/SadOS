#include <sys/ipc.h>
#include <sys/sched.h>
#include <sys/syscall.h>
#include <sys/interrupt.h>
#include <sys/aspace.h>
#include <sys/lib/binradix.h>
#include <uapi/aspace.h>
#include <errno.h>
static void _page_entry_visitor(void *_pe, void *d) {
	struct address_space *as = d;
	struct page_entry *pe = _pe;
	if (pe->flags & PF_HARDWARE)
		return;
	address_space_assign_page(as, pe->p, pe->vaddr, PF_SNAPSHOT);
}
static void as_snapshot(struct address_space *new_as) {
	printk("as_snapshot\n");
	for (struct vm_area *vma = current->as->vma;
	     vma; vma = vma->next) {
		if (vma->vma_flags & VMA_HW)
			continue;
		insert_vma_to_vaddr(new_as, vma->vma_begin, vma->vma_length, vma->vma_flags);
	}
	binradix_for_each(current->as->vaddr_map, new_as, _page_entry_visitor);
	printk("as_snapshot return\n");
}
SYSCALL(1, asnew, int, flags) {
	disable_interrupts();
	struct address_space *new_as = aspace_new(0);
	int fd = fdtable_insert(current->astable, new_as);
	if (fd < 0) {
		destroy_as(new_as);
		enable_interrupts();
		return -EMFILE;
	}
	new_as->owner = current;

	if (flags&AS_SNAPSHOT)
		as_snapshot(new_as);
	enable_interrupts();
	return fd;
}
SYSCALL(1, asdestroy, int, as) {
	disable_interrupts();
	if (as <= 0 || as >= current->astable->max_fds || !current->astable->file[as]) {
		enable_interrupts();
		return -EBADF;
	}
	struct address_space *_as = current->astable->file[as];
	current->astable->file[as] = NULL;
	destroy_as(_as);
	enable_interrupts();
	return 0;
}
//Map page start at vaddr into address space numbered 'as'
//Optionally destination address, there can't be anything mapped at destination
//address
SYSCALL(4, sendpage, int, as, uint64_t, vaddr, uint64_t, dst, size_t, len) {
	disable_interrupts();
	long ret = -EBADF;
	printk("Requested to map %p~%p from addrspace 0 in task %d, to %p in addrspace %d\n",
	       vaddr, vaddr+len, current->pid, dst, as);
	if (as < 0 || as > current->astable->max_fds || !current->astable->file[as])
		goto end;

	ret = -EINVAL;
	if (!IS_ALIGNED(vaddr, PAGE_SIZE_BIT))
		goto end;

	if (!IS_ALIGNED(len, PAGE_SIZE_BIT))
		goto end;

	if (vaddr && validate_range(vaddr, PAGE_SIZE))
		goto end;

	struct address_space *_as = current->astable->file[as];
	struct vm_area *vma;
	if (dst)
		vma = insert_vma_to_vaddr(_as, dst, len, VMA_RW);
	else
		vma = insert_vma_to_any(_as, len, VMA_RW);

	ret = -ENOMEM;
	if (PTR_IS_ERR(vma))
		goto end;

	ret = vma->vma_begin;
	if (vaddr) {
		for (uint64_t i = 0; i < len; i += PAGE_SIZE) {
			struct page_entry *pe = get_allocated_page(current->as, vaddr+i);
			if (pe)
				address_space_assign_page(_as, pe->p, vma->vma_begin+i, PF_SNAPSHOT);
		}
	}
end:
	enable_interrupts();
	return ret;
}

SYSCALL(2, munmap, void *, base, size_t, len) {
	if (!IS_ALIGNED((uint64_t)base, PAGE_SIZE_BIT) || !IS_ALIGNED(len, PAGE_SIZE_BIT))
		return -EINVAL;
	remove_vma_range(current->as, (uint64_t)base, len);
	return 0;
}
