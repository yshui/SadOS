#include <stdlib.h>
#include <errno.h>
#include <syscall.h>

#include "brk.h"

static void *current_brk = NULL;

static inline void *__brk(void *newbrk) {
	uint64_t ret = syscall_1(SYS_brk, (uint64_t)newbrk);
	void *nowbrk = (void *)ret;
	return nowbrk;
}

int brk(void *newbrk) {
	current_brk = __brk(newbrk);
	if (current_brk != newbrk) {
		errno = ENOMEM;
		return -1;
	}
	return 0;
}

void *sbrk(size_t inc) {
	if (!current_brk)
		current_brk = __brk(0);
	if (!inc)
		return current_brk;
	void *oldbrk = current_brk;
	current_brk = __brk(current_brk+inc);
	if (current_brk == oldbrk) {
		errno = ENOMEM;
		return (void *)-1;
	}
	return oldbrk;
}
