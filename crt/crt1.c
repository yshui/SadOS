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
#include <compiler.h>

//no frame pointer to ruin our perfect stack
__attribute__((optimize("omit-frame-pointer")))
__noreturn void _start() {
__asm__ ("xor %rbp,%rbp\n\t" //Clear rbp
	"mov %rdx,%r9\n\t" //6th = rdx?
	"pop %rsi\n\t" //Second arg = argc
	"mov %rsp,%rdx\n\t" //Thrid arg = stack
	"andq $-16,%rsp\n\t" //Aligh stack pointer
	"mov $main,%rdi\n\t" //First arg = main
	"call __real_start\n\t"
	"hlt");
	__builtin_unreachable();
}

extern char __cwd[];
void __real_start(int (*main)(int, char **, char **),
		  int argc, char *rsp) {
	if (*rsp != '\0')
		strcpy(__cwd, rsp);

	//Rebuild argv and envp
	rsp = rsp+strlen(rsp)+1;
	char **argv = ((char **)rsp)-argc-1;
	for (int i = 0; i < argc; i++) {
		argv[i] = rsp;
		rsp += strlen(rsp)+1;
	}
	argv[argc] = NULL;

	char **envp = argv-1;
	*(envp--) = NULL;
	while(*rsp) {
		*(envp--) = rsp;
		rsp += strlen(rsp)+1;
	}
	environ = envp+1;
	exit(main(argc, argv, envp));
}
