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
#include <stdlib.h>
#include "err.h"
#include "time.h"

#define syscall_ret(ret, rett) if (ISERR(ret)) { \
	errno = -ret; \
	return -1; \
} \
return (rett)ret;

#define sys0(name, rett) rett name(void) { \
	uint64_t ret = syscall_0(NR_##name); \
	syscall_ret(ret, rett) \
}

#define sys1(name, type1, rett) rett name(type1 a1) { \
	uint64_t ret = syscall_1(NR_##name, (uint64_t)a1); \
	syscall_ret(ret, rett) \
}

#define sys2(name, type1, type2, rett) rett name(type1 a1, type2 a2) { \
	uint64_t ret = syscall_2(NR_##name, (uint64_t)a1, \
			      (uint64_t)a2); \
	syscall_ret(ret, rett) \
}

#define sys3(name, type1, type2, type3, rett) \
rett name(type1 a1, type2 a2, type3 a3) { \
	uint64_t ret = syscall_3(NR_##name, (uint64_t)a1, \
			      (uint64_t)a2, (uint64_t)a3); \
	syscall_ret(ret, rett) \
}

struct linux_dirent;
pid_t wait4(pid_t, int *, int, void *);
int nanosleep(const struct timespec *req, struct timespec *rem);
int getdents(unsigned int, struct linux_dirent *, unsigned int);
