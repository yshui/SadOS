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
#include <limits.h>
#include <errno.h>
#include <syscall.h>
#include <string.h>

char __cwd[PATH_MAX];

char *getcwd(char *buf, size_t size)
{
	if (!buf)
		return strdup(__cwd);

	if (!size) {
		errno = EINVAL;
		return NULL;
	}
	if (size < strlen(__cwd)) {
		errno = ERANGE;
		return NULL;
	}
	strcpy(buf, __cwd);
	return buf;
}
