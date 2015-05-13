#include <sys/copy.h>
#include <sys/interrupt.h>
#include <bitops.h>
struct list_head *copy_from_user(void *buf, size_t len) {
	disable_interrupts();
	if (validate_range((uint64_t)buf, len)) {
		enable_interrupts();
		return NULL;
	}
	//Copy data
	//Map all the pages in the range in process
	struct obj_pool *pg = obj_pool_create(sizeof(struct page_list));
	struct list_head *ret = obj_pool_alloc(pg);
	list_head_init(ret);
	uint8_t *page = get_page();
	uint64_t pl_offset = 0;
	uint64_t addr = (uint64_t)buf;
	while(len) {
		uint64_t aaddr = ALIGN(addr, PAGE_SIZE_BIT);
		struct page_entry *pe = get_allocated_page(current->as, aaddr);

		uint64_t offset = addr-aaddr, cp_len = PAGE_SIZE-offset;
		uint8_t *src = pe ? phys_to_virt(pe->p->phys_addr) : NULL;

		if (cp_len > len)
			cp_len = len;
		if (cp_len > PAGE_SIZE-pl_offset) {
			size_t cp_len2 = PAGE_SIZE-pl_offset;
			if (src)
				memcpy(page+pl_offset, src+offset, cp_len2);
			else
				memset(page+pl_offset, 0, cp_len2);

			struct page_list *pl = obj_pool_alloc(pg);
			pl->page = page;
			pl->pg = pg;
			list_add_tail(ret, &pl->next);
			page = get_page();

			pl_offset = 0;
			offset += cp_len2;
			cp_len -= cp_len2;
			len -= cp_len2;
		}
		if (src)
			memcpy(page+pl_offset, src+offset, cp_len);
		else
			memset(page+pl_offset, 0, cp_len);
		addr = aaddr+PAGE_SIZE;
		len -= cp_len;
	}
	struct page_list *pl = obj_pool_alloc(pg);
	pl->page = page;
	pl->pg = pg;
	list_add_tail(ret, &pl->next);
	enable_interrupts();
	return ret;
}

int copy_to_user(struct list_head *pgs, void *buf, size_t len) {
	disable_interrupts();
	if (validate_range((uint64_t)buf, len)) {
		enable_interrupts();
		return -EINVAL;
	}

	//Copy data
	//Map all the pages in the range in process
	struct page_list *pl = list_top(pgs, struct page_list, next);
	uint64_t pl_offset = 0;
	uint64_t addr = (uint64_t)buf;
	while(len) {
		uint64_t aaddr = ALIGN(addr, PAGE_SIZE_BIT);
		struct page_entry *pe = get_allocated_page(current->as, aaddr);
		uint8_t *page = NULL;
		if (!pe) {
			page = get_page();
			memset(page, 0, PAGE_SIZE);
			struct page *p = manage_page((uint64_t)page);
			address_space_assign_page(current->as, p, aaddr, PF_SHARED);
		} else {
			if (pe->p->snap_count > 0)
				unshare_page(pe);
			page = phys_to_virt(pe->p->phys_addr);
		}

		uint64_t offset = addr-aaddr, cp_len = PAGE_SIZE-offset;

		if (cp_len > len)
			cp_len = len;
		if (cp_len > PAGE_SIZE-pl_offset) {
			size_t cp_len2 = PAGE_SIZE-pl_offset;
			if (pl->page)
				memcpy(page+offset, pl->page+pl_offset, cp_len2);

			pl = list_next(pgs, pl, next);
			pl_offset = 0;
			offset += cp_len2;
			cp_len -= cp_len2;
			len -= cp_len2;
		}
		if (pl->page)
			memcpy(page+offset, pl->page+pl_offset, cp_len);
		addr = aaddr+PAGE_SIZE;
		len -= cp_len;
	}
	enable_interrupts();
	return 0;
}

int copy_to_user_simple(void *src, void *buf, size_t len) {
	if (len > 4096)
		panic("Please use copy_to_user()");
	disable_interrupts();
	if (validate_range((uint64_t)buf, len)) {
		enable_interrupts();
		return -1;
	}

	uint64_t aaddr = ALIGN((uint64_t)buf, PAGE_SIZE_BIT);
	struct page_entry *pe = get_allocated_page(current->as, aaddr);
	char *page;
	if (!pe) {
		page = get_page();
		memset(page, 0, PAGE_SIZE);
		struct page *p = manage_page((uint64_t)page);
		address_space_assign_page(current->as, p, aaddr, PF_SHARED);
	} else {
		if (pe->p->snap_count > 0)
			unshare_page(pe);
		page = phys_to_virt(pe->p->phys_addr);
	}
	memcpy(page+((uint64_t)buf-aaddr), src, len);
	enable_interrupts();
	return 0;
}
