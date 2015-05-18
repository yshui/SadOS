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
#include <sys/mm.h>
#include <sys/defs.h>
#include <string.h>
struct fdtable {
	void **file;
	uint64_t max_fds;
};

static inline int fdtable_insert(struct fdtable *fds, void *d) {
	int i;
	for (i = 0; i < fds->max_fds; i++) {
		if (!fds->file[i]) {
			fds->file[i] = d;
			return i;
		}
	}
	return -1;
}

static inline struct fdtable *fdtable_new(void) {
	struct fdtable *ret = get_page();
	memset(ret, 0, PAGE_SIZE);
	ret->file = (void *)(((uint8_t *)ret)+sizeof(struct fdtable));
	ret->max_fds = (PAGE_SIZE-sizeof(struct fdtable))/8;
	return ret;
}
