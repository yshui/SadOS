#pragma once
#include <sys/mm.h>
#include <sys/defs.h>
#include <string.h>
struct fdtable {
	void **file;
	uint64_t max_fds;
};

static inline int fdtable_insert(struct fdtable *fds, void *d) {
	int i;
	for (i = 0; i < fds->max_fds; i++) {
		if (!fds->file[i]) {
			fds->file[i] = d;
			return i;
		}
	}
	return -1;
}

static inline struct fdtable *fdtable_new(void) {
	struct fdtable *ret = get_page();
	memset(ret, 0, PAGE_SIZE);
	ret->file = (void *)(((uint8_t *)ret)+sizeof(struct fdtable));
	ret->max_fds = (PAGE_SIZE-sizeof(struct fdtable))/8;
	return ret;
}
