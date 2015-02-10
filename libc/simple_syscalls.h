#pragma once
#include <stdlib.h>
#include "time.h"
struct linux_dirent;
pid_t wait4(pid_t, int *, int, void *);
int nanosleep(const struct timespec *req, struct timespec *rem);
int getdents(unsigned int, struct linux_dirent *, unsigned int);
