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
#include <sys/copy.h>
#include <sys/interrupt.h>
#include <bitops.h>

//Map a range of pages from user, increase their snapshot count
struct page_list_head *map_from_user(void *buf, size_t len) {
	disable_interrupts();
	if (validate_range((uint64_t)buf, len)) {
		enable_interrupts();
		return ERR_PTR(-EINVAL);
	}
	struct obj_pool *pg = obj_pool_create(sizeof(struct page_list));
	struct page_list_head *res = obj_pool_alloc(pg);
	res->ph = obj_pool_alloc(pg);
	res->start_offset = 0;
	res->pg = pg;
	list_head_init(res->ph);

	uint64_t abuf = ALIGN((uint64_t)buf, PAGE_SIZE_BIT);
	res->start_offset = (uint64_t)buf-abuf;
	if (abuf != (uint64_t)buf) {
		char *page = get_page();
		memset(page, 0, PAGE_SIZE);

		struct page_entry *pe = get_allocated_page(current->as, abuf);
		if (pe) {
			uint64_t poffset = (uint64_t)buf-abuf;
			uint64_t plen = PAGE_SIZE-poffset;
			if (plen > len)
				plen = len;
			memcpy(page+poffset, phys_to_virt(pe->p->phys_addr)+poffset, plen);
		}
		struct page_list *pl = obj_pool_alloc(pg);
		pl->p = manage_page((uint64_t)page);
		pl->flags = PF_SHARED;
		printk("src addr: %p, phys addr: %p (copy)\n", abuf, pl->p->phys_addr);
		list_add_tail(res->ph, &pl->next);
		res->npage = 1;
		if ((uint64_t)buf+len <= abuf+PAGE_SIZE) {
			enable_interrupts();
			return res;
		}
	} else
		res->npage = 0;

	uint64_t end = (uint64_t)buf+len;
	uint64_t aend = ALIGN(end, PAGE_SIZE_BIT);

	for (uint64_t i = abuf+PAGE_SIZE; i < aend; i += PAGE_SIZE) {
		struct page_list *pl = obj_pool_alloc(pg);
		pl->p = NULL;
		struct page_entry *pe = get_allocated_page(current->as, i);
		if (pe) {
			pl->p = pe->p;
			pl->flags = PF_SNAPSHOT;
			pe->p->ref_count++;
			pe->p->snap_count++;
			printk("src addr: %p, phys addr: %p (snap)\n", i, pe->p->phys_addr);
		}
		list_add_tail(res->ph, &pl->next);
		res->npage++;
	}

	if (end != aend) {
		char *page = get_page();
		memset(page, 0, PAGE_SIZE);
		struct page_entry *pe = get_allocated_page(current->as, aend);
		if (pe) {
			uint64_t len = (uint64_t)end-aend;
			memcpy(page, phys_to_virt(pe->p->phys_addr), len);
		}
		struct page_list *pl = obj_pool_alloc(pg);
		pl->p = manage_page((uint64_t)page);
		pl->flags = PF_SHARED;
		printk("src addr: %p, phys addr: %p (copy)\n", end, pl->p->phys_addr);
		list_add_tail(res->ph, &pl->next);
		res->npage++;
	}
	enable_interrupts();
	return res;
}

uint64_t map_to_user_as(struct address_space *as, struct page_list_head *plh, uint64_t vaddr) {
	//Map pages in page list to user space

	if (!IS_ALIGNED(vaddr, PAGE_SIZE_BIT))
		panic("Unaligned");

	struct vm_area *vma;
	if (vaddr)
		vma = insert_vma_to_vaddr(as, vaddr, PAGE_SIZE*plh->npage, VMA_RW);
	else
		vma = insert_vma_to_any(as, PAGE_SIZE*plh->npage, VMA_RW);

	if (PTR_IS_ERR(vma))
		return -ENOMEM;

	struct page_list *pl;
	vaddr = vma->vma_begin;
	list_for_each(plh->ph, pl, next) {
		if (pl->p) {
			printk("map to user: %p->%p\n", pl->p->phys_addr, vaddr);
			address_space_assign_page_with_vma(vma, pl->p, vaddr, PF_SNAPSHOT);
		}
		vaddr += PAGE_SIZE;
	}
	return vma->vma_begin;
}

uint64_t map_to_user(struct page_list_head *plh, uint64_t vaddr) {
	return map_to_user_as(current->as, plh, vaddr);
}

void page_list_free(struct page_list_head *plh) {
	struct page_list *pl;
	list_for_each(plh->ph, pl, next) {
		if (pl->flags == PF_SNAPSHOT)
			pl->p->snap_count--;
		page_unref(pl->p, 0);
	}
	obj_pool_destroy(plh->pg);
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
	uint64_t leftover = 0;
	if ((uint64_t)buf+len > aaddr+PAGE_SIZE) {
		leftover = (uint64_t)buf+len-aaddr-PAGE_SIZE;
		len = aaddr+PAGE_SIZE-(uint64_t)buf;
	}
	struct page_entry *pe = get_allocated_page(current->as, aaddr);
	char *page;
	if (!pe) {
		page = get_page();
		memset(page, 0, PAGE_SIZE);
		struct page *p = manage_page((uint64_t)page);
		address_space_assign_page(current->as, p, aaddr, PF_SHARED);
		page_unref(p, 0);
	} else {
		if (pe->p->snap_count > 0)
			unshare_page(pe);
		page = phys_to_virt(pe->p->phys_addr);
	}
	memcpy(page+((uint64_t)buf-aaddr), src, len);
	if (leftover) {
		pe = get_allocated_page(current->as, aaddr+PAGE_SIZE);
		if (!pe) {
			page = get_page();
			memset(page, 0, PAGE_SIZE);
			struct page *p = manage_page((uint64_t)page);
			address_space_assign_page(current->as, p, aaddr+PAGE_SIZE, PF_SHARED);
			page_unref(p, 0);
		} else {
			if (pe->p->snap_count > 0)
				unshare_page(pe);
			page = phys_to_virt(pe->p->phys_addr);
		}
		memcpy(page, src+len, leftover);
	}
	enable_interrupts();
	return 0;
}

int copy_from_user_simple(void *src, void *dst, size_t len) {
	if (len > 4096)
		panic("Please use copy_from_user()");
	disable_interrupts();
	memset(dst, 0 ,len);
	if (validate_range((uint64_t)src, len)) {
		enable_interrupts();
		return -1;
	}

	uint64_t aaddr = ALIGN((uint64_t)src, PAGE_SIZE_BIT);
	uint64_t leftover = 0;
	if ((uint64_t)src+len > aaddr+PAGE_SIZE) {
		leftover = (uint64_t)src+len-aaddr-PAGE_SIZE;
		len = aaddr+PAGE_SIZE-(uint64_t)src;
	}
	struct page_entry *pe = get_allocated_page(current->as, aaddr);
	if (pe) {
		char *ksrc = phys_to_virt(pe->p->phys_addr);
		memcpy(dst, ksrc+(uint64_t)src-aaddr, len);
	}

	if (leftover) {
		pe = get_allocated_page(current->as, aaddr+PAGE_SIZE);
		if (pe) {
			char *ksrc = phys_to_virt(pe->p->phys_addr);
			memcpy(dst+len, ksrc, leftover);
		}
	}
	enable_interrupts();
	return 0;

}
