#include <stdlib.h>
#include <string.h>

void (*_init)() __attribute__((weak)) = NULL;
void (*_fini)() __attribute__((weak)) = NULL;

//force -O1
_Noreturn void _start() {
__asm__ ("xor %rbp,%rbp\n\t"
	"mov %rdx,%r9\n\t"
	"pop %rsi\n\t"
	"mov %rsp,%rdx\n\t"
	"andq $-16,%rsp\n\t"
	"mov $_fini,%r8\n\t"
	"mov $_init,%rcx\n\t"
	"mov $main,%rdi\n\t"
	"call __real_start\n\t"
	"hlt");
	__builtin_unreachable();
}

void __real_start(int (*main)(int, char **, char **),
		  int argc, char **argv) {
	char** envp = argv+argc+1;
	environ = envp;
	exit(main(argc, argv, envp));
}
