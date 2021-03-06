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
extern int current_pid;
extern int (*print_handler)(const char *);
int _printk(const char *fmt, ...);
#define printk(...) \
	do { \
		_printk("[" __FILE__ ":%d] ", __LINE__ ); \
		_printk("(pid %d) ", current_pid); \
		_printk(__VA_ARGS__); \
	}while(0)

