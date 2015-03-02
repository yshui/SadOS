#include <stdio.h>
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
	//printf("EOI%d\n",irq);
}

void i8259_eoi(unsigned char irq) {
	_i8259_eoi(irq);
}

void i8259_int_eoi(int v) {
	_i8259_eoi(v-32);
}

void i8259_init(void) {
	int i;
	for (i = 32; i < 48; i++)
		int_register_eoi(i, i8259_int_eoi);
}
