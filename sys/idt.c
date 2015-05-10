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
#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/interrupt.h>
#include <sys/printk.h>
#include <bitops.h>
#include <string.h>
extern int interrupt_disabled;
struct idtr_t {
	uint16_t size;
	uint64_t addr;
}__attribute__((packed));
static inline void load_idt(struct idtr_t *idtr) {
	__asm__ volatile ("lidt (%0)" : : "r"(idtr));
}

uint64_t idt[512] = {
	0,
};

struct igate {
	uint16_t loaddr:16;
	uint16_t cs:16;
	uint8_t ist:3;
	uint8_t reserved0:5;
	uint8_t type:4;
	uint8_t zero:1;
	uint8_t dpl:2;
	uint8_t present:1;
	uint64_t hiaddr:48;
	uint32_t reserved1;
}__attribute__((packed));

static struct idtr_t idtr = {
	sizeof(idt),
	(uint64_t)idt,
};
extern uint64_t int_entry[];
void idt_init(void) {
	int i;
	for (i = 32; i < 256; i++)
		//Register IRQs
		register_handler(i, int_entry+(i-32)*2, 0xE, 0);
	load_idt(&idtr);

	__asm__ volatile ("cli");
	uint64_t rflags;
	__asm__ volatile ("\tpushfq\n"
			  "\tpopq %0\n" : "=r"(rflags) : : );
	printk("%p\n", rflags);
	if (GET_BIT(rflags, 9))
		interrupt_disabled = 0;
	else
		interrupt_disabled = 1;
	printk("Start IF: %d\n", interrupt_disabled);
}

void register_handler(uint16_t index, void *addr, uint8_t type, int ist) {
	if (type != 0xE && type != 0xF) {
		printk("Wrong interrupt type %x\n", type);
		return;
	}
	struct igate ig = {0};
	ig.loaddr = ((uint64_t)addr)&(0xffff);
	ig.cs = KERN_CS*8;
	ig.ist = ist;
	ig.reserved0 = 0;
	ig.type = type;
	ig.zero = 0;
	ig.dpl = 0;
	ig.present = 1;
	ig.hiaddr = ((uint64_t)addr)>>16;
	ig.reserved1 = 0;
	memcpy(idt+(index*2), &ig, sizeof(uint64_t)*2);
}
