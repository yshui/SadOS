.text
.global boot
boot:
	movq %rsp, loader_stack
	movq $stack, %rax
	lea 0x1000(%rax), %rsp
	call real_boot
	hlt
