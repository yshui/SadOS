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
#include <sys/defs.h>

static inline void __idle(void) {
	__asm__ volatile ("hlt\n");
}

static inline uint64_t read_cr3(void) {
	uint64_t ret;
	__asm__ volatile ("movq %%cr3, %0" : "=r"(ret) : : );
	return ret;
}

static inline uint64_t read_cr2(void) {
	uint64_t ret;
	__asm__ volatile ("movq %%cr2, %0" : "=r"(ret) : : );
	return ret;
}
