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
typedef void (*handler_t)(int vector);

static inline void disable_interrupts(void) {
	__asm__ volatile ("cli");
}

static inline void enable_interrupts(void) {
	__asm__ volatile ("sti");
}

int int_register_handler(uint64_t, handler_t);
void int_deregister_handler(uint64_t);
int int_register_eoi(uint64_t, handler_t);
