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
#include <string.h>

void *memcpy(void *dst0, const void *src0, size_t s) {
	char *dst = dst0;
	const char *src = src0;
	if (dst < src) {
		//Copy forward
		while(s--)
			*(dst++) = *(src++);
	} else {
		//Copy backward
		src += s;
		dst += s;
		while(s--)
			*(--dst) = *(--src);
	}
	return dst0;
}

void *memmove(void *s1, const void *s2, size_t n) {
	return memcpy(s1, s2, n);
}

void bcopy(const void *s1, void *s2, size_t n) {
	memcpy(s2, s1, n);
}

void *memset(void *dst0, char v, size_t s) {
	char *dst = dst0;
	while(s--)
		*(dst++) = v;
	return dst0;
}

char *strchr(const char *in, char c) {
	while(*in) {
		if (*in == c)
			return (char *)in;
		in++;
	}
	return NULL;
}

size_t strlen(const char *in) {
	size_t ret = 0;
	while(*in++)
		ret++;
	return ret;
}

char *strdup(const char *in) {
	int len = strlen(in);
	char *new = malloc(len+1);
	memcpy(new, in, len);
	new[len] = 0;
	return new;
}

int strncmp(const char *a, const char *b, int n) {
	int i;
	for (i = 0; i < n; i++) {
		if (a[i] < b[i])
			return -1;
		if (a[i] > b[i])
			return 1;
	}
	return 0;
}

int strcmp(const char *a, const char *b) {
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

char *strcpy(char *dst, const char *src) {
	int len = strlen(src);
	memcpy(dst, src, len+1);
	return dst;
}

char *strcat(char *dst, const char *append) {
	int ldst = strlen(dst);
	strcpy(dst+ldst, append);
	return dst;
}
