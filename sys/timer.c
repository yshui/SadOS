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
#include <sys/apic.h>
#include <sys/idt.h>
#include <sys/drivers/vga_text.h>
#include <sys/portio.h>
#include <sys/interrupt.h>
#include <sys/i8259.h>
#include <sys/printk.h>
extern char kernend;
extern void timer_wrapper(void);
extern int itoa(long a, int base, char *str, int width, int sign);
//static int calibrating = 0;
//static uint16_t start_pit = 0;

static inline uint16_t i8254_read(void) {
	outb(0x43, 0); //Counter latch
	uint8_t l, h;
	l = inb(0x42);
	h = inb(0x42);
	return (h<<8)+l;
}

uint64_t pit_handler(int v) {
	static volatile uint32_t count = 0;
	count++;
	return 1;
}
extern void pit_wrapper(void);
void timer_init(void) {
	//Init the i8254 chip
	disable_interrupts();
	outb(0x43, 0x36); //Counter 0, lo/hi
	//Set it up for 41HZ
	outb(0x40, 0xAE);
	outb(0x40, 0x71); //Initial value to 65535

	int_register_handler(32, pit_handler);
#if 0
	apic_write(0x320, 1<<16);
	register_handler(200, timer_wrapper, 0xE);
	apic_write(0x380, 50000);
	apic_write(0x3e0, 3); //Divider 16
	apic_write(0x320, 200);
	printk("XXX%d\n",apic_read(0x390));
	//Timer vector set to 200, TMM = oneshot, for calibration
	start_pit = i8254_read();
	printk("spit %d\n", start_pit);
	calibrating = 1;
#endif
	enable_interrupts();
}

#if 0
void timer_handler(void) {
	static uint32_t count = 0;
	if (calibrating) {
		disable_interrupts();
		uint16_t now_pit = i8254_read();
		printk("%d\n", now_pit);
		uint16_t diff = start_pit-now_pit;
		//PIT frequency is 1193182HZ
		uint32_t second = 59659100000l/(uint64_t)diff;
		printk("APIC Timer is %d*16HZ, %d", second, diff);
		apic_write(0x380, second);
		apic_write(0x320, 200 | (1<<17));
		enable_interrupts();
		calibrating = 0;
		return;
	}
	count++;

	//Get some space at the end of kernel
	char *str = &kernend;
	int len = itoa(count, 10, str, 0, 0);
	vga_puts_at(str, 24, 80-len, 7);
}
#endif
