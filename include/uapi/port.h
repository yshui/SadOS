#pragma once
#include <sys/defs.h>
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

		uint64_t mask = (uint64_t)-1;
		if ((i+1)*64 >= fds->nfds)
			mask = (1ull<<(fds->nfds-i*64))-1;
		ans |= fds->bitmask[i]&mask;
	}
	return ans == 0;
}
