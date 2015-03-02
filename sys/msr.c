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
