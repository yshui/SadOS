#pragma once

static inline _Noreturn void __idle(void) {
	__asm__ volatile ("hlt\n");
	__builtin_unreachable();
}
