#include <syscall.h>
#include <stdlib.h>
#include "err.h"

#define syscall_ret(ret, rett) if (ISERR(ret)) { \
	errno = -ret; \
	return -1; \
} \
return (rett)ret;

#define sys0(name, rett) rett name(void) { \
	uint64_t ret = syscall_0(SYS_##name); \
	syscall_ret(ret, rett) \
}

#define sys1(name, type1, rett) rett name(type1 a1) { \
	uint64_t ret = syscall_1(SYS_##name, (uint64_t)a1); \
	syscall_ret(ret, rett) \
}

#define sys2(name, type1, type2, rett) rett name(type1 a1, type2 a2) { \
	uint64_t ret = syscall_2(SYS_##name, (uint64_t)a1, \
			      (uint64_t)a2); \
	syscall_ret(ret, rett) \
}

#define sys3(name, type1, type2, type3, rett) \
rett name(type1 a1, type2 a2, type3 a3) { \
	uint64_t ret = syscall_3(SYS_##name, (uint64_t)a1, \
			      (uint64_t)a2, (uint64_t)a3); \
	syscall_ret(ret, rett) \
}

static __inline uint64_t syscall_4(uint64_t n, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4)
{
	uint64_t ret;
	register uint64_t r10 __asm__("r10") = a4;
	__asm__ __volatile__ ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2),
						  "d"(a3), "r"(r10): "rcx", "r11", "memory");
	return ret;
}

#define sys4(name, type1, type2, type3, type4, rett) \
rett name(type1 a1, type2 a2, type3 a3, type4 a4) { \
	uint64_t ret = syscall_4(SYS_##name, (uint64_t)a1, \
			 (uint64_t)a2, (uint64_t)a3, \
			 (uint64_t)a4); \
	syscall_ret(ret, rett) \
}

void exit(int status) {
	syscall_1(SYS_exit, (uint64_t)status);
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
