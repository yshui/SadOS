#pragma once
#include <sys/defs.h>
typedef void (*handler_t)(int vector);

void irq_init(void);
int irq_register_handler(uint64_t, handler_t);
void irq_deregister_handler(uint64_t);
