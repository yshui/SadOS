#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <syscall.h>
#include <string.h>

char *getcwd(char *buf, size_t size)
{
	char tmp[PATH_MAX];
	if (!buf) {
		buf = tmp;
		size = PATH_MAX;
	}

	if (!size) {
		errno = EINVAL;
		return NULL;
	}

	int64_t ret = syscall_2(SYS_getcwd, (uint64_t)buf, size);
	if (ret < 0) {
		errno = -ret;
		return NULL;
	}

	if (buf == tmp)
		return strdup(tmp);
	return buf;
}
