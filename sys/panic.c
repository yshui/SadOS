#include <sys/panic.h>
#include <sys/cpu.h>
#include <sys/interrupt.h>
#include <sys/drivers/serio.h>
_Noreturn void panic(const char *str) {
	disable_interrupts();
	serial_puts("Panic: ");
	serial_puts(str);
	__idle();
	__builtin_unreachable();
}
