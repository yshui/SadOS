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
#include <sys/gdt.h>
#include <sys/msr.h>

/* adapted from Chris Stones, shovelos */

#define MAX_GDT 32

#define GDT_CS        (0x00180000000000)  /*** code segment descriptor ***/
#define GDT_DS        (0x00100000000000)  /*** data segment descriptor ***/

#define C             (0x00040000000000)  /*** conforming ***/
#define DPL0          (0x00000000000000)  /*** descriptor privilege level 0 ***/
#define DPL1          (0x00200000000000)  /*** descriptor privilege level 1 ***/
#define DPL2          (0x00400000000000)  /*** descriptor privilege level 2 ***/
#define DPL3          (0x00600000000000)  /*** descriptor privilege level 3 ***/
#define P             (0x00800000000000)  /*** present ***/
#define L             (0x20000000000000)  /*** long mode ***/
#define D             (0x40000000000000)  /*** default op size ***/
#define W             (0x00020000000000)  /*** writable data segment ***/

struct sys_segment_descriptor {
	uint64_t sd_lolimit:16;/* segment extent (lsb) */
	uint64_t sd_lobase:24; /* segment base address (lsb) */
	uint64_t sd_type:5;    /* segment type */
	uint64_t sd_dpl:2;     /* segment descriptor priority level */
	uint64_t sd_p:1;       /* segment descriptor present */
	uint64_t sd_hilimit:4; /* segment extent (msb) */
	uint64_t sd_xx1:3;     /* avl, long and def32 (not used) */
	uint64_t sd_gran:1;    /* limit granularity (byte/page) */
	uint64_t sd_hibase:40; /* segment base address (msb) */
	uint64_t sd_xx2:8;     /* reserved */
	uint64_t sd_zero:5;    /* must be zero */
	uint64_t sd_xx3:19;    /* reserved */
}__attribute__((packed));

struct tss_t {
	uint32_t reserved0;
	uint64_t rsp[3];
	uint64_t ist[8]; //ist[0] must be 0
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iobase;
}__attribute__((packed));

struct tss_t tss;

uint64_t gdt[MAX_GDT] = {
	0,                      /* NULL descriptor */
	GDT_CS | P | DPL0 | L,  /* kernel code segment descriptor */
	GDT_DS | P | W | DPL0,  /* kernel data segment descriptor */
	0,                      /* padding */
	GDT_DS | P | W | DPL3,  /* user data segment descriptor */
	GDT_CS | P | DPL3 | L,  /* user code segment descriptor */
	0, 0,                   /* TSS */
};

struct gdtr_t {
	uint16_t size;
	uint64_t addr;
}__attribute__((packed));

static struct gdtr_t gdtr = {
	sizeof(gdt),
	(uint64_t)gdt,
};

void _x86_64_asm_lgdt(struct gdtr_t* gdtr, uint64_t cs_idx, uint64_t ds_idx);

extern void syscall_entry(void);
void reload_gdt() {
	_x86_64_asm_lgdt(&gdtr, KERN_CS*8, KERN_DS*8);

	//Set STAR, LSTAR
	uint64_t star = (0x18ull<<48)|((KERN_CS*8ull)<<32); //0x1b is 0x18|3, USER_CS-0x10|RPL=3
	wrmsr(0xc0000081, star);

	uint64_t lstar = (uint64_t)syscall_entry;
	wrmsr(0xc0000082, lstar);

	uint64_t sfmask = 0xffffffff;
	wrmsr(0xc0000084, sfmask);
}

static inline void load_tss(uint16_t idx) {
	__asm__ volatile ("ltr %0" : : "r"(idx));
}

#define STACK_SIZE 4096
char stack[STACK_SIZE];
char estack[STACK_SIZE];

void setup_tss() {
	struct sys_segment_descriptor* sd = (struct sys_segment_descriptor*)&gdt[6]; // 7th&8th entry in GDT
	sd->sd_lolimit = sizeof(struct tss_t);
	sd->sd_lobase = ((uint64_t)&tss)&0xffffff;
	sd->sd_type = 9; // 386 TSS
	sd->sd_dpl = 0;
	sd->sd_p = 1;
	sd->sd_hilimit = 0;
	sd->sd_gran = 0;
	sd->sd_hibase = ((uint64_t)&tss) >> 24;

	//Put a rsp in tss.rsp as well, just in case
	tss.rsp[0] = (uint64_t)(stack+STACK_SIZE); //Interrupt stack
	tss.ist[0] = 0;
	tss.ist[1] = (uint64_t)(estack+STACK_SIZE); //Exception stack
	load_tss(48);
}
