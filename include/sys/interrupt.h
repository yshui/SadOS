#pragma once
#include <sys/defs.h>
typedef void (*handler_t)(int vector);

static inline void disable_interrupts(void) {
	__asm__ volatile ("cli");
}

static inline void enable_interrupts(void) {
	__asm__ volatile ("sti");
}

int int_register_handler(uint64_t, handler_t);
void int_deregister_handler(uint64_t);
int int_register_eoi(uint64_t, handler_t);
