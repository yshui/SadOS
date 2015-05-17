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
#include <compiler.h>

static inline void outb(uint16_t port, uint8_t d) {
	__asm__ volatile (
		"outb %%al, %%dx"
		: : "d"(port), "a"(d)
	);
}
static inline uint8_t inb(uint16_t port) {
	uint8_t val;
	__asm__ volatile (
		"inb %%dx, %%al"
		: "=a"(val) : "d"(port)
	);
	return val;
}
static inline void outl(uint16_t port, uint32_t d) {
	__asm__ volatile (
		"outl %%eax, %%dx"
		: : "d"(port), "a"(d)
	);
}
static inline uint32_t inl(uint16_t port) {
	uint32_t val;
	__asm__ volatile (
		"inl %%dx, %%eax"
		: "=a"(val) : "d"(port)
	);
	return val;
}
static inline void io_wait(void) {
    __asm__ volatile ( "outb %%al, $0x80" : : "a"(0) );
}
