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
#ifndef __SYS_SYSCALL_H
#define __SYS_SYSCALL_H

#define SYS_read        0
#define SYS_write       1
#define SYS_open        2
#define SYS_close       3
#define SYS_lseek       8
//#define SYS_mmap        9
//#define SYS_mprotect   10
//#define SYS_munmap     11
#define SYS_brk        12
#define SYS_pipe       22
#define SYS_dup        32
#define SYS_dup2       33
#define SYS_nanosleep  35
#define SYS_alarm      37
#define SYS_getpid     39
#define SYS_fork       57
#define SYS_execve     59
#define SYS_exit       60
#define SYS_wait4      61
#define SYS_getdents   78
#define SYS_getcwd     79
#define SYS_chdir      80
#define SYS_getppid   110

#endif
