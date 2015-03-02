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
#include <sys/defs.h>
#include <sys/msr.h>
#include <stdio.h>

//All pml4e is map to the same pdpe
//pdpe[511] is map to the apic pde
//all apic pde is map to apic register
//so bit 38~30 need to be 1
static volatile uint32_t *apic_base = (void *)0x007fc0000000;

uint32_t apic_read(uint16_t addr) {
	return apic_base[addr/4];
}

void apic_write(uint16_t addr, uint32_t val) {
	apic_base[addr/4] = val;
}

void apic_eoi(void) {
	apic_base[0xb0/4] = 0;
}

void apic_init(void) {
	uint64_t apicr = rdmsr(MSR_APIC_BASE);
	if (!(apicr&(1<<11))) {
		apicr |= (1<<11);
		wrmsr(MSR_APIC_BASE, apicr);
	}
}
