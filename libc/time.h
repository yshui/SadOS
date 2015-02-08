#pragma once
#include <stdlib.h>
typedef int64_t time_t;
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
