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
#include <uapi/port.h>

long port_connect(int, size_t, void *);
long get_response(int, struct response *);
long request(int, size_t, void *);
long open_port(int);
long wait_on(struct fd_set *rfds, struct fd_set *wfds, int timeout);
long pop_request(int pd, struct urequest *);
long wait_on_port(int port_number);
long respond(int, size_t, void *);
