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
#include <sys/defs.h>
#include <sys/panic.h>
#include <sys/sched.h>
int gp_handler(uint64_t *rsp) {
	uint64_t saved_rip = *(rsp+1);
	if (((long)saved_rip) < 0)
		panic("GP in kernel, %p\n", saved_rip);
	printk("User space program GP, %p\n", saved_rip);
	kill_current(2);
	return 1;
}
int ud_handler(uint64_t *rsp) {
	uint64_t saved_rip = *(rsp+1);
	if (((long)saved_rip) < 0)
		panic("UD in kernel, %p\n", saved_rip);
	printk("User space program UD, %p\n", saved_rip);
	kill_current(2);
	return 1;
}
int de_handler(uint64_t *rsp) {
	uint64_t saved_rip = *(rsp+1);
	if (((long)saved_rip) < 0)
		panic("DE in kernel, %p\n", saved_rip);
	printk("User space program divide-by-zero, %p\n", saved_rip);
	kill_current(2);
	return 1;
}
