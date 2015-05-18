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

struct service_init {
	void (*init)(void);
};

#define SERVICE_INIT(x, name) \
	static struct service_init \
	__attribute__((section("__services"), used)) \
	__name##_init = { \
		.init = x, \
	}
