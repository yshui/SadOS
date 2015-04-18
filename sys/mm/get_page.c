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
	uint64_t length;
	struct free_page *next;
};
static struct free_page *head = NULL;
void page_allocator_init(uint64_t vmbase, uint64_t end, struct memory_range *rm, int nrm) {
	int i;
	uint64_t now = (vmbase+0xfff)&~0xfff;
	for (i = 0; i < nrm; i++) {
		if (rm[i].base < now)
			panic("Invalid reserved memory range");
		printf("Reserved range %p ~ %p\n", rm[i].base, rm[i].base+rm[i].length);
		if (now+sizeof(struct free_page) >= rm[i].base) {
			//The free_page header might overwrite a reserved range
			now = rm[i].base+rm[i].length;
			continue;
		}
		struct free_page *node = (void *)now;
		node->next = head;
		head = node;
		node->length = (rm[i].base-now)&~0xfff; //Align to page boundary
		now = (rm[i].base+rm[i].length+0xfff)&~0xfff; //Round up to page boundary
	}
	if (end-now >= 0x1000) { //More than 1 page left
		struct free_page *node = (void *)now;
		node->next = head;
		head = node;
		node->length = (end-now)&~0xfff;
	}
}

void *get_page(void) {
	if (!head)
		return NULL;
	if (head->length == 0x1000) {
		void *ret = head;
		head = head->next;
		return ret;
	}
	struct free_page *new = (void *)(((uint64_t)head)+0x1000);
	new->length = head->length-0x1000;
	new->next = head->next;

	void *ret = head;
	head = new;
	return ret;
}

void drop_page(void *pg) {
	if ((uint64_t)pg&0xfff)
		panic("Dropping invalid page");
	struct free_page *new = pg;
	new->next = head;
	new->length = 0x1000;
	head = new;
}
