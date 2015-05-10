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
