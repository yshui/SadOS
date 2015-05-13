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
#include <sys/printk.h>
#include <sys/panic.h>

typedef uint64_t (*handler_t)(int vector);
extern int interrupt_disabled;

#define disable_interrupts() _disable_interrupts(__FILE__, __func__, __LINE__)
#define enable_interrupts() _enable_interrupts(__FILE__, __func__, __LINE__)

static inline void _disable_interrupts(const char *file, const char *func, int line) {
	if (interrupt_disabled) {
		interrupt_disabled++;
		printk("Nested disable interrupt %d, at %s:%d (%s)\n", interrupt_disabled, file, line, func);
		return;
	}
	__asm__ volatile ("cli");
	interrupt_disabled = 1;
	printk("Disable interrupt 1, at %s:%d (%s)\n", file, line, func);
}

static inline void _enable_interrupts(const char *file, const char *func, int line) {
	if (!interrupt_disabled)
		panic("Invalid call to enable interrupt, at %s:%d (%s)\n", file, line, func);
	interrupt_disabled--;
	printk("Nested enable interrupt %d, at %s:%d (%s)\n", interrupt_disabled, file, line, func);
	if (!interrupt_disabled)
		__asm__ volatile ("sti");
}

int int_register_handler(uint64_t, handler_t);
void int_deregister_handler(uint64_t);
int int_register_eoi(uint64_t, handler_t);
