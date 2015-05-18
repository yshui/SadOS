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
#include <sys/defs.h>
struct binradix_node;
void *binradix_find(struct binradix_node *, uint64_t);
void *binradix_insert(struct binradix_node *, uint64_t, void *);
struct binradix_node *binradix_new(void);
void *binradix_delete(struct binradix_node *root, uint64_t key);

typedef void (*visitor_fn)(void *node, void *d);
void binradix_for_each(struct binradix_node *, void *, visitor_fn);
