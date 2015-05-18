# Copyright (C) 2013 Cloudius Systems, Ltd.
# Copyright (C) 2015 Yuxuan Shui
#
# This work is open source software, licensed under the terms of the
# BSD license as described in the BSD file in the license directory.

tmp_reg: .quad 0
.macro push_all_register, has_argument
	.if \has_argument == 1
	movq %rdi, tmp_reg
	movq (%rsp), %rdi #get the argument we pushed to stack
	addq $8, %rsp
	pushq tmp_reg
	.else
	pushq %rdi
	.endif
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
.endm

.macro pop_all_register, pop_rax
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
	.if \pop_rax == 1
	popq %rax
	.else
	addq $8, %rsp
	.endif
	popq %rdi
.endm

.global kernel_stack
.macro interrupt_entry name, handler, has_argument
	.global \name
	.type \name, @function
	\name :
	push_all_register \has_argument
	call \handler
	#a return value of 1 indicate
	#schedule is needed
	bt $0, %rax
	jnc normal_intret
	movq %rsp, %rdi
	call int_reschedule
	pop_all_register 1
	iretq
.endm

normal_intret:
	pop_all_register 1
	iretq

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

.global syscall_entry, syscall_return
.global syscall_table
.global interrupt_disabled

syscall_entry:
	movq %rsp, (tmp_reg)
	movq kernel_stack, %rsp
	pushq tmp_reg
	push_all_register 0
	movq %r10, %rcx
	movq current, %r10
	movq %rsp, 16(%r10) #set current->ti
	movq $syscall_table, %r10
	movq 0(%r10, %rax, 8), %r10
	testq %r10, %r10
	jz err_syscall_nr
	call *%r10

syscall_return:
	movl interrupt_disabled, %r10d
	testl %r10d, %r10d
	jnz ret_panic
	movq current, %r10
	movq $0, 16(%r10) #current->ti = NULL
	pop_all_register 0
	popq %rsp         #switch back to user stack
	sysretq

.global panic

panic_msg: .ascii "Return from syscall with interrupt disabled\n\0"
err_nr_msg: .ascii "Non existent syscall %d\n\0"

err_syscall_nr:
	movq $err_nr_msg, %rdi
	movq %rax, %rsi
	call _printk
	cli
	hlt
	jmp syscall_return

ret_panic:
	cli
	movq $panic_msg, %rdi
	call _printk
	hlt

interrupt_entry int_entry_common, int_handler, 1
interrupt_entry gp_entry2, gp_handler, 1
interrupt_entry ud_entry2, ud_handler, 1
interrupt_entry de_entry2, de_handler, 1

.global page_fault_entry
page_fault_entry:
	push_all_register 1
	movl $1, interrupt_disabled
	call page_fault_handler
	movl $0, interrupt_disabled
	#a return value of 1 indicate
	#schedule is needed
	bt $0, %rax
	jnc normal_intret
	movq %rsp, %rdi
	call int_reschedule
	pop_all_register 1
	iretq

.global gp_entry
gp_entry:
	pushq %rsp
	jmp gp_entry2
.global ud_entry
ud_entry:
	pushq %rsp
	jmp ud_entry2
.global de_entry
de_entry:
	pushq %rsp
	jmp de_entry2

.global switch_to
.global current
switch_to:
	#switch to 'next' task, passed in via
	#We need to save callee saved registers
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	#first, save rsp
	movq current, %rax
	movq %rsp, 8(%rax)
	#move next to current
	movq %rdi, current
	#change the rsp
	movq 8(%rdi), %rsp
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp
	retq


