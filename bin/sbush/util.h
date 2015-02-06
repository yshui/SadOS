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
#include <stdio.h>
#include <stdlib.h>

#include "string.h"
static inline int readline(char **buf, int *len) {
	char tmp[128];
	int tlen = 0, off = 0;

	do {
		int ret = scanf("%127[^\n]%n", tmp, &tlen);

		if (ret < 0)
			return ret;

		//Read in the trailing \n
		char ttt;
		scanf("%c", &ttt);

		if (off+tlen+1 > *len) {
			char *newbuf = malloc(off+tlen+1);
			if (*buf)
				strcpy(newbuf, *buf);
			free(*buf);
			*buf = newbuf;
			*len = off+tlen+1;
		}
		strcpy(*buf+off, tmp);
		off += tlen;
		if (tlen < 127) {
			break;
		}
	}while(true);
	return off;
}

static inline void unescape(char *in) {
	char *out = in;
	while(*in) {
		if (*in != '\\') {
			*out = *in;
			out++;
		}
		in++;
	}
	*out = '\0';
}

static inline int count_chr(const char *str, char a) {
	int c = 0;
	while(str && *str) {
		str = strchr(str, a);
		if (!str)
			break;
		c++;
		str++;
	}
	return c;
}

#define SPLIT_UNESC 1
#define SPLIT_NOESC 2
#define SPLIT_ANY 4

static inline int split(char *str, char d, char ***arr, int *len, int opt) {
	if (opt & SPLIT_ANY) {
		while(*str == d)
			str++;
		char *end = str+strlen(str)-1;
		while(*end == d) {
			if (*(end-1) == '\\' && !(opt & SPLIT_NOESC))
				break;
			*(end--) = '\0';
		}
	}
	int c = count_chr(str, d)+1;
	if (c > *len) {
		free(*arr);
		*arr = malloc(sizeof(void *)*c);
		*len = c;
	}
	(*arr)[0] = str;
	c = 1;

	char *tmp = str;
	while(tmp && *tmp) {
		tmp = strchr(tmp, d);
		if (!tmp)
			break;
		if ((tmp != str && *(tmp-1) != '\\') || (opt & SPLIT_NOESC)) {
			if (opt & SPLIT_UNESC)
				unescape((*arr)[c-1]);
			*(tmp++) = '\0';
			if (opt & SPLIT_ANY)
				while(*tmp == d)
					*(tmp++) = '\0';
			(*arr)[c++] = tmp;
		}
		tmp++;
	}
	if (opt & SPLIT_UNESC)
		unescape((*arr)[c-1]);
	return c;
}
