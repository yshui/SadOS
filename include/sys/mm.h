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
#pragma once
#include <sys/defs.h>
#include <compiler.h>

//Mappings are not immediately reflected in page table
//Don't use for kernel address space
#define AS_LAZY 0x1

#define VMA_RW 0x1

#define PAGE_SIZE (0x1000)
#define KERN_VMBASE (0xffff808000000000ull) //Start address for whole physical page mapping
#define KERN_TOP (0xfffffffffffff000ull)

#define USER_BASE (0x400000ull)
//Leave 1 page just in case
#define USER_TOP (0x7ffffffffffff000ull)

#define BIT(n) (1<<(n))

#define PTE_P BIT(0)
#define PTE_W BIT(1)
#define PTE_U BIT(2)

#define GET_BIT(a, n) (a&BIT(n))

struct smap_t {
	uint64_t base, length;
	uint32_t type;
}__attribute__((packed));

struct memory_range {
	uint64_t base, length;
	struct memory_range *next;
};

struct address_space;

extern struct address_space kern_aspace;

void memory_init(struct smap_t *, int);
void map_page(uint64_t, uint64_t, int, int);
uint64_t *get_pte_addr(uint64_t addr, int level);
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
pte_set_flags(uint64_t in, int flags) {
	//flags take up lower 12 bits
	flags &= 0xfff;
	in &= ~0xfff;
	in |= flags;
	return in;
}

static inline  void invlpg(uint64_t m) {
	__asm__ volatile ("invlpg (%0)" : : "r"(m) : "memory");
}

void page_allocator_init_early(uint64_t, int);
void page_allocator_init(struct smap_t *, int, struct memory_range *);
void *get_page();
uint64_t get_phys_page();
void drop_page(void *);

struct obj_pool;

struct obj_pool *obj_pool_create(uint64_t);
void *obj_pool_alloc(struct obj_pool *);
void obj_pool_free(struct obj_pool *, void *);

void kaddress_space_init(void *, uint64_t, struct memory_range *);
uint64_t kphysical_lookup(uint64_t);
uint64_t kvirtual_lookup(uint64_t);
uint64_t kmmap_to_vmbase(uint64_t addr);
int kmmap_to_vaddr(uint64_t addr, uint64_t vaddr, uint64_t length, int flags);
uint64_t kmmap_to_any(uint64_t addr, uint64_t length, int flags);
