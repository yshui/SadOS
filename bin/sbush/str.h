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
#include <stdlib.h>

#include "stddef.h"

struct str {
	int len;
	struct str_block *head, *tail;
};
struct str_block {
	int len;
	char s[128];
	struct str_block *next;
};

static inline struct str *
str_new(void) {
	struct str *s = malloc(sizeof(struct str));
	s->head = s->tail = NULL;
	s->len = 0;
	return s;
}
static inline void str_free(struct str *s) {
	struct str_block *tmp = s->head;
	while(tmp) {
		struct str_block *n = tmp->next;
		free(tmp);
		tmp = n;
	}
	free(s);
}
void append(struct str *, char);
int dump(struct str *, char **);
