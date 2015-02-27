#pragma once

static inline void mask_all_interrupt(void) {
	__asm__ volatile ("cli");
}

static inline void unmask_all_interrupt(void) {
	__asm__ volatile ("sti");
}
