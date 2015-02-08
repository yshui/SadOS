#include <stdlib.h>
#include "simple_syscalls.h"

pid_t waitpid(pid_t pid, int *status, int options) {
	return wait4(pid, status, options, NULL);
}
