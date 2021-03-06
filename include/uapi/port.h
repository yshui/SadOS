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
#include <sys/defs.h>
#include <string.h>
struct response {
	size_t len;
	void *buf;
};

struct urequest {
	size_t len;
	void *buf;
	int pid;
};

struct fd_set {
	int nfds;
	uint64_t bitmask[8];
};

static inline void fd_set_set(struct fd_set *fds, int fd) {
	if (fd < 0 || fd >= 512)
		return;
	fds->bitmask[fd/64] |= 1<<(fd%64);
	if (fd >= fds->nfds)
		fds->nfds = fd+1;
}

static inline int fd_is_set(struct fd_set *fds, int fd) {
	if (fd < 0 || fd >= fds->nfds)
		return 0;
	return (fds->bitmask[fd/64] >> (fd%64))&1;
}

static inline void fd_zero(struct fd_set *fds) {
	fds->nfds = 0;
	memset(fds, 0, sizeof(*fds));
}

static inline void fd_clear(struct fd_set *fds, int fd) {
	if (fd < 0 || fd >= fds->nfds)
		return;
	fds->bitmask[fd/64] &= ~(1<<(fd%64));
	if (fd == fds->nfds-1)
		fds->nfds--;
}

static inline int fd_set_empty(struct fd_set *fds) {
	uint64_t ans = 0;
	for (int i = 0; i < 8; i++) {
		if (i*64 >= fds->nfds)
			return ans == 0;

		ans |= fds->bitmask[i];
	}
	return ans == 0;
}
