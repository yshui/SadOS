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
#include <sys/printk.h>
#include <sys/portio.h>
#include <sys/defs.h>
#include <sys/interrupt.h>

#define MASTER (0x20)
#define SLAVE  (0xA0)

static inline void _i8259_eoi(unsigned char irq) {
	if (irq >= 8) {
		io_wait();
		outb(SLAVE, 0x20); //EOI;
	}
	io_wait();
	outb(MASTER, 0x20);
	//printk("EOI%d\n",irq);
}

void i8259_eoi(unsigned char irq) {
	_i8259_eoi(irq);
}

uint64_t i8259_int_eoi(int v) {
	_i8259_eoi(v-32);
	return 0;
}

void i8259_init(void) {
	int i;
	for (i = 32; i < 48; i++)
		int_register_eoi(i, i8259_int_eoi);
}
