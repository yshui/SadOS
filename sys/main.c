#include <sys/sbunix.h>
#include <sys/interrupt.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/tarfs.h>
#include <sys/portio.h>
#include <sys/cpu.h>
#include <sys/mm.h>
#include <sys/msr.h>
#include <sys/apic.h>
#include <sys/paging.h>
#include <sys/i8259.h>
#include <sys/drivers/vga_text.h>
#include <sys/drivers/kbd.h>
extern void timer_init(void);
_Noreturn void start(uint32_t* modulep, void* physbase, void* physfree) {
	struct smap_t {
		uint64_t base, length;
		uint32_t type;
	}__attribute__((packed)) *smap;
	while(modulep[0] != 0x9001)
		modulep += modulep[1]+2;
	int count = 0;
	for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
		if (smap->type == 1 /* memory */ && smap->length != 0) {
			printf("Available Physical Memory [%x-%x]\n", smap->base, smap->base + smap->length);
			count ++;
		}
	}
	printf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
	timer_init();
	kbd_init();

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
	//__asm__ volatile ("cli");
	reload_gdt();
	setup_tss();
	paging_init();
	vga_text_init();
	i8259_init();
	apic_init();
	idt_init();
	start(
		(uint32_t*)((char*)(uint64_t)loader_stack[1] + (uint64_t)&kernmem - (uint64_t)&physbase),
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
