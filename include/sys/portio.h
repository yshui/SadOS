#pragma once
#include <sys/defs.h>
static inline void outb(uint16_t port, uint8_t d) {
	__asm__ volatile (
		"outb %%al, %%dx"
		: : "d"(port), "a"(d)
	);
}
static inline uint8_t inb(uint16_t port) {
	uint8_t val;
	__asm__ volatile (
		"inb %%dx, %%al"
		: "=a"(val) : "d"(port)
	);
	return val;
}
static inline void io_wait(void) {
    __asm__ volatile ( "outb %%al, $0x80" : : "a"(0) );
}
