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

struct splay;
struct splay_node;

typedef int (*splay_cmp)(const void *, const void *);

void *splay_next(struct splay *s, void *data);
void *splay_prev(struct splay *s, void *data);
void *splay_delete(struct splay *s, void *data);
void *splay_insert(struct splay *s, void *data);
void *splay_find(struct splay *s, void *data);
struct splay *splay_new(splay_cmp);
