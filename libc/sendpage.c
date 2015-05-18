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
#include <sys/defs.h>
#include <syscall.h>
#include "simple_syscalls.h"

//Note the return value might not be an valid address in current as
void *sendpage(int as, uint64_t src, uint64_t dst, size_t len) {
	long ret = syscall_4(NR_sendpage, as, src, dst, len);
	if (ret < 0) {
		errno = -ret;
		return NULL;
	}
	return (void *)ret;
}

sys2(munmap, void *, size_t, long)
