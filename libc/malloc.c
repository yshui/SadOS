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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uapi/mem.h>
#include <bitops.h>
#include <ipc.h>
#include "brk.h"
//we will align allocation to 8 bytes

#define _HUGE BIT(4)
#define _FREE BIT(5)

struct _hdr {
	struct _hdr *next, **prev;
	uint8_t flags;
};

static struct _hdr *_free[13];

void *alloc_new(size_t *s) {
	struct mem_req mreq;
	mreq.type = MAP_NORMAL;
	mreq.len = *s;

	long rd = port_connect(1, sizeof(mreq), &mreq);
	if (rd < 0)
		return NULL;

	struct response res;
	long ret = get_response(rd, &res);
	if (ret < 0)
		return NULL;
	*s = res.len;
	return res.buf;
}

void split(int index) {
	if (index < 5)
		return;
	struct _hdr *tmp = _free[index];
	struct _hdr *tmp2 = (void *)((uint64_t)tmp+(1<<(index-1)));

	//Remove tmp from the index list
	_free[index] = tmp->next;
	if (tmp->next)
		tmp->next->prev = &_free[index];

	//tmp is splited into tmp and tmp2
	tmp->next = tmp2;
	tmp2->prev = &tmp->next;
	tmp2->next = _free[index-1];
	if (tmp2->next)
		tmp2->next->prev = &tmp2->next;
	_free[index-1] = tmp;
	tmp->prev = &_free[index-1];
	tmp->flags = tmp2->flags = _FREE|(index-1);
}

static inline void insert(int order, struct _hdr *x) {
	x->flags = order|_FREE;
	x->prev = &_free[order];
	x->next = _free[order];
	_free[order] = x;
	if (x->next)
		x->next->prev = &x->next;

}

void *malloc(size_t s){
	size_t newsize = s+sizeof(struct _hdr);
	if (newsize > 0x1000) {
		struct _hdr *x = alloc_new(&newsize);
		if (!x)
			return NULL;
		x->flags = _HUGE;
		x->next = (void *)newsize;
		return x+1;
	}
	uint8_t order = 64-__builtin_clzl(newsize), xorder = order;

	while (xorder < 13 && !_free[xorder])
		xorder++;
	if (xorder >= 13) {
		size_t s = 0x1000;
		struct _hdr *x = alloc_new(&s);
		if (!x)
			return NULL;
		insert(12, x);
		xorder = 12;
	}

	while (xorder > order) {
		split(xorder);
		xorder--;
	}
	struct _hdr *x = _free[order];
	x->flags &= ~_FREE;
	_free[order] = x->next;
	if (x->next)
		x->next->prev = &_free[order];
	x->prev = NULL;
	x->next = NULL;
	return x+1;
}
void free(void *ptr){
	if (!ptr)
		return;
	struct _hdr *x = ((struct _hdr *)ptr)-1;

	if (x->flags & _HUGE) {
		uint64_t size = (uint64_t)x->next;
		uint8_t *tmp = (void *)x;
		for (int i = 0; i < size; i+=0x1000) {
			struct _hdr *y = (void *)(tmp+i);
			insert(12, y);
		}
		return;
	}

	if (x->flags > 12) {
		//printf("Invalid free()\n");
		return;
	}

	struct _hdr *buddy = (void *)((uint64_t)x^(1<<x->flags));
	if (buddy->flags & _FREE) {
		//Remove buddy from list
		*(buddy->prev) = buddy->next;
		if (buddy->next)
			buddy->next->prev = buddy->prev;

		struct _hdr *new_hdr;
		if (buddy < x)
			new_hdr = buddy;
		else
			new_hdr = x;

		insert(x->flags+1, new_hdr);
	} else
		insert(x->flags, x);
}
