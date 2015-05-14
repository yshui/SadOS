#pragma once
#include <sys/list.h>
#include <sys/mm.h>
#include <sys/defs.h>
#include <sys/sched.h>
#include <errno.h>
#include <string.h>

struct page_list {
	void *page; //page == NULL means zero page
	struct list_node next;
	struct obj_pool *pg;
};

struct list_head *copy_from_user(void *buf, size_t len);
int copy_to_user(struct list_head *pgs, void *buf, size_t len);
int copy_to_user_simple(void *src, void *user_buf, size_t len);
int copy_from_user_simple(void *user_buf, void *dst, size_t len);
