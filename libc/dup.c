#include <stdlib.h>
#include "dup.h"

int dup(int old_fd) {
	return dup0(old_fd, -1);
}

int dup2(int old_fd, int new_fd) {
	close(new_fd);
	return dup0(old_fd, new_fd);
}
