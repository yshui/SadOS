#pragma once
#include <sys/defs.h>
static inline void outb(uint16_t port, uint8_t d) {
	__asm__ (
		"outb %%al, %%dx"
		: : "d"(port), "a"(d)
	);
}
