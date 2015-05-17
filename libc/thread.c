#include <uapi/thread.h>
#include <syscall.h>
#include "simple_syscalls.h"
#include <uapi/aspace.h>
#include <as.h>
sys1(get_thread_info, struct thread_info *, long)
sys3(create_task, int, struct thread_info *, int, long)
uint32_t fork(void) {
	int as = asnew(AS_SNAPSHOT);
	struct thread_info ti;
	get_thread_info(&ti);
	ti.rcx = (uint64_t)__builtin_return_address(0);
	ti.rax = 0;
	return create_task(as, &ti, 0);
}
