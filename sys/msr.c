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
uint64_t rdmsr(uint32_t msr) {
	uint32_t D, A;
	__asm__ volatile (
		"rdmsr\n\t"
		: "=d"(D), "=a"(A)
		: "c"(msr)
	);
	return (((uint64_t)D)<<32)+(uint64_t)A;
}
void wrmsr(uint32_t msr, uint64_t val) {
	uint32_t D = val>>32, A = val&0xffffffff;
	__asm__ volatile (
		"wrmsr\n\t"
		: : "d"(D), "a"(A), "c"(msr)
	);
}
void cpuid(uint32_t in, uint64_t *out1, uint64_t *out2) {
	uint32_t a, b, c, d;
	__asm__ volatile (
		"cpuid\n\t"
		: "=a"(a), "=b"(b), "=c"(c), "=d"(d)
		: "a"(in)
	);
	*out1 = (((uint64_t)a)<<32)+b;
	*out2 = (((uint64_t)c)<<32)+d;
}
