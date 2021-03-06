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
#include <sys/interrupt.h>
#include <sys/i8259.h>
#include <sys/list.h>
#include <sys/port.h>
#include <sys/sched.h>

#include <sys/printk.h>

extern uint64_t int_entry[];
extern struct list_head int_wait_list[];
int interrupt_disabled = 0;

handler_t htable[224];
handler_t eoitable[256];

uint64_t int_handler(uint64_t vector) {
	handler_t fn = htable[vector-32];
	uint64_t ret = 0;
	if (fn)
		ret = fn(vector);
	struct request *ri;
	list_for_each(&int_wait_list[vector], ri, next_req) {
		if (ri->waited_rw&1)
			wake_up(ri->owner);
		ri->data = (void *)((uint64_t)ri->data|256);
	}
	if (eoitable[vector])
		eoitable[vector](vector);
	return ret;
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
