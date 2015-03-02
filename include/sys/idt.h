#pragma once

void idt_init(void);
void register_handler(uint16_t index, void *addr, uint8_t type);
