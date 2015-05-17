#include <sys/printk.h>
#include <sys/mm.h>
#include <sys/panic.h>
#include <macro.h>
#include <sys/interrupt.h>

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
static int early_get_page = 1;

void page_allocator_init(uint64_t extra_page, int count,
			 struct smap_t *smap, int nsmap,
			 struct memory_range *rm) {
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
	printk("Page allocator init early done\n");

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
	early_get_page = 0;

	//Now everything is setup, let's map the whole physical memory
	struct memory_range *mr = usable_head;
	while (mr) {
		int begin = ALIGN_UP(mr->base, PAGE_SIZE_BIT);
		int end = ALIGN(mr->base+mr->length, PAGE_SIZE_BIT);
		int end2 = ALIGN_UP(begin, PAGE_SIZE_BIT+9);
		int end3 = ALIGN(mr->base+mr->length, PAGE_SIZE_BIT+9);
		//Map up to the first address aligned to 2M page
		for (i=begin; i<min(end, end2); i+=PAGE_SIZE)
			map_page(i, KERN_VMBASE+i, 0, PTE_W);
		for (i=end2; i<end3; i+=512*PAGE_SIZE)
			map_page(i, KERN_VMBASE+i, 1, PTE_W);
		for (i=end3; i<end; i+=PAGE_SIZE)
			map_page(i, KERN_VMBASE+i, 0, PTE_W);
		mr = mr->next;
	}
}

static uint64_t get_phys_page_locked(void) {
	if (early_get_page == 1)
		panic("Early get phys page");
	if (head) {
		void *ret = head;
		head = head->next;
		uint64_t p = ((uint64_t)ret)-KERN_VMBASE;
		//printk("Get phys page': %p\n", p);
		return p;
	}

	uint64_t begin = usable_head->base;
	if (usable_head->length == PAGE_SIZE) {
		struct memory_range *n = usable_head->next;
		obj_pool_free(mrng_pool, usable_head);
		usable_head = n;
	}else {
		usable_head->base += PAGE_SIZE;
		usable_head->length -= PAGE_SIZE;
	}
	//printk("Get phys page: %p\n", begin);
	return begin;
}

uint64_t get_phys_page(void) {
	disable_interrupts();
	uint64_t ret = get_phys_page_locked();
	enable_interrupts();
	return ret;
}

void *get_page(void) {
	if (early_get_page == 1)
		printk("Early get page ");
	disable_interrupts();
	if (head) {
		void *ret = head;
		head = head->next;
		//printk("get_page: %p\n", ret);
		enable_interrupts();
		return ret;
	}
	uint64_t ret = get_phys_page_locked()+KERN_VMBASE;
	enable_interrupts();
	//printk("get_page: %p\n", ret);
	return (void *)ret;
}

void drop_page(void *pg) {
	disable_interrupts();
	if ((uint64_t)pg&0xfff)
		panic("Dropping invalid page");
	struct free_page *new = pg;
	new->next = head;
	head = new;
	enable_interrupts();
}
