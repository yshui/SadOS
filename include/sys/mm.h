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

//Mappings are not immediately reflected in page table
//Don't user for kernel address space
#define AS_LAZY 0x1

//Hardware reserved range
#define VMA_RESERVED 0x1

#define PAGE_SIZE (0x1000)
#define KERN_VMBASE (0xffff800000000000ull) //Start address for whole physical page mapping

struct smap_t {
	uint64_t base, length;
	uint32_t type;
}__attribute__((packed));

struct memory_range {
	uint64_t base, length;
	struct memory_range *next;
};

uint64_t make_virtual_addr_for_physical(uint64_t addr);

void page_allocator_init(uint64_t, uint64_t, struct memory_range *, int nrm);
void *get_page();
void drop_page(void *);

struct obj_pool;

struct obj_pool *obj_pool_new(uint64_t);
void *obj_pool_alloc(struct obj_pool *);
void obj_pool_free(struct obj_pool *, void *);
