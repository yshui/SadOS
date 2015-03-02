#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/interrupt.h>
#include <stdio.h>
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
		register_handler(i*2, int_entry+(i-32)*2, 0xE);
	load_idt(&idtr);
	enable_interrupts();
}

void register_handler(uint16_t index, void *addr, uint8_t type) {
	if (type != 0xE && type != 0xF) {
		printf("Wrong interrupt type %x\n", type);
		return;
	}
	struct igate ig = {0};
	ig.loaddr = ((uint64_t)addr)&(0xffff);
	ig.cs = KERN_CS*8;
	ig.ist = 0;
	ig.reserved0 = 0;
	ig.type = type;
	ig.zero = 0;
	ig.dpl = 0;
	ig.present = 1;
	ig.hiaddr = ((uint64_t)addr)>>16;
	ig.reserved1 = 0;
	idt[index] = *(uint64_t *)&ig;
	idt[index+1] = *(((uint64_t *)&ig)+1);
}
