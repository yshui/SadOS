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

struct argv_part {
	//Unescaped argv parts
	char *str;
	struct argv_part *next;
};

struct pipe_part {
	int argc;
	struct argv_part *argv0;
	struct pipe_part *next;
};

struct cmd {
	int npipe;
	struct pipe_part *pipe0;
};

struct cmd *parse(void);

static inline void pipe_free(struct pipe_part *p) {
	struct argv_part *tmp = p->argv0;
	while(tmp) {
		struct argv_part *n = tmp->next;
		free(tmp->str);
		free(tmp);
		tmp = n;
	}
	free(p);
}

static inline void cmd_free(struct cmd *c) {
	struct pipe_part *tmp = c->pipe0;
	while(tmp) {
		struct pipe_part *n = tmp->next;
		pipe_free(tmp);
		tmp = n;
	}
	free(c);
}
