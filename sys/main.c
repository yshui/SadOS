#include <sys/sbunix.h>
#include <sys/gdt.h>
#include <sys/tarfs.h>
#include <sys/portio.h>
#include <sys/cpu.h>
#include <sys/mm.h>
#include <sys/drivers/vga_text.h>

_Noreturn void start(uint32_t* modulep, void* physbase, void* physfree) {
	vga_text_init();
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
	// kernel starts here
	while(1)
		__idle();
}

#define INITIAL_STACK_SIZE 4096
char stack[INITIAL_STACK_SIZE];
uint32_t* loader_stack;
struct tss_t tss;

_Noreturn void real_boot(void)
{
	reload_gdt();
	setup_tss();
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

#if 0
_Noreturn __attribute__((optimize("omit-frame-pointer")))
void boot1(void){
	// note: function changes rsp, local stack variables can't be practically used
	__asm__(
		"movq %%rsp, %0\n\t"
		"movq %1, %%rsp;\n\t"
		"call real_boot\n\t"
		"hlt"
		:"=g"(loader_stack)
		:"r"(stack+INITIAL_STACK_SIZE)
	);
	__builtin_unreachable();
}
#endif
