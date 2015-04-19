#include <stdio.h>
#include <sys/mm.h>
#include <sys/panic.h>

/* Initialize the allocator
 * rm is reserved memory ranges, which will not be returned from
 * this allocator.
 *
 * Those ranges typically include the kernel itself, and bootstrap
 * page tables.
 *
 * rm is assumed to be sorted.
 */

struct free_page {
	struct free_page *next;
};
static struct free_page *head = NULL;
static struct memory_range *usable_head = NULL;
static struct obj_pool *mrng_pool = NULL;

void page_allocator_init_early(uint64_t extra_page, int count) {
	if (count < 4)
		panic("Not enough memory");
	int i;
	for (i = 0; i < count; i++) {
		struct free_page *fp = (void *)extra_page;
		fp->next = head;
		head = fp;
	}
	//Take one page here
	mrng_pool = obj_pool_create(sizeof(struct memory_range));
}

void page_allocator_init(struct smap_t *smap, int nsmap, struct memory_range *rm) {
	int i;
	uint64_t rmend = rm->base+rm->length;
	for (i = 0; i < nsmap; i++) {
		int base = smap[i].base, length = smap[i].length;
		if (rm->base <= smap[i].base && smap[i].base < rmend) {
			if (smap[i].base+smap[i].length <= rmend)
				continue;
			length -= (rmend-base);
			base = rmend;
		}

		printf("Memory range %p-%p free to allocate\n", base, base+length);

		struct memory_range *mr = obj_pool_alloc(mrng_pool);
		mr->base = base;
		mr->length = length;
		mr->next = usable_head;
		usable_head = mr;
	}
}

void *get_page(void) {
	if (head) {
		void *ret = head;
		head = head->next;
		return ret;
	}

	//No mapped free page, map a new one
	uint64_t begin = usable_head->base;
	if (usable_head->length == 0x1000) {
		struct memory_range *n = usable_head->next;
		obj_pool_free(mrng_pool, usable_head);
		usable_head = n;
	}
	uint64_t ret = kmmap_to_vmbase(begin);
	return (void *)ret;
}

void drop_page(void *pg) {
	if ((uint64_t)pg&0xfff)
		panic("Dropping invalid page");
	struct free_page *new = pg;
	new->next = head;
	head = new;
}
