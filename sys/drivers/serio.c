#define COM1 0x3f8
#include <sys/portio.h>
#include <sys/defs.h>
#include <sys/printk.h>
static inline int is_transmit_empty(uint16_t port) {
   return inb(port + 5) & 0x20;
}
int serial_puts(const char *str) {
	uint16_t port = COM1;
	int c = 0;
	while(*str) {
		int i;
		while (!is_transmit_empty(port));
		for (i = 0; i < 16; i++) {
			if (!*str)
				break;
			outb(port, *str);
			str++;
			c++;
		}
	}
	return c;
}
void serial_init() {
	uint16_t port = COM1;
	outb(port + 1, 0x00);    // Disable all interrupts
	outb(port + 3, 0x83);    // Enable DLAB (set baud rate divisor)
	outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(port + 1, 0x00);    //                  (hi byte)
	outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set

	print_handler = serial_puts;
}
