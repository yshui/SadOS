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

#define APIC_ID    0x20
#define APIC_VER   0x30
#define APIC_TPR   0x80
#define APIC_ISR   0x100
#define APIC_TIMER 0x320

void apic_init(void);
uint32_t apic_read(uint16_t);
void apic_write(uint16_t, uint32_t);
