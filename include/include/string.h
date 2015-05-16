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

char *strchr(const char *, char);
size_t strlen(const char *);
char *strdup(const char *);
int strncmp(const char *, const char *, int);
int strcmp(const char *, const char *);
char *strcpy(char *, const char *);
char *strcat(char *, const char *);

void *memcpy(void *, const void *, size_t);
void *memset(void *, char, size_t);
void *memmove(void *, const void *, size_t);
void bcopy(const void *, void *, size_t);
