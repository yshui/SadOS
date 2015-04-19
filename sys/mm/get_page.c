#include <sys/printk.h>
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
static int status = 0;

void page_allocator_init_early(uint64_t extra_page, int count) {
	if (count < 4)
		panic("Not enough memory");
	printk("Init allocator with %d pages\n", count);
	int i;
	for (i = 0; i < count; i++) {
		struct free_page *fp = (void *)(extra_page+i*0x1000);
		fp->next = head;
		head = fp;
	}
	//Take one page here
	mrng_pool = obj_pool_create(sizeof(struct memory_range));
	status = 1;
	printk("Page allocator init early done\n");
}

void page_allocator_init(struct smap_t *smap, int nsmap, struct memory_range *rm) {
	int i;
	uint64_t rmend = rm->base+rm->length;
	printk("RM %p-%p\n", rm->base, rmend);
	for (i = 0; i < nsmap; i++) {
		uint64_t base = smap[i].base, length = smap[i].length;
		uint64_t end = base+length;
		if (smap[i].type != 1)
			continue;
		if (rmend > smap[i].base && end > rm->base) {
			if (rmend >= end && rm->base <= smap[i].base)
				continue;
			if (rmend >= end)
				length = (rm->base-base);
			else {
				if (rm->base > base) {
					struct memory_range *mr = obj_pool_alloc(mrng_pool);
					mr->base = base;
					mr->length = rm->base-base;
					mr->next = usable_head;
					usable_head = mr;
					printk("Memory range %p-%p free to use\n", base, rm->base);
				}
				base = rmend;
				length = end-base;
			}
		}
		printk("Memory range %p-%p free to use\n", base, base+length);

		struct memory_range *mr = obj_pool_alloc(mrng_pool);
		mr->base = base;
		mr->length = length;
		mr->next = usable_head;
		usable_head = mr;
	}
	status = 2;
}

uint64_t get_phys_page(void) {
	if (status == 1)
		panic("Early get phys page");
	if (head) {
		void *ret = head;
		head = head->next;
		uint64_t p = kphysical_lookup((uint64_t)ret);
		if (!p)
			panic("Page in free list not mapped??");
		return p;
	}

	//No mapped free page, map a new one
	uint64_t begin = usable_head->base;
	if (usable_head->length == 0x1000) {
		struct memory_range *n = usable_head->next;
		obj_pool_free(mrng_pool, usable_head);
		usable_head = n;
	}
	return begin;
}

void *get_page(void) {
	if (status == 1)
		printk("Early get page ");
	if (head) {
		void *ret = head;
		head = head->next;
		printk("get_page: %p\n", ret);
		return ret;
	}
	uint64_t ret = kmmap_to_vmbase(get_phys_page());
	printk("get_page: %p\n", ret);
	return (void *)ret;
}

void drop_page(void *pg) {
	if ((uint64_t)pg&0xfff)
		panic("Dropping invalid page");
	struct free_page *new = pg;
	new->next = head;
	head = new;
}
