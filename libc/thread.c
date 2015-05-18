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
#include <uapi/thread.h>
#include <syscall.h>
#include "simple_syscalls.h"
#include <uapi/aspace.h>
#include <as.h>
#include <errno.h>
sys1(get_thread_info, struct thread_info *, long)
sys3(create_task, int, struct thread_info *, int, long)
uint32_t fork(void) {
	int as = asnew(AS_SNAPSHOT);
	if (as < 0) {
		if (errno == EFORKED)
			return 0;
		return -1;
	}
	struct thread_info ti;
	get_thread_info(&ti);
	ti.rax = -EFORKED;
	return create_task(as, &ti, 0);
}
