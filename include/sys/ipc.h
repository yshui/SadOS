#pragma once
#include <sys/defs.h>
struct fdtable {
	void **file;
	uint64_t max_fds;
};
