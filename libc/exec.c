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
#include <errno.h>
#include "util.h"

int execvp(const char *name, char * const* argv) {
	char *env_path = strdup(getenv("PATH"));
	int npath, paths = 0;
	char **path = NULL;
	int flen = strlen(argv[0]);
	npath = split(env_path, ':', &path, &paths, SPLIT_NOESC);

	int i;
	int tmp_errno = 0;
	for (i = 0; i < npath; i++) {
		int plen = strlen(path[i]);
		char *buf = malloc(plen+flen+2);
		strcpy(buf, path[i]);
		buf[plen] = '/';
		buf[plen+1] = '\0';
		strcat(buf, argv[0]);
#ifdef __LIBC_DEBUG
		printf("Trying: %s\n", buf);
#endif

		execve(buf, argv, environ);
		if (errno != ENOENT || tmp_errno == 0)
			tmp_errno = errno;
	}
	errno = tmp_errno;
	return -1;
}
