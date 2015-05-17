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
#define NR_dup             16

//Deprecated
#define NR_read        0
#define NR_write       1
#define NR_open        2
#define NR_lseek       8
//#define NR_mmap        9
//#define NR_mprotect   10
//#define NR_munmap     11
#define NR_brk        12
#define NR_pipe       22
#define NR_dup2       33
#define NR_nanosleep  35
#define NR_alarm      37
#define NR_getpid     39
#define NR_fork       57
#define NR_execve     59
#define NR_wait4      61
#define NR_getdents   78
#define NR_getcwd     79
#define NR_chdir      80
#define NR_getppid   110
