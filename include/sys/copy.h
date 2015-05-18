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
#pragma once
#include <sys/list.h>
#include <sys/mm.h>
#include <sys/defs.h>
#include <sys/sched.h>
#include <errno.h>
#include <string.h>

struct page_list {
	struct page *p;
	struct list_node next;
	uint64_t flags;
};

struct page_list_head {
	uint64_t start_offset;
	uint64_t npage;
	struct list_head *ph;
	struct obj_pool *pg;
};

struct page_list_head *map_from_user(void *user_buf, size_t len);
uint64_t map_to_user(struct page_list_head *plh, uint64_t vaddr);
uint64_t map_to_user_as(struct address_space *, struct page_list_head *, uint64_t vaddr);
void page_list_free(struct page_list_head *plh);
int copy_to_user_simple(void *src, void *user_buf, size_t len);
int copy_from_user_simple(void *user_buf, void *dst, size_t len);
