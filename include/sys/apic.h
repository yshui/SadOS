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
