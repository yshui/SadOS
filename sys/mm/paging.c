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
#include <sys/printk.h>
#include <string.h>
#include <sys/panic.h>

static uint64_t endaddr = 0;
static uint64_t get_phys_page_early(void) {
	uint64_t tmp = endaddr;
	endaddr += 4096;
	return (tmp-(uint64_t)&physoffset);
}

uint64_t *get_pte_addr(uint64_t addr, int level) {
	int i;
	uint64_t res = 0;
	addr&=0xffffffffffffull;
	for (i = 0; i < 4-level; i++)
		res |= 0x100ull<<(39-9*i);

	addr >>= 12;
	if (level < 3)
		addr >>= 9*(3-level)-3;
	else
		addr <<= 3;
	res |= addr&~0x7;
	res |= (0xffff000000000000ull); //sign extension
	return (void *)res;
}

static uint64_t *
get_pte_addr_checked(uint64_t vaddr, int pgsize, int flags,
		     uint64_t (*gpp)(void), int create) {
	int i;
	for (i = 0; i < 3-pgsize; i++) {
		uint64_t *pte = get_pte_addr(vaddr, i);
		if (!(*pte & PTE_P)) {
			if (!create)
				return NULL;
			uint64_t entry = pte_set_base(0, gpp(), 0);
			entry = pte_set_flags(entry, flags);
			entry |= PTE_P|PTE_W;
			*pte = entry;
			//clear next level page
			pte = get_pte_addr(vaddr, i+1);
			pte = (void *)((uint64_t)pte&~0xfff);
			memset(pte, 0, 0x1000);
		} else if (i!=3 && (*pte & BIT(7))) {
			printk("Mapping an already mapped page %p, %d\n", vaddr, 3-i);
			panic("Xx");
		}
	}
	uint64_t *pte = get_pte_addr(vaddr, 3-pgsize);
	return pte;
}
static void _map_page(uint64_t addr, uint64_t vaddr, int pgsize,
		      int flags, uint64_t (*gpp)(void)) {
	uint64_t *pte = get_pte_addr_checked(vaddr, pgsize, flags, gpp, 1);
	if (*pte & PTE_P)
		panic("mapping an already mapped page");
	uint64_t entry = pte_set_base(0, addr, pgsize);
	entry = pte_set_flags(entry, flags);
	entry |= PTE_P;
	*pte = entry;
}

void unmap(uint64_t vaddr) {
	uint64_t *pte = get_pte_addr_checked(vaddr, 0, 0, NULL, 0);
	if (!pte)
		return;
	*pte = 0;
	invlpg(vaddr);
}

static void map_4k_early(uint64_t addr, uint64_t vaddr) {
	_map_page(addr, vaddr, 0, PTE_W, get_phys_page_early);
}

void map_page(uint64_t addr, uint64_t vaddr, int pgsize, int flags) {
	printk("%p->%p\n", vaddr, addr);
	_map_page(addr, vaddr, pgsize, flags, get_phys_page);
}

void map_range(uint64_t addr, uint64_t vaddr, uint64_t len, int pgsize, int flags) {
	int size=PAGE_SIZE<<(9*pgsize);
	if (len&(size-1) || addr&(size-1) || vaddr&(size-1))
		panic("Invalid args in map_range");
	int i;
	for (i=0; i<len; i+=size)
		map_page(addr+i, vaddr+i, pgsize, flags);
}
static struct memory_range *free_range;
static struct obj_pool *frng_pool;
static void memory_range_init(struct smap_t *smap, int smap_len) {
	frng_pool = obj_pool_create(sizeof(struct memory_range));

	struct memory_range **ptr = &free_range;
	for (int i = 0; i < smap_len; i++) {
		struct memory_range *mr = obj_pool_alloc(frng_pool);
		if (smap[i].type != 1)
			continue;
		mr->base = smap[i].base;
		mr->length = smap[i].length;
		*ptr = mr;
		ptr = &mr->next;
	}
}

static int is_range_reserved(uint64_t base, size_t len) {
	struct memory_range *now = free_range;
	uint64_t end = base+len;
	while(now) {
		uint64_t mrend = now->base+now->length;
		if (end > now->base && now->base < mrend)
			return 0;
		if (now->base >= end)
			return 1;
		now = now->next;
	}
	return 1;
}

long insert_range(uint64_t base, size_t len) {
	if (!is_range_reserved(base, len))
		return -1;
	struct memory_range *new = obj_pool_alloc(frng_pool);
	new->base = base;
	new->length = len;
	struct memory_range **nowp = &free_range;
	while(*nowp) {
		if ((*nowp)->base >= base+len)
			break;
		nowp = &(*nowp)->next;
	}
	new->next = *nowp;
	*nowp = new;
	return 0;
}

void memory_init(struct smap_t *smap, int smap_len) {
	//We need an initial page table to bootstrap
	//So we grab some memory from after the kernel
	//Then we can use get_free_page() to setup new page table

	uint64_t firstaddr = ((uint64_t)&kernend+0x1000)&~0xfff;
	endaddr = firstaddr;

	uint64_t *pml4 = (void *)endaddr;
	memset(pml4, 0, 4096);
	endaddr += 0x1000;

	//map kernel image
	uint64_t kbase = (uint64_t)&kernbase;
	uint64_t entry = pte_set_base(0, endaddr-(uint64_t)&physoffset, 0);
	entry |= PTE_P|PTE_W;
	pml4[(kbase>>39)&0x1ff] = entry;

	uint64_t *pdp = (void *)endaddr;
	endaddr += 0x1000;
	memset(pdp, 0, 4096);
	entry = pte_set_base(0, endaddr-(uint64_t)&physoffset, 0);
	entry |= PTE_P|PTE_W;
	pdp[(kbase>>30)&0x1ff] = entry;

	uint64_t *pd = (void *)endaddr;
	endaddr += 0x1000;
	memset(pd, 0, 4096);

	//Map 2 2M pages
	entry = pte_set_base(0, (uint64_t)&physbase, 1);
	entry |= PTE_P|PTE_W;
	pd[(kbase>>21)&0x1ff] = entry;

	entry = pte_set_base(0, (uint64_t)&physbase+0x200000, 1);
	entry |= PTE_P|PTE_W;
	pd[((kbase>>21)&0x1ff)+1] = entry;

	//Self-referencing
	entry = pte_set_base(0, (uint64_t)pml4-(uint64_t)&physoffset, 0);
	entry |= PTE_P|PTE_W;
	pml4[256] = entry;

	printk("Before loading cr3\n");
	//Load cr3
	uint64_t cr3 = ((uint64_t)pml4)-(uint64_t)&physoffset;
	__asm__ volatile ("movq %0, %%cr3" : : "r"(cr3));
	printk("After loading cr3\n");

	uint64_t i, physend = endaddr-(uint64_t)&physoffset;
	for (i = physend; i < physend+0x10000; i += 0x1000)
		map_4k_early(i, i+KERN_VMBASE);
	if (smap[0].base == 0) {
		//Map low 1M
		uint64_t top = smap[0].length > 0x9f000 ? 0x9f000 : smap[0].length&~0xfff;
		for (i = 0; i < top; i += 0x1000)
			map_4k_early(i, i+KERN_VMBASE);
		if ((smap[0].length&~0xfff) > top) {
			smap[0].base = top;
			smap[0].length -= top;
		} else {
			smap++;
			smap_len--;
		}
	}
	uint64_t extra_page = endaddr-(uint64_t)&physoffset+KERN_VMBASE;
	int npage = (physend+0x10000+KERN_VMBASE-extra_page)>>12;

	uint64_t last_addr = 0;
	for (i = 0; i < smap_len; i++) {
		if (smap[i].type != 1)
			continue;
		last_addr = smap[i].base+smap[i].length;
	}
	last_addr &= ~0xfff;
	printk("Last addr: %p\n", last_addr);

	struct memory_range rm;
	rm.base = ((uint64_t)&physbase)&~0xfff;
	rm.length = physend+0x10000-rm.base;
	page_allocator_init(extra_page, npage, smap, smap_len, &rm);
	memory_range_init(smap, smap_len);
	address_space_init();
	page_man_init();
	page_fault_init();
}
