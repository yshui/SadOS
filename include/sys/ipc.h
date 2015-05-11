#pragma once
#include <sys/defs.h>
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
