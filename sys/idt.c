#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/interrupt.h>
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
}__attribute__((packed));

static struct idtr_t idtr = {
	sizeof(idt),
	(uint64_t)idt,
};

void register_handler(uint8_t index, void *addr) {
	struct igate ig;
	ig.loaddr = ((uint64_t)addr)&(0xffff);
	ig.cs = KERN_CS*8;
	ig.dpl = 0;
	ig.present = 1;
	ig.hiaddr = ((uint64_t)addr)>>16;
	ig.zero = 0;
	ig.reserved0 = 0;
	//Mask interrupt before we change handler table
	mask_all_interrupt();
	idt[index] = *(uint64_t *)&ig;
	idt[index+1] = *(((uint64_t *)&ig)+1);
	unmask_all_interrupt();
}
