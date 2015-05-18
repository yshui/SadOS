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
#include <syscall.h>
#include <uapi/port.h>
#include "simple_syscalls.h"

sys3(port_connect, int, size_t, void *, long)
sys2(get_response, int, struct response *, long)
sys3(request, int, size_t, void *, long)
sys3(respond, int, size_t, void *, long)
sys1(open_port, int, long)
sys1(wait_on_port, int, long)
sys3(wait_on, struct fd_set *, struct fd_set *, int, long)
sys2(pop_request, int, struct response *, long)
