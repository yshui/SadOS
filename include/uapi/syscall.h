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

#define NR_open_port        0
#define NR_get_response     1
#define NR_request          2
#define NR_port_connect     3
#define NR_asnew            4
#define NR_sendpage         5
#define NR_exit             6
#define NR_get_thread_info  7
#define NR_create_task      8
#define NR_asdestroy        9
#define NR_close           10
#define NR_pop_request     11
#define NR_respond         12
#define NR_wait_on         13
#define NR_wait_on_port    14
#define NR_munmap          15
#define NR_dup0            16
#define NR_get_physical    17
#define NR_getpid          18
#define NR_getppid         19

//Deprecated
#define NR_lseek       8
#define NR_pipe       22
#define NR_nanosleep  35
#define NR_alarm      37
#define NR_execve     59
#define NR_wait4      61
#define NR_getdents   78
