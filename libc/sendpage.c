#include <sys/defs.h>
#include <syscall.h>
#include "simple_syscalls.h"

//Note the return value might not be an valid address in current as
void *sendpage(int as, uint64_t src, uint64_t dst, size_t len) {
	long ret = syscall_4(NR_sendpage, as, src, dst, len);
	if (ret < 0) {
		errno = -ret;
		return NULL;
	}
	return (void *)ret;
}

