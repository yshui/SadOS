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
#include <uapi/port.h>

enum io_type {
	IO_READ,
	IO_WRITE,
    IO_OPEN,
    IO_OPENDIR,
    IO_READDIR,
    IO_LSEEK
};

struct io_req {
	int type;
	int flags;
	size_t len;
};

struct io_res {
	int err;
	size_t len;
};
