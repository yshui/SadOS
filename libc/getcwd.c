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
char __cwd_new[PATH_MAX];

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

int chdir(const char *path) {
	strcpy(__cwd_new, __cwd);
	if (*path == '/') {
		strncpy(__cwd, path, PATH_MAX);
		return 0;
	}
	char *cwd_end = __cwd_new+strlen(__cwd)-1;
	if (*cwd_end == '/' && cwd_end != __cwd_new)
		cwd_end--;
	while(*path) {
		if (path[0] == '.' && path[1] == '.' &&
		    (path[2] == '/' || path[2] == '\0')) {
			if (cwd_end == __cwd_new) {
				errno = ENOENT;
				return -1;
			}
			while(cwd_end >= __cwd_new && *cwd_end != '/')
				cwd_end--;
			if (cwd_end != __cwd_new)
				*(cwd_end--) = '\0';
			else
				*(cwd_end+1) = '\0';
			if (path[2])
				path += 3;
			else
				break;
		} else {
			const char *next_path = path;
			while(*next_path && *next_path != '/')
				next_path++;
			if (cwd_end+(next_path-path)+1 > __cwd_new+PATH_MAX) {
				errno = ENAMETOOLONG;
				return -1;
			}
			if (cwd_end != __cwd_new)
				*(++cwd_end) = '/';
			cwd_end++;
			strncpy(cwd_end, path, next_path-path);
			cwd_end += next_path-path;
			*(cwd_end--) = '\0';
			if (*next_path)
				path = next_path+1;
			else
				break;
		}
	}
	strcpy(__cwd, __cwd_new);
	return 0;
}
