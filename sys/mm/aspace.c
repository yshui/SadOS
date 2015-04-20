#include <sys/lib/binradix.h>
#include <sys/lib/splay.h>
#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/kernaddr.h>
#include <sys/panic.h>
#include <sys/printk.h>
struct vm_area {
	int vma_flags;
	int page_size; //0 for 4k, 1 for 2M
	uint64_t vma_begin, vma_length;
	struct vm_area *next, **prev;
};

struct address_space {
	struct obj_pool *vma_pool, *page_pool;
	struct binradix_node *vaddr_map; // Virtual -> Physical map
	struct vm_area *vma;
	uint64_t low_addr, high_addr;
	int as_flags;
	void *pml4;
};

struct page {
	//Structure for single page
	uint64_t phys_addr;
	int page_size;

	//Should include a list of owners
};

struct address_space kern_aspace;
static struct obj_pool *aspace_pool = NULL;

static inline int vma_flags_to_page_flags(int vmaf) {
	uint64_t res = 0;
	if (vmaf & VMA_RW)
		res |= PTE_W;
	return res;
}

static int aspace_add_vma(struct address_space *as, struct vm_area *vma) {
	if (!as->vma) {
		as->vma = vma;
		vma->prev = &as->vma;
		vma->next = NULL;
		return 0;
	}
	struct vm_area **now = &as->vma;
	while(*now) {
		if ((*now)->vma_begin > vma->vma_begin)
			break;
		if ((*now)->vma_begin+(*now)->vma_length > vma->vma_begin)
			return -1;
	}
	uint64_t vma_end = vma->vma_begin+vma->vma_length;
	if (*now && (*now)->vma_begin < vma_end)
		return -1;

	if (*now)
		(*now)->prev = &vma->next;
	vma->next = *now;
	vma->prev = now;
	*now = vma;
	return 0;
}

static void aspace_init(struct address_space *ret, void *pml4, int flags) {
	ret->vma_pool = obj_pool_create(sizeof(struct vm_area));
	ret->page_pool = obj_pool_create(sizeof(struct page));
	ret->vma = NULL;
	ret->low_addr = USER_BASE;
	ret->high_addr = USER_TOP;
	ret->pml4 = pml4;

	ret->vaddr_map = binradix_new();
	ret->as_flags = flags;
}

__attribute__((unused)) static struct address_space *aspace_new(void *pml4, int flags) {
	if (!aspace_pool)
		aspace_pool = obj_pool_create(sizeof(struct address_space));
	struct address_space *ret = obj_pool_alloc(aspace_pool);
	aspace_init(ret, pml4, flags);
	return ret;
}

void kaddress_space_init(void *pml4, uint64_t last_addr, struct memory_range *rm) {
	//Add a vma for kernel image
	struct address_space *as = &kern_aspace;
	aspace_init(as, pml4, 0);
	kern_aspace.low_addr = KERN_VMBASE;
	kern_aspace.high_addr = 0xfffffffffffff000ull;
	printk("Initialzed kern_aspace\n");

	struct vm_area *vma = obj_pool_alloc(as->vma_pool);
	vma->page_size = 1;
	vma->vma_flags = 0;
	vma->vma_begin = (uint64_t)&kernbase;
	vma->vma_length = 0x200000;
	aspace_add_vma(as, vma);
	printk("Inserted kernel image vma\n");

	vma = obj_pool_alloc(as->vma_pool);
	vma->page_size = 0;
	vma->vma_flags = 0;
	vma->vma_begin = KERN_VMBASE;
	vma->vma_length = last_addr;
	aspace_add_vma(as, vma);
	printk("Inserted physical memory vma\n");

	struct page *p = obj_pool_alloc(as->page_pool);
	p->page_size = 1;
	p->phys_addr = (uint64_t)&physbase;
	binradix_insert(as->vaddr_map, (uint64_t)&kernbase, p);

	uint64_t i;
	for (i = rm->base; i < rm->base+rm->length; i += 0x1000) {
		struct page *p = obj_pool_alloc(as->page_pool);
		p->page_size = 0;
		p->phys_addr = i;
		//printk("Addr mapping %p->%p\n", i+KERN_VMBASE, i);
		binradix_insert(as->vaddr_map, i+KERN_VMBASE, p);
	}
	printk("Kaddress init done\n");
}

struct vm_area *aspace_vma_find_by_vaddr(struct address_space *as, uint64_t addr) {
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

static void
address_space_map_with_vma(struct address_space *as, struct vm_area *cvma,
			   uint64_t addr, uint64_t vaddr) {
	int flags = vma_flags_to_page_flags(cvma->vma_flags);
	map_page(addr, vaddr, cvma->page_size, flags);

	struct page *p = obj_pool_alloc(as->page_pool);
	p->phys_addr = addr;
	p->page_size = cvma->page_size;
	binradix_insert(as->vaddr_map, vaddr, p);
}

static int
address_space_map(struct address_space *as, uint64_t addr, uint64_t vaddr) {
	struct vm_area *cvma = aspace_vma_find_by_vaddr(as, vaddr);
	if (!cvma)
		return -1;
	address_space_map_with_vma(as, cvma, addr, vaddr);
	return 0;
}

static uint64_t
address_space_get_physical(struct address_space *as, uint64_t vaddr) {
	struct page *p = binradix_find(as->vaddr_map, vaddr);
	if (!p)
		return 0;
	return p->phys_addr;
}

uint64_t kmmap_to_vmbase(uint64_t addr) {
	//Don't map memory range manged by page allocator!!
	printk("Mapping %p to vmbase\n", addr);
	uint64_t vaddr = KERN_VMBASE+addr;
	uint64_t phys = address_space_get_physical(&kern_aspace, vaddr);
	if (!phys) {
		//Not mapped
		printk("Not mapped yet\n");
		int ret = address_space_map(&kern_aspace, addr, vaddr);
		if (ret < 0)
			return ret;
	}
	return vaddr;
}

static void do_map_range(struct address_space *as, struct vm_area *vma,
			 uint64_t addr) {
	uint64_t i, vaddr = vma->vma_begin;
	for (i = 0; i < vma->vma_length; i+=0x1000)
		address_space_map_with_vma(&kern_aspace, vma, addr+i, vaddr+i);

}

int mmap_to_vaddr(struct address_space *as, uint64_t addr, uint64_t vaddr,
		  uint64_t length, int flags) {
	struct vm_area *vma = obj_pool_alloc(as->vma_pool);
	if ((addr & 0xfff) || (vaddr & 0xfff) || (length & 0xfff))
		//address not aligned
		return -2;

	vma->vma_begin = vaddr;
	vma->vma_length = length;
	vma->vma_flags = flags;
	vma->page_size = 0;

	int ret = aspace_add_vma(&kern_aspace, vma);
	if (ret < 0)
		//Overlap
		return ret;

	if (!(as->as_flags&AS_LAZY))
		do_map_range(as, vma, addr);
	return 0;
}

uint64_t mmap_to_any(struct address_space *as, uint64_t addr,
		     uint64_t length, int flags) {
	printk("Mapping %p to any\n", addr);
	struct vm_area *now = as->vma;
	while(now) {
		uint64_t now_end = now->vma_begin+now->vma_length;
		if (now->next) {
			if (now->next->vma_begin-now_end >= length)
				break;
		} else {
			if (as->high_addr-now_end >= length)
				break;
		}
	}
	if (!now)
		return -1;
	struct vm_area *nvma = obj_pool_alloc(as->vma_pool);
	nvma->vma_begin = now->vma_begin+now->vma_length;
	nvma->vma_length = length;
	nvma->page_size = 0;
	nvma->vma_flags = flags;
	printk("Target: %p\n", nvma->vma_begin);

	if (!(as->as_flags&AS_LAZY))
		do_map_range(as, nvma, addr);
	return nvma->vma_begin;

}

int kmmap_to_vaddr(uint64_t addr, uint64_t vaddr, uint64_t length, int flags) {
	return mmap_to_vaddr(&kern_aspace, addr, vaddr, length, flags);
}

uint64_t kmmap_to_any(uint64_t addr, uint64_t length, int flags) {
	return mmap_to_any(&kern_aspace, addr, length, flags);
}

uint64_t kphysical_lookup(uint64_t vaddr) {
	uint64_t phys = address_space_get_physical(&kern_aspace, vaddr);
	return phys;
}
