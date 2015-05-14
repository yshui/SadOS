#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/panic.h>
#include <sys/interrupt.h>
#include <string.h>
//Allocator for fixed size objects
struct free_obj {
	struct free_obj *next;
};
struct page_hdr {
	struct page_hdr *next;
};
struct obj_pool {
	void *free_page;
	struct free_obj *free_list;
	struct page_hdr *page_head;
	uint64_t obj_size;
	uint16_t page_free;
}meta_obj_pool = {
	.free_page = NULL,
	.obj_size = sizeof(struct obj_pool),
};

extern int early_address_space;

void *obj_pool_alloc(struct obj_pool *obp) {
	if (obp->free_list) {
		void *ret = (void *)obp->free_list;
		obp->free_list = obp->free_list->next;
		return ret;
	}
	if (obp->page_free < obp->obj_size) {
		void *page = get_page();
		struct page_hdr *page_header = page;
		page_header->next = obp->page_head;
		obp->page_head = page_header;
		obp->free_page = page_header;
		obp->page_free = PAGE_SIZE-sizeof(struct page_hdr);
	}
	void *ret = obp->free_page+PAGE_SIZE-obp->page_free;
	obp->page_free -= obp->obj_size;
	return ret;
}

void obj_pool_free(struct obj_pool *obp, void *obj) {
	//Sanity check
	uint64_t tmp = (uint64_t)obj;
	uint64_t tmp_aligned = ALIGN(tmp, PAGE_SIZE_BIT);
	if ((tmp-tmp_aligned-sizeof(struct page_hdr))%obp->obj_size != 0)
		panic("Invalid object free %p %p %d\n", tmp, tmp_aligned, obp->obj_size);

	struct free_obj *free_obj = obj;
	free_obj->next = obp->free_list;
	obp->free_list = free_obj;
}

struct obj_pool *obj_pool_create(uint64_t size) {
	if (size < sizeof(struct free_obj) || size > PAGE_SIZE)
		panic("Invalid object size");
	disable_interrupts();
	struct obj_pool *obp = obj_pool_alloc(&meta_obj_pool);
	memset(obp, 0, sizeof(struct obj_pool));
	enable_interrupts();
	obp->page_head = NULL;
	obp->free_page = NULL;
	obp->free_list = NULL;
	obp->page_free = 0;
	obp->obj_size = size;
	return obp;
}

void obj_pool_destroy(struct obj_pool *obp) {
	struct page_hdr *ph = obp->page_head;
	while(ph) {
		struct page_hdr *next = ph->next;
		drop_page(ph);
		ph = next;
	}
	disable_interrupts();
	obj_pool_free(&meta_obj_pool, obp);
	enable_interrupts();
}

int obj_pool_size_eq(struct obj_pool *obp, uint64_t size) {
	return obp->obj_size == size;
}
