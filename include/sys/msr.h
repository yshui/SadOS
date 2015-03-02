#pragma once
#include <sys/defs.h>

#define MSR_APIC_BASE (0x0000001b)

uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t val);
void cpuid(uint32_t, uint64_t *, uint64_t *);
