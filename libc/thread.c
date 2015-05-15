#include <uapi/thread.h>
#include <syscall.h>
#include "simple_syscalls.h"
sys1(get_thread_info, struct thread_info *, long)
sys3(create_task, int, struct thread_info *, int, long)
