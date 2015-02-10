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

char *getcwd(char *buf, size_t size)
{
	char tmp[PATH_MAX];
	if (!buf) {
		buf = tmp;
		size = PATH_MAX;
	}

	if (!size) {
		errno = EINVAL;
		return NULL;
	}

	int64_t ret = syscall_2(SYS_getcwd, (uint64_t)buf, size);
	if (ret < 0) {
		errno = -ret;
		return NULL;
	}

	if (buf == tmp)
		return strdup(tmp);
	return buf;
}
