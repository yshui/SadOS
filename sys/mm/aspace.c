#include <sys/lib/binradix.h>
#include <sys/lib/splay.h>
#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/kernaddr.h>
struct vm_area {
	int vma_flags;
	int page_size; //0 for 4k, 1 for 2M
	uint64_t vma_begin, vma_length;
};

struct address_space {
	struct obj_pool *vma_pool, *page_pool;
	struct binradix_node *vaddr_map; // Virtual -> Physical map
	struct splay *vma;
	int as_flags;
};

struct page {
	//Structure for single page
	uint64_t phys_addr;
	int page_size;

	//Should include a list of owners
};

static struct obj_pool *aspace_pool = NULL;

static int vma_cmp(const void *a, const void *b) {
	//Return 1 if a is to the right of b, -1 otherwise
	//Return 0 is intersect
	const struct vm_area *vma = a, *vmb = b;
	if (vma->vma_begin > vmb->vma_begin+vmb->vma_length)
		return 1;
	if (vmb->vma_begin > vma->vma_begin+vma->vma_length)
		return -1;
	return 0;
}

static struct address_space *aspace_new(int flags) {
	if (!aspace_pool)
		aspace_pool = obj_pool_new(sizeof(struct address_space));
	struct address_space *ret = obj_pool_alloc(aspace_pool);
	ret->vma_pool = obj_pool_new(sizeof(struct vm_area));
	ret->page_pool = obj_pool_new(sizeof(struct page));
	ret->vma = splay_new(vma_cmp);
	ret->vaddr_map = binradix_new();
	ret->as_flags = flags;
	return ret;
}

struct address_space *
kernel_address_space_init(struct smap_t *smap, int smap_len) {
	struct address_space *as = aspace_new(0);
	//Add a vma for kernel image
	struct vm_area *vma = obj_pool_alloc(as->vma_pool);
	vma->page_size = 1;
	vma->vma_flags = 0;
	vma->vma_begin = (uint64_t)&physoffset;
	vma->vma_length = 0x200000;

	splay_insert(as->vma, vma);

	struct page *p = obj_pool_alloc(as->page_pool);
	p->page_size = 1;
	p->phys_addr = (uint64_t)&physbase;
	binradix_insert(as->vaddr_map, (uint64_t)&kernbase, p);

	//Then setup 
}
