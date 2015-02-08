#include "simple_syscalls.h"
unsigned int sleep(unsigned int secs) {
	struct timespec s, rem;
	s.tv_sec = secs;
	s.tv_nsec = 0;
	nanosleep(&s, &rem);
	return rem.tv_sec;
}
