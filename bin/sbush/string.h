#pragma once
#include <stdlib.h>

#include "stddef.h"
#include "memcpy.h"
#include "memset.h"

static inline char *strchr(const char *in, char c) {
	while(*in) {
		if (*in == c)
			return (char *)in;
		in++;
	}
	return NULL;
}

static inline size_t strlen(const char *in) {
	size_t ret = 0;
	while(*in++)
		ret++;
	return ret;
}

static inline char *strdup(const char *in) {
	int len = strlen(in);
	char *new = malloc(len+1);
	memcpy(new, in, len);
	new[len] = 0;
	return new;
}

static inline int strncmp(const char *a, const char *b, int n) {
	int i;
	for (i = 0; i < n; i++) {
		if (a[i] < b[i])
			return -1;
		if (a[i] > b[i])
			return 1;
	}
	return 0;
}

static inline int strcmp(const char *a, const char *b) {
	while (*a || *b) {
		if (*a < *b)
			return -1;
		if (*a > *b)
			return 1;
		a++;
		b++;
	}
	return 0;
}

static inline char *strcpy(char *dst, const char *src) {
	int len = strlen(src);
	memcpy(dst, src, len+1);
	return dst;
}

static inline char *strcat(char *dst, const char *append) {
	int ldst = strlen(dst);
	strcpy(dst+ldst, append);
	return dst;
}
