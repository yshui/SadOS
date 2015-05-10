/* Copyright (C) 2015, Yuxuan Shui <yshuiv7@gmail.com> */

/*
 * Everyone is allowed to copy and/or redistribute verbatim
 * copies of this code as long as the following conditions
 * are met:
 *
 * 1. The copyright infomation and license is unchanged.
 *
 * 2. This piece of code is not submitted as or as part of
 *    an assignment to the CSE506 course at Stony Brook
 *    University, unless the submitter is the copyright
 *    holder.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void (*_init)() __attribute__((weak)) = NULL;
void (*_fini)() __attribute__((weak)) = NULL;

//no frame pointer to ruin our perfect stack
__attribute__((optimize("omit-frame-pointer")))
_Noreturn void _start() {
__asm__ ("xor %rbp,%rbp\n\t" //Clear rbp
	"mov %rdx,%r9\n\t" //6th = rdx?
	"pop %rsi\n\t" //Second arg = argc
	"mov %rsp,%rdx\n\t" //Thrid arg = stack
	"andq $-16,%rsp\n\t" //Aligh stack pointer
	"mov $_fini,%r8\n\t" //5th = _fini
	"mov $_init,%rcx\n\t" //4th = _init
	"mov $main,%rdi\n\t" //First arg = main
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
