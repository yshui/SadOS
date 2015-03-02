#pragma once

static inline void __idle(void) {
	__asm__ volatile ("hlt\n");
}
