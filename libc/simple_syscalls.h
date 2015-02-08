#pragma once
#include <stdlib.h>
#include "time.h"
pid_t wait4(pid_t, int *, int, void *);
int nanosleep(const struct timespec *req, struct timespec *rem);
