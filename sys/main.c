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
#include <sys/port.h>
#include <sys/syscall.h>
#include <sys/sbunix.h>
#include <sys/interrupt.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/tarfs.h>
#include <sys/portio.h>
#include <sys/cpu.h>
#include <sys/mm.h>
#include <sys/kernaddr.h>
#include <bitops.h>
#include <sys/msr.h>
#include <sys/apic.h>
#include <sys/paging.h>
#include <sys/i8259.h>
#include <sys/drivers/vga_text.h>
#include <sys/drivers/kbd.h>
#include <sys/drivers/serio.h>
#include <sys/printk.h>
#include <sys/sched.h>
#include <sys/list.h>
#include <sys/tar.h>
#include <sys/service.h>
#include <string.h>
#include <sys/tarfs.h>
//#include <sys/ahci.h>
#include <uapi/thread.h>
#include <vfs.h>
char buf[20000] = {0};
uint8_t stack[4096];

extern void timer_init(void);
struct smap_t smap_buf[20];
int ptsetup;
struct task *create_idle_task(void) {
	struct task *t = new_task();
	t->as = NULL;
	t->kstack_base = stack+4096;
	t->state = TASK_RUNNABLE;
	t->pid = 0;
	tasks[0] = t;
	t->priority = -21;
	idle_task = t;
	return t;
}
static inline uint64_t atoi_oct(const char *str) {
	uint64_t ans = 0;
	while (*str) {
		ans = ans*8+*str-'0';
		str++;
	}
	return ans;
}
__noreturn void start_init(void) {
	//Search for /bin/init in tarfs
	struct tar_header *thdr = (void *)&_binary_tarfs_start;
	while((char *)thdr <= &_binary_tarfs_end) {
		char *n = thdr->name;
		if (*n == '/')
			n++;
		if (strcmp(n, "bin/init") == 0)
			break;
		uint64_t size = atoi_oct(thdr->size);
		printk("Name: %s, size: %d\n", thdr->name, size);
		char *tmp = (void *)thdr;
		tmp += 512+((size+511)&~511);
		thdr = (void *)tmp;
	}

	//Setup the init task
	uint64_t init_size = atoi_oct(thdr->size);
	char *init_addr = (char *)(thdr)+512;
	uint64_t user_addr = 0x400000;
	uint64_t entry_point = *(uint64_t *)(init_addr+24);
	struct address_space *user_as = aspace_new(0);
	user_as->as_flags = AS_LAZY;
	struct vm_area *image_vma =
	    insert_vma_to_vaddr(user_as, user_addr, ALIGN_UP(init_size, PAGE_SIZE_BIT), 0);
	struct vm_area *image_vma2 =
	    insert_vma_to_vaddr(user_as, user_addr+0x200000, ALIGN_UP(init_size, PAGE_SIZE_BIT), VMA_RW);
	//Copy /bin/init into user space memory
	while (init_size) {
		char *page = get_page();
		if (init_size >= PAGE_SIZE) {
			memcpy(page, init_addr, PAGE_SIZE);
			init_size -= PAGE_SIZE;
			init_addr += PAGE_SIZE;
		} else {
			memset(page, 0, PAGE_SIZE);
			memcpy(page, init_addr, init_size);
			init_size = 0;
		}
		uint64_t phys_addr = ((uint64_t)page)-KERN_VMBASE;
		printk("Start init: %p\n", user_addr);
		address_space_assign_addr_with_vma(image_vma, phys_addr, user_addr, PF_SHARED);
		address_space_assign_addr_with_vma(image_vma2, phys_addr, user_addr+0x200000, PF_SHARED);
		user_addr += PAGE_SIZE;
	}

	//Map a page for stack
	char *page = get_page();
	memset(page, 0, PAGE_SIZE);
	*(uint64_t *)(page+PAGE_SIZE-32) = 0; //cwd
	*(uint64_t *)(page+PAGE_SIZE-24) = 0; //argc
	*(uint64_t *)(page+PAGE_SIZE-16) = 0; //argv end
	*(uint64_t *)(page+PAGE_SIZE-8) = 0; //envp end
	uint64_t phys_addr = ((uint64_t)page)-KERN_VMBASE;
	insert_vma_to_vaddr(user_as, user_addr, PAGE_SIZE, VMA_RW);
	address_space_assign_addr(user_as, phys_addr, user_addr, PF_SHARED);

	//Hard coded entry point for init
	struct thread_info ti;
	memset(&ti, 0, sizeof(ti));
	ti.rcx = entry_point;
	ti.rsp = user_addr+PAGE_SIZE-32;
	struct task *t = new_process(user_as, &ti);
	t->pid = 1;
	tasks[1] = t;
	list_add(&runq, &t->tasks);

	enable_interrupts();
	schedule();

	//Schedule returns here when there's no runnable task

	while(1) {
		//Simply idle and wait for interrupts
		//Interrupt handler should call schedule() when appropriate
		__idle();
		schedule(); //Should be called in timer
		printk("Hey, this is idle task, nice to see you again\n");
	}
}
extern struct syscall_metadata syscall_meta_table[];
extern struct syscall_metadata syscall_meta_end;
void *syscall_table[512];
void syscalls_init(void) {
	int i = 0;
	printk("%p\n", syscall_meta_table);
	while(syscall_meta_table+i < &syscall_meta_end) {
		if (syscall_meta_table[i].syscall_nr >= 512)
			panic("Syscall number too big");
		int nr = syscall_meta_table[i].syscall_nr;
		syscall_table[nr] = syscall_meta_table[i].ptr;
		printk("%d->%p\n", nr, syscall_table[nr]);
		i++;
	}

}

extern struct service_init services_init_table[];
extern struct service_init services_init_end;
void services_init(void) {
	int i = 0;
	printk("%p~%p\n", services_init_table, &services_init_end);
	while(services_init_table+i < &services_init_end) {
		services_init_table[i].init();
		i++;
	}
}
extern char gp_entry, ud_entry, de_entry;
__noreturn void start(uint32_t* modulep, void* physbase, void* physfree) {
	struct smap_t *smap;
	int i, smap_len;

	reload_gdt();
	setup_tss();
	serial_init();
	printk("Serial test\n");
	i8259_init();
	apic_init();
	syscalls_init();
	idt_init();

	register_handler(0xd, &gp_entry, 0xe, 1);
	register_handler(0x6, &ud_entry, 0xe, 1);
	register_handler(0x0, &de_entry, 0xe, 1);

	while(modulep[0] != 0x9001)
		modulep += modulep[1]+2;
	smap = (struct smap_t *)(modulep+2);
	smap_len = modulep[1]/sizeof(struct smap_t);
	memcpy(smap_buf, smap, smap_len*sizeof(struct smap_t));
	memory_init(smap_buf, smap_len);
	//get_page is usable from this point onwards.

	smap = smap_buf;
	for(i = 0; i < smap_len; i++)
		printk("Physical Memory [%x-%x] %s\n", smap[i].base,
		       smap[i].base+smap[i].length,
		       smap[i].type == 1 ? "Available" : "Reserved");
	printk("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
	services_init();
	vga_text_init();
	task_init();
	port_init();
	timer_init();
	//kbd_init();
	current = create_idle_task();

	for(int i=0;i<=30000000;i++);
	printk("Start!!!");
	start_init();
}

uint32_t* loader_stack;

__noreturn void real_boot(void)
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
