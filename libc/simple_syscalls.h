/* Copyright (C) 2015, Yuxuan Shui <yshuiv7@gmail.com> */

/*
 * Everyone is allowed to copy and/or redistribute verbatim
 * copies of this code as long as the following conditions
 * are met:
 *
 * 1. The copyright infomation and license is unchanged.
 *
 * 2. This piece of code is not submitted as or as part of
 *    an assignment to the CSE506 course at Stony Brook
 *    University, unless the submitter is the copyright
 *    holder.
 */
#pragma once
#include <stdlib.h>
#include "time.h"
struct linux_dirent;
pid_t wait4(pid_t, int *, int, void *);
int nanosleep(const struct timespec *req, struct timespec *rem);
int getdents(unsigned int, struct linux_dirent *, unsigned int);
