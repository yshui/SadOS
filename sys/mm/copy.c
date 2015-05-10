#include <sys/copy.h>
#include <sys/interrupt.h>
#include <bitops.h>
struct list_head *copy_from_user(void *buf, size_t len) {
	if (validate_range((uint64_t)buf, len))
		return NULL;
	struct obj_pool *pg = obj_pool_create(sizeof(struct page_list));
	struct list_head *ret = obj_pool_alloc(pg);
	uint64_t addr = (uint64_t)buf;
	while (len) {
		struct page_entry *pe = get_allocated_page(current->as, addr);
		struct page_list *pl = obj_pool_alloc(pg);
		pl->pg = pg;
		pl->page = NULL;
		list_add_tail(ret, &pl->next);
		size_t cp_len = PAGE_SIZE;
		if (cp_len > len)
			cp_len = len;
		if (pe) {
			uint8_t *page = get_page();
			memcpy(page, phys_to_virt(pe->p->phys_addr), cp_len);
			pl->page = page;
		}
		len -= cp_len;
	}
	return ret;
}
#if 0
int copy_to_user(struct list_head *pgs, void *buf, size_t len) {
	disable_interrupts();
	if (validate_range((uint64_t)buf, len))
		return -EINVAL;

	//Copy data
	//Map all the pages in the range in process
	struct page_list *pl = list_top(pgs, struct page_list, next);
	uint64_t addr = (uint64_t)buf, end = addr+len;
	while(addr < end) {
		uint64_t aaddr = ALIGN(addr, PAGE_SIZE_BIT);
		struct page_entry *pe = get_allocated_page(current->as, aaddr);
		uint8_t *page = NULL;
		if (!pe)
			page = get_page();
		else {
			if (pe->p->snap_count > 0)
				unshare_page(pe);
			page = phys_to_virt(pe->p->phys_addr);
		}

		uint64_t offset = aaddr-addr, cp_len = PAGE_SIZE-offset;

		if (cp_len > end-addr)
			cp_len = end-addr;
		if (cp_len > len)
			cp_len = len
		memcpy(page+offset, )

	}
}
#endif
