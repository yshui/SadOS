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
#include <sys/kernaddr.h>
#include <sys/mm.h>
#include <string.h>

#define BIT(n) (1<<(n))

#define PTE_P BIT(0)
#define PTE_W BIT(1)
#define PTE_U BIT(2)

#define GET_BIT(a, n) (a&BIT(n))

static struct memory_range *usable_phys = NULL;
static struct obj_pool *mem_range_pool = NULL;
static inline uint64_t
pte_set_base(uint64_t in, uint64_t base, int type) {
	uint64_t clr = in & (~0xffffffffff000);
	uint64_t sanitized_base = base&(~0xfff);
	if (type == 1)
		//2M page, last level entry
		sanitized_base = sanitized_base & (~0x1fffff);
	else if (type == 2)
		//1G page, last level entry
		sanitized_base = sanitized_base & (~0x3fffffff);
	uint64_t out = clr | sanitized_base;

	if (type)
		//set bit 7
		out |= (1<<7);
	return out;
}

static inline uint64_t
pte_get_base(uint64_t in) {
	return in&0xffffffffff000;
}

static inline uint64_t
pte_set_p(uint64_t in, int p) {
	return (in&0xfffffffffffffffe)|(p&1);
}

static inline uint64_t
pte_set_us(uint64_t in, int us) {
	return (in&0xfffffffffffffffb)|(us<<2);
}

static inline uint64_t
pte_set_rw(uint64_t in, int rw) {
	return (in&0xfffffffffffffffd)|(rw<<3);
}

static inline uint64_t
pte_set_flags(uint64_t in, int flags) {
	//flags take up lower 12 bits
	flags &= 0xfff;
	in &= ~0xfff;
	in |= flags;
	return in;
}

extern char kernend, physoffset;
static uint64_t endaddr = 0;
static uint64_t *get_page_early(void) {
	uint64_t tmp = endaddr;
	endaddr += 4096;
	return (void *)tmp;
}
static inline uint64_t early_p2v(uint64_t addr) {
	return addr+(uint64_t)&physoffset;
}
static inline uint64_t early_v2p(uint64_t addr) {
	return addr-(uint64_t)&physoffset;
}

typedef uint64_t (*addrtl_fn)(uint64_t);
static uint64_t *
_get_entry(uint64_t *pml4, uint64_t vaddr, int pgsize, int *level,
	  addrtl_fn p2v) {
	int i;
	uint64_t *table_base = pml4;

	for (i = 0; i < 3; i++) {
		int offset = (vaddr>>(39-i*9))&0x1ff;
		uint64_t pte = table_base[offset];
		if ((!(pte&1)) ||
		    (i > 0 && GET_BIT(pte, 7))) {
			//Return if is last level, or not present
			*level = i;
			return &table_base[offset];
		}
		table_base = (void *)p2v(pte_get_base(pte));
	}
	*level = 3;
	int offset = (vaddr>>12)&0x1ff;
	return &table_base[offset];
}

//pgsize:
// 0 -> 4k
// 1 -> 2M
// 2 -> 1G (not supported by qemu)
static uint64_t *
_new_table(uint64_t *pml4, uint64_t vaddr, int pgsize, int flags,
	  addrtl_fn p2v, addrtl_fn v2p) {
	//Insert a page table for vaddr, with pgsize.
	//Align the address
	int last_level, level;
	uint64_t *pte, entry;
	vaddr &= 0xffffffffffff; //Only lower 48bits are useful
	vaddr &= ~((1<<(12+9*pgsize))-1);
	last_level = 3-pgsize;
	pte = _get_entry(pml4, vaddr, pgsize, &level, p2v);
	entry = *pte;
	if ((entry&1) && GET_BIT(entry, 7))
		//present and is last level return
		return pte;

	for (;level < last_level; level++) {
		uint64_t entry = 0;
		uint64_t next_base = (uint64_t)get_page_early();
		uint64_t offset = (vaddr>>(30-level*9))&0x1ff;
		entry = pte_set_base(entry, v2p(next_base), 0);
		entry = pte_set_flags(entry, flags);
		entry = pte_set_p(entry, 1);
		*pte = entry;
		memset((void *)next_base, 0, 4096);
		pte = (void *)(next_base+offset*8);
	}
	return pte;
}

uint64_t paging_init_early(struct smap_t *smap, int smap_len) {
	//We need an initial page table to bootstrap
	//So we grab some memory from after the kernel
	//Then we can use get_free_page() to setup new page table

	endaddr = (uint64_t)&kernend;
	endaddr = (endaddr+4096)&(~4095); //Align to page boundary

	uint64_t *pml4e = (void *)endaddr;
	memset(pml4e, 0, 4096);
	endaddr += 4096;

	int i, last_page = ((uint64_t)(&physend-&physbase))/0x200000;
	uint64_t kbase = (uint64_t)&kernbase;
	//Mapping physbase~physend to kernbase~kernend
	for (i = 0; i <= last_page; i++) {
		uint64_t addr = kbase+0x200000*i;
		uint64_t *pte =
		    _new_table(pml4e, addr, 1, PTE_W, early_p2v, early_v2p);
		uint64_t entry = pte_set_base(0, (uint64_t)&physbase+0x200000*i, 1);
		entry = pte_set_p(entry, 1);
		entry |= PTE_W;
		*pte = entry;
	}

	//Mapping whole physical memory onto 0xffff800000000000
	//We have several TBs of address space
	int offset = 0;
	for (i = 0; i < smap_len; i++) {
		int begin = smap[i].base, len = smap[i].length, k;
		if (smap[i].type != 1)
			continue;
		//Align begin and len to page boundary
		len = len&~0xfff;
		begin = (begin+0xfff)&~0xfff;
		for (k = 0; k < len/0x1000; k++) {
			uint64_t addr = KERN_VMBASE+offset*0x1000;
			uint64_t *pte = _new_table(pml4e, addr, 0, PTE_W, early_p2v, early_v2p);
			uint32_t entry = pte_set_base(0, begin+0x1000*k, 0);
			entry |= PTE_P | PTE_W;
			*pte = entry;
			offset++;
		}
	}

	//Load cr3
	uint64_t cr3 = ((uint64_t)pml4e)-(uint64_t)&physoffset;
	__asm__ volatile ("movq %0, %%cr3" : : "r"(cr3));

	struct memory_range rm[2];
	rm[0].base = (uint64_t)&physbase;
	rm[0].length = (uint64_t)(&physend-&physbase);
	rm[1].base = (uint64_t)pml4e-(uint64_t)&physoffset;
	rm[1].length = endaddr-(uint64_t)pml4e;
	page_allocator_init(KERN_VMBASE, smap, smap_len, rm, 2);

	mem_range_pool = obj_pool_new(sizeof(struct memory_range));
	for (i = smap_len-1; i >= 0; i--) {
		if (smap[i].type != 1)
			continue;
		struct memory_range *m = obj_pool_alloc(mem_range_pool);
		m->base = (smap[i].base+0xfff)&~0xfff;
		m->length = smap[i].length&~0xfff;
		m->next = usable_phys;
		usable_phys = m;
	}

	return KERN_VMBASE;
}

uint64_t make_virtual_addr_for_physical(uint64_t addr) {
	struct memory_range *now = usable_phys;
	uint64_t offset = KERN_VMBASE;
	while(addr >= now->base+now->length) {
		offset += now->length;
		now = now->next;
	}
	if (addr < now->base)
		return 0;
	return offset+addr-now->base;
}
