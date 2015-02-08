#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "util.h"

int execvp(const char *name, char * const* argv) {
	char *env_path = strdup(getenv("PATH"));
	int npath, paths = 0;
	char **path = NULL;
	int flen = strlen(argv[0]);
	npath = split(env_path, ':', &path, &paths, SPLIT_NOESC);

	int i;
	int tmp_errno = 0;
	for (i = 0; i < npath; i++) {
		int plen = strlen(path[i]);
		char *buf = malloc(plen+flen+2);
		strcpy(buf, path[i]);
		buf[plen] = '/';
		buf[plen+1] = '\0';
		strcat(buf, argv[0]);
		printf("Trying: %s\n", buf);

		execve(buf, argv, environ);
		if (errno != ENOENT || tmp_errno == 0)
			tmp_errno = errno;
	}
	if (tmp_errno == EACCES || tmp_errno == EPERM)
		printf("%s: permission denied\n", name);
	else if (tmp_errno == ENOENT)
		printf("%s: command not found\n", name);
	else
		printf("%s: failed to execv\n", name);
	return -1;
}
