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
#include <sys/sbunix.h>
#include <sys/interrupt.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/tarfs.h>
#include <sys/portio.h>
#include <sys/cpu.h>
#include <sys/mm.h>
#include <sys/kernaddr.h>
#include <sys/msr.h>
#include <sys/apic.h>
#include <sys/paging.h>
#include <sys/i8259.h>
#include <sys/drivers/vga_text.h>
#include <sys/drivers/kbd.h>
#include <sys/drivers/serio.h>
#include <string.h>
extern void timer_init(void);
struct smap_t smap_buf[20];
int ptsetup;
_Noreturn void start(uint32_t* modulep, void* physbase, void* physfree) {
	struct smap_t *smap;
	int i, smap_len;

	reload_gdt();
	setup_tss();
	serial_init();
	printf("Serial test\n");
	i8259_init();
	apic_init();
	idt_init();

	while(modulep[0] != 0x9001)
		modulep += modulep[1]+2;
	smap = (struct smap_t *)(modulep+2);
	smap_len = modulep[1]/sizeof(struct smap_t);
	memcpy(smap_buf, smap, smap_len*sizeof(struct smap_t));
	memory_init(smap_buf, smap_len);
	//get_page is usable from this point onwards.

	smap = smap_buf;
	for(i = 0; i < smap_len; i++)
		printf("Physical Memory [%x-%x] %s\n", smap[i].base,
		       smap[i].base+smap[i].length,
		       smap[i].type == 1 ? "Available" : "Reserved");
	printf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
	vga_text_init();
	timer_init();
	kbd_init();

	uint64_t p1, p2;
	cpuid(0x80000001, &p1, &p2);
	printf("1Gb page: %d\n", (p2>>26)&1);

	// kernel starts here
	while(1)
		//printf("%d\n", apic_read(0x390));
		__idle();

}

#define INITIAL_STACK_SIZE 4096
char stack[INITIAL_STACK_SIZE];
uint32_t* loader_stack;

_Noreturn void real_boot(void)
{
	start(
		(uint32_t*)((char*)(uint64_t)loader_stack[1] + (uint64_t)&kernbase - (uint64_t)&physbase),
		&physbase,
		(void*)(uint64_t)loader_stack[4]
	);
	char *s, *v;
	s = "!!!!! start() returned !!!!!";
	for(v = (char*)0xb8000; *s; ++s, v += 2){
		*v = *s;
		*(v+1) = 0x07;
	}

	while(1);
}
