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
#include <sys/idt.h>
#include <sys/irq.h>
#include <sys/i8259.h>

#include <sys/printk.h>

extern uint64_t int_entry[];

handler_t htable[224];
handler_t eoitable[256];

void int_handler(uint64_t vector) {
	handler_t fn = htable[vector-32];
	if (fn)
		fn(vector);
	if (eoitable[vector])
		eoitable[vector](vector);
}

int int_register_eoi(uint64_t vector, handler_t fn) {
	if (vector > 255)
		return -1;
	if (eoitable[vector])
		return -1;
	eoitable[vector] = fn;
	return 0;
}

int int_register_handler(uint64_t vector, handler_t fn) {
	if (vector < 32)
		return -1;
	if (htable[vector-32])
		return -1;
	htable[vector-32] = fn;
	return 0;
}

void int_deregister_handler(uint64_t vector) {
	if (vector < 32)
		return;
	htable[vector] = NULL;
}
