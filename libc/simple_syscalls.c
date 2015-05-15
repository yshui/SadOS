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
#include <syscall.h>
#include <stdlib.h>
#include <compiler.h>
#include "err.h"
#include "simple_syscalls.h"


__noreturn void exit(int status) {
	syscall_1(NR_exit, (uint64_t)status);
	__builtin_unreachable();
}
sys2(open, const char *, int, int)
sys3(read, int, void *, size_t, ssize_t)
sys3(write, int, const void *, size_t, ssize_t)
sys0(fork, pid_t)
sys0(getpid, pid_t)
sys0(getppid, pid_t)
sys3(execve, const char *, char *const *, char *const *, int)
sys4(wait4, pid_t, int *, int, void *, pid_t)
sys1(close, int, int)
sys3(lseek, int, off_t, int, off_t)
sys1(pipe, int *, int)
sys1(dup, int, int)
sys2(dup2, int, int, int)
sys1(chdir, const char *, int)
sys1(alarm, unsigned int, unsigned int)
sys2(nanosleep, const struct timespec *, struct timespec *, int)
sys3(getdents, unsigned int, struct linux_dirent *, unsigned int, int)
#if 0
static __inline uint64_t
syscall_6(uint64_t n, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
	  uint64_t a5, uint64_t a6) {
	uint64_t ret;
	register uint64_t r10 __asm__("r10") = a4;
	register uint64_t r8 __asm__("r8") = a5;
	register uint64_t r9 __asm__("r9") = a6;
	__asm__ __volatile__ ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2),
						  "d"(a3), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
	return ret;
}

void *mmap(void *a1, size_t a2, int a3, int a4, int a5, off_t a6) {
	uint64_t ret = syscall_6(SYS_mmap, (uint64_t)a1,
			 (uint64_t)a2, (uint64_t)a3,
			 (uint64_t)a4, (uint64_t)a5,
			 (uint64_t)a6);
	if (ISERR(ret)) {
		errno = -ret;
		return NULL;
	}
	return (void *)ret;
}
#endif
