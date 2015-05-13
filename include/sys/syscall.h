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
#include <sys/defs.h>

#define NR_open_port    0
#define NR_get_response 1
#define NR_request      2
#define NR_port_connect 3
#define NR_asnew        4
#define NR_sendpage     5

struct syscall_metadata {
	void *ptr;
	uint32_t syscall_nr;
};
#define __MAP0(m,...)
#define __MAP1(m,t,a) m(t,a)
#define __MAP2(m,t,a,...) m(t,a), __MAP1(m,__VA_ARGS__)
#define __MAP3(m,t,a,...) m(t,a), __MAP2(m,__VA_ARGS__)
#define __MAP4(m,t,a,...) m(t,a), __MAP3(m,__VA_ARGS__)
#define __MAP5(m,t,a,...) m(t,a), __MAP4(m,__VA_ARGS__)
#define __MAP6(m,t,a,...) m(t,a), __MAP5(m,__VA_ARGS__)
#define __MAP(n,...) __MAP##n(__VA_ARGS__)

#define __ARG_DECL(t, a) t a

#define SYSCALL(nargs, x, ...) \
	long sys_##x(__MAP(nargs, __ARG_DECL, __VA_ARGS__)); \
	static struct syscall_metadata \
	__attribute__ ((section("__syscalls_metadata"), used)) \
	__syscall_meta_##x = { \
		.syscall_nr = NR_##x, \
		.ptr = sys_##x, \
	}; \
	long sys_##x(__MAP(nargs, __ARG_DECL, __VA_ARGS__))

#define NR_read        0
#define NR_write       1
#define NR_open        2
#define NR_close       3
#define NR_lseek       8
//#define NR_mmap        9
//#define NR_mprotect   10
//#define NR_munmap     11
#define NR_brk        12
#define NR_pipe       22
#define NR_dup        32
#define NR_dup2       33
#define NR_nanosleep  35
#define NR_alarm      37
#define NR_getpid     39
#define NR_fork       57
#define NR_execve     59
#define NR_exit       60
#define NR_wait4      61
#define NR_getdents   78
#define NR_getcwd     79
#define NR_chdir      80
#define NR_getppid   110
