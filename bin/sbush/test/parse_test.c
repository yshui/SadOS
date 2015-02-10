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

#include <stdio.h>
#include "parse.h"
static inline void
pipe_part_print(struct pipe_part *p) {
	struct argv_part *a = p->argv0;
	printf("argc=%d\n", p->argc);
	int n = 0;
	while(a) {
		printf("%d: %s\n", n, a->str);
		a = a->next;
		n++;
	}
}
int main(){
	char xx[512];
	scanf("%[^\n]", xx);

	struct cmd *c = parse(xx);
	if (!c) {
		printf("parse failed\n");
		return 0;
	}
	struct pipe_part *p = c->pipe0;
	printf("npipe=%d\n", c->npipe);

	int n = 0;
	while(p) {
		printf("pipe%d\n", n);
		pipe_part_print(p);
		p = p->next;
		n++;
	}
	cmd_free(c);
}
