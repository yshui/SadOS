#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "str.h"

#include "parse.h"

static inline struct pipe_part *
pipe_part_new(void) {
	struct pipe_part *ret = malloc(sizeof(struct pipe_part));
	ret->argv0 = NULL;
	ret->argc = 0;
	ret->next = NULL;
	return ret;
}

static inline struct argv_part *
argv_part_new(void) {
	struct argv_part *ret = malloc(sizeof(struct argv_part));
	ret->next = NULL;
	ret->str = NULL;
	return ret;
}

struct cmd *parse(void) {
	struct str *w = str_new();
	char p = getchar();
	if (p < 0)
		return NULL;

	struct cmd *ret = malloc(sizeof(struct cmd));
	ret->npipe = 0;
	ret->pipe0 = NULL;

	struct pipe_part *nowp = NULL, **nowpp = &ret->pipe0;
	struct argv_part *nowa = NULL, **nowap = NULL;

	//1: consuming space
	//2: argv part
	//3: expecting "
	//4: expecting '
	int state = 0;
	while(p) {
		if (state == 3 || state == 4) {
			char exp = state == 3 ? '"' : '\'';
			if (p == exp)
				state = 2;
			else {
				int escape = 0;
				if (p == '\\'){
					p = getchar();
					escape = 1;
				}
				if (p < 0)
					break;
				if (p != '\n' || escape != 1)
					append(w, p);
			}
			p = getchar();
			continue;
		}
		switch (p) {
		case '\n':
			if (state == 2)
				dump(w, &nowa->str);
			state = 5;
			break;
		case ' ':
			if (state == 1 || state == 0)
				break;
			if (state == 2) {
				dump(w, &nowa->str);
				str_free(w);
				w = str_new();
				state = 1;
			}
			break;
		case '\'':
		case '"':
			if (state == 0) {
				*nowpp = pipe_part_new();
				nowp = *nowpp;
				nowpp = &nowp->next;
				ret->npipe++;
				nowap = &nowp->argv0;
				nowa = NULL;
				state = 1;
			}
			if (state == 1) {
				*nowap = argv_part_new();
				nowa = *nowap;
				nowap = &nowa->next;
				nowp->argc++;
			}
			state = (p == '\'') ? 4 : 3;
			break;
		case '|':
			if (state == 2) {
				dump(w, &nowa->str);
				str_free(w);
				w = str_new();
			}
			state = 1;
			*nowpp = pipe_part_new();
			nowp = *nowpp;
			nowpp = &nowp->next;
			nowap = &nowp->argv0;
			nowa = NULL;
			ret->npipe++;
			break;
		case '\\':
			p = getchar();
			if (p < 0) {
				printf("syntax error: unexpected end of file\n");
				cmd_free(ret);
				return NULL;
			}
			if (p == '\n')
				break;
		default:
			if (state == 0) {
				*nowpp = pipe_part_new();
				nowp = *nowpp;
				nowpp = &nowp->next;
				ret->npipe++;
				nowap = &nowp->argv0;
				nowa = NULL;
				state = 1;
			}
			if (state == 1) {
				*nowap = argv_part_new();
				nowa = *nowap;
				nowap = &nowa->next;
				nowp->argc++;
			}
			append(w, p);
			state = 2;
			break;
		}
		if (state == 5)
			break;
		p = getchar();
	}
	str_free(w);
	if (state == 3 || state == 4) {
		printf("syntax error: unclosed '%c'\n", state == 3 ? '"' : '\'');
		cmd_free(ret);
		return NULL;
	}
	return ret;
}
