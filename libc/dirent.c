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
#include "simple_syscalls.h"
struct linux_dirent {
	long           d_ino;       /* inode number */
	off_t          d_off;       /* not an offset; see NOTES */
	unsigned short d_reclen;    /* length of this record */
	char           d_name[0]; /* filename */
};

struct dirp {
	int fd;
	char buf[1024];
	struct linux_dirent *off, *end;
};

void *opendir(const char *path) {
	struct dirp *ret = malloc(sizeof(struct dirp));
	ret->fd = open(path, O_RDONLY|O_DIRECTORY);
	ret->off = ret->end = NULL;
	return ret;
}
int getdents(int fd, const void *b, size_t c) {
	return 0;
}

struct dirent *readdir(void *_p) {
	struct dirp *p = _p;
	static struct dirent ret;
	if (p->off >= p->end) {
		int len = getdents(p->fd, (void *)p->buf, sizeof(p->buf));
		if (len <= 0) {
			if (len < 0 && len != -ENOENT) errno = -len;
			return NULL;
		}
		p->end = (struct linux_dirent *)(p->buf+len);
		p->off = (struct linux_dirent *)p->buf;
	}
	strcpy(ret.d_name, p->off->d_name);
	ret.d_off = p->off->d_off;
	ret.d_ino = p->off->d_ino;
	ret.d_reclen = p->off->d_reclen;
	p->off = ((void *)p->off)+p->off->d_reclen;
	return &ret;
}

int closedir(void *_p) {
	struct dirp *p = _p;
	close(p->fd);
	free(_p);
	return 0;
}
