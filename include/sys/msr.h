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
#pragma once
#include <sys/defs.h>

#define MSR_APIC_BASE (0x0000001b)

uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t val);
void cpuid(uint32_t, uint64_t *, uint64_t *);
