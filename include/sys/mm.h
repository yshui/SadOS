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
#include <sys/err.h>
#include <sys/list.h>
#include <compiler.h>
#include <bitops.h>
#include <sys/lib/binradix.h>

//Mappings are not immediately reflected in page table
//Don't use for kernel address space
#define AS_LAZY 0x1

#define VMA_RW 0x1

#define PF_SHARED 1
#define PF_SNAPSHOT 2
#define PF_HARDWARE 4

#define PAGE_SIZE_BIT (12)
#define PAGE_SIZE BIT(PAGE_SIZE_BIT)
#define KERN_VMBASE (0xffff808000000000ull) //Start address for whole physical page mapping
#define KERN_TOP (0xfffffffffffff000ull)

#define USER_BASE (0x400000ull)
//Leave 1 page just in case
#define USER_TOP (0x7ffffffffffff000ull)

#define PTE_P BIT(0)
#define PTE_W BIT(1)
#define PTE_U BIT(2)

#define MAP_NO_AS BIT(0)

struct smap_t {
	uint64_t base, length;
	uint32_t type;
}__attribute__((packed));

struct memory_range {
	uint64_t base, length;
	struct memory_range *next;
};

struct page {
	//Structure for single page
	uint64_t phys_addr;
	int page_size;

	volatile uint64_t ref_count; //Total reference count
	volatile uint64_t snap_count; //Number of snapshots

	//Should include a list of owners
	struct list_head owner;
};

struct address_space {
	struct obj_pool *vma_pool;
	struct binradix_node *vaddr_map; // Virtual -> Physical map
	struct vm_area *vma;
	uint64_t low_addr, high_addr;
	int as_flags;
	void *pml4;
};

struct page_entry {
	struct page *p;
	struct address_space *as;
	struct list_node owner_of;
	uint64_t vaddr;
	uint16_t flags;
};

struct vm_area {
	int vma_flags;
	int page_size; //0 for 4k, 1 for 2M
	uint64_t vma_begin, vma_length;
	union {
		struct {
			struct vm_area *snap;
			uint64_t offset;
		}vm_snap;
		struct {
			uint64_t pad1, pad2;
		}vm_req;
	}n;
	struct vm_area *next, **prev;
	struct address_space *as;
};

struct vm_area;

void memory_init(struct smap_t *, int);
void map_page(uint64_t addr, uint64_t vaddr, int pgsize, int flags);
void unmap(uint64_t vaddr);
void map_range(uint64_t addr, uint64_t vaddr, uint64_t len, int pgsize, int flags);
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
	in |= flags;
	return in;
}

static inline  void invlpg(uint64_t m) {
	__asm__ volatile ("invlpg (%0)" : : "r"(m) : "memory");
}

static inline void *phys_to_virt(uint64_t x) {
	return (void *)(x+KERN_VMBASE);
}

void page_allocator_init(uint64_t, int, struct smap_t *, int, struct memory_range *);
void *get_page(void);
void *get_page_with_flags(int flags);
uint64_t get_phys_page();
void drop_page(void *);

struct obj_pool;

struct obj_pool *obj_pool_create(uint64_t);
void *obj_pool_alloc(struct obj_pool *);
void obj_pool_free(struct obj_pool *, void *);
void obj_pool_destroy(struct obj_pool *);

uint64_t vma_get_base(struct vm_area *vma);
struct vm_area *insert_vma_to_vaddr(struct address_space *as, uint64_t vaddr, uint64_t len, int flags);
struct vm_area *insert_vma_to_any(struct address_space *as, uint64_t len, int flags);

/* Map the page assigned to vaddr to vaddr, return 1
 * If no page is assigned to vaddr, return 0
 */
long address_space_map(uint64_t vaddr);

long address_space_assign_page(struct address_space *as, struct page *,
			       uint64_t vaddr, int flags);
long address_space_assign_page_with_vma(struct address_space *as,
					struct vm_area *vma,
					struct page *, uint64_t vaddr, int flags);
long address_space_assign_addr(struct address_space *as, uint64_t addr,
			       uint64_t vaddr, int flags);
long address_space_assign_addr_with_vma(struct address_space *as, struct vm_area *vma,
					uint64_t addr, uint64_t len, int flags);
long address_space_assign_addr_range(struct address_space *as, uint64_t addr,
				     uint64_t vaddr, uint64_t len);

struct page_entry;

static inline struct page_entry *
get_allocated_page(struct address_space *as, uint64_t vaddr) {
	struct page_entry *p = binradix_find(as->vaddr_map, vaddr);
	return p;
}
struct page *manage_phys_page_locked(uint64_t addr);
struct page *manage_phys_page(uint64_t addr);
static inline struct page *manage_page(uint64_t addr) {
	return manage_phys_page(addr-KERN_VMBASE);
}
static inline struct page *manage_page_locked(uint64_t addr) {
	return manage_phys_page_locked(addr-KERN_VMBASE);
}
struct vm_area *vma_find_by_vaddr(struct address_space *, uint64_t vaddr);
int validate_range(uint64_t base, size_t len);

struct address_space *aspace_new(int flags);

void page_fault_init(void);
void address_space_init(void);

uint64_t *ptable_get_entry_4k(uint64_t *pml4, uint64_t vaddr);
void unmap_maybe_current(struct address_space *as, uint64_t vaddr);

struct page_entry *page_entry_new(struct address_space *, uint64_t vaddr);
void share_page(struct page *);

//Call without disable interrupt
void unshare_page(struct page_entry *);

void page_man_init(void);
