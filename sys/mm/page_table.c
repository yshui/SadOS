/* Copyright (C) 2015, Yuxuan Shui <yshuiv7@gmail.com> */

/*
 * Everyone is allowed to copy and/or redistribute verbatim
 * copies of this code as long as the following conditions
 * are met:
 *
 * 1. The copyright infomation and license is unchanged.
 *
 * 2. This piece of code is not submitted as or as part of
 *    an assignment to the CSE506 course at Stony Brook
 *    University, unless the submitter is the copyright
 *    holder.
 */
#include <sys/defs.h>
#include <sys/panic.h>
#include <sys/mm.h>
#include <sys/sched.h>
#include <bitops.h>
uint64_t *
ptable_get_entry_4k(uint64_t *pml4, uint64_t vaddr) {
	int i;
	uint64_t *table_base = pml4;
	if (GET_BIT(vaddr, 63))
		panic("Trying to lookup kernel address in user space page table\n");

	for (i = 0; i < 3; i++) {
		int offset = (vaddr>>(39-i*9))&0x1ff;
		uint64_t pte = table_base[offset];
		if (!(pte&1))
			return NULL;
		table_base = (void *)(pte_get_base(pte)+KERN_VMBASE);
	}
	int offset = (vaddr>>12)&0x1ff;
	return &table_base[offset];
}

void unmap_maybe_current(struct address_space *as, uint64_t vaddr) {
	if (as == current->as) {
		unmap(vaddr);
		return;
	}
	uint64_t *pte = ptable_get_entry_4k(as->pml4, vaddr);
	if (pte)
		*pte = 0;
	return;
}

static void _walk_page_table(uint64_t *pt, uint64_t *pte, int depth, pt_visitor_fn fn) {
	if (depth < 3) {
		int top = 512;
		if (depth == 0)
			top = 256;
		for (int i = 0; i < top; i++) {
			if (GET_BIT(pt[i], 7))
				panic("Huge page in user space");
			if (pt[i]&PTE_P) {
				uint64_t *next_level = (void *)(KERN_VMBASE+pte_get_base(pt[i]));
				_walk_page_table(next_level, &pt[i], depth+1, fn);
			}
		}
	}
	fn(pt, pte);
}
void walk_user_page_table(uint64_t *pml4, pt_visitor_fn fn) {
	_walk_page_table(pml4, NULL, 0, fn);
}
