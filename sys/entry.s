# Copyright (C) 2013 Cloudius Systems, Ltd.
# Copyright (C) 2015 Yuxuan Shui
#
# This work is open source software, licensed under the terms of the
# BSD license as described in the BSD file in the license directory.


.macro interrupt_entry name, handler, has_argument
	.global \name
	.type \name, @function
	\name :
	.if \has_argument == 0
	pushq $0
	.endif
	pushq %rdi
	movq 8(%rsp), %rdi #get the argument we pushed to stack
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rsi
	pushq %rbp
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rsp
	call \handler
	popq %rsp
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rbp
	popq %rsi
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax
	popq %rdi
	add $8, %rsp
	iretq
.endm

.align 16
.global int_entry
int_entry:
vector = 32
.rept 256 - 32
    .align 16
    pushq $vector
    jmp int_entry_common
    vector = vector + 1
.endr

interrupt_entry int_entry_common, int_handler, 1
