#pragma once

void vga_text_init(void);
void puts(const char *);
void puts_at(const char *str, uint8_t x, uint8_t y, uint8_t color);
