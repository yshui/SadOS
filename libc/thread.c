#include <uapi/thread.h>
#include <syscall.h>
#include "simple_syscalls.h"
sys1(get_thread_info, struct thread_info *, long)
sys2(create_task, int, struct thread_info *, long)
