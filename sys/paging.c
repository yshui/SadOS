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
#include <string.h>
extern uint64_t physoffset;
static uint64_t pml4e[512] __attribute__((aligned(4096)));
static uint64_t pdpe[512] __attribute__((aligned(4096)));
static uint64_t pde[2][512] __attribute__((aligned(4096)));
void paging_init(void) {
	//Copy the old page table
	int i;
	uint64_t phys_pdpe = ((uint64_t)pdpe)-((uint64_t)&physoffset);
	for (i = 0; i < 512; i++)
		pml4e[i] = phys_pdpe | 7; //U, RW, P

	//Setting up mapping for normal pages
	uint64_t phys_pde = ((uint64_t)pde[0])-((uint64_t)&physoffset);
	for (i = 0; i < 511; i++)
		pdpe[i] =  phys_pde | 7 ; //U, RW, P

	pdpe[511] = (phys_pde+4096) | 7;

	for (i = 0; i < 512; i++)
		pde[0][i] = (i<<21) | (1<<7) | 7;

	//Set up mapping for APIC
	//APIC base is 0xfee00000
	for (i = 0; i < 512; i++)
		pde[1][i] =  ((uint64_t)0x7f7)<<21 | 7 | (1<<7);
	//Load cr3
	uint64_t cr3 = ((uint64_t)pml4e)-(uint64_t)&physoffset;
	__asm__ volatile ("movq %0, %%cr3" : : "r"(cr3));
}
