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
#include <sys/portio.h>
#include <stdio.h>
#define R (25)
#define C (80)
#define VBASE ((uint8_t *)0xb8000)
#define VBUF_SIZE sizeof(buffer)
static uint16_t video_port = 0;
static uint8_t pos_x, pos_y;
static uint8_t *voff = VBASE;
static uint8_t buffer[4096];
static uint16_t offset;
void vga_text_init(void) {
	video_port = *(uint16_t *)(0x463);
	uint8_t *v = VBASE;
	while(v-VBASE <= R*C*2) {
		*(v++) = 0;
		*(v++) = 0x07;
	}
}

void set_cursor(void) {
	uint16_t pos = (pos_x*C)+pos_y;
	outb(video_port, 0x0f);
	outb(video_port+1, pos&0xff);
	outb(video_port, 0x0e);
	outb(video_port+1, (pos>>8)&0xff);
}

void blit(void) {
	int start = offset-(pos_y+(pos_x-1)*C)*2;
	if (start < 0)
		start += VBUF_SIZE;
	uint8_t *v = VBASE;
	while(start != offset) {
		*(v++) = buffer[start];
		start = (start+1)%VBUF_SIZE;
	}
	voff = v;
	while(v-VBASE <= R*C*2) {
		*(v++) = 0;
		*(v++) = 0x07;
	}
}

void line_feed(void) {
	while(pos_y < C) {
		//Clear rest of the line
		buffer[offset] = 0;
		offset = (offset+1)%VBUF_SIZE;
		buffer[offset] = 0x07;
		offset = (offset+1)%VBUF_SIZE;
		*(voff++) = 0;
		*(voff++) = 0x07;
		pos_y++;
	}
	pos_x++;
	pos_y = 0;
	if (pos_x >= R) {
		pos_x = R-1;
		blit();
	}
	set_cursor();
}

void puts(const char *str) {
	while(*str) {
		switch(*str) {
		case '\n':
			line_feed();
			break;
		default:
			buffer[offset] = *str;
			offset = (offset+1)%VBUF_SIZE;
			buffer[offset] = 0x07;
			offset = (offset+1)%VBUF_SIZE;
			*(voff++) = *str;
			*(voff++) = 0x07;
			pos_y++;
			if (pos_y >= C)
				line_feed();
			break;
		}
		str++;
	}
	set_cursor();
}

void puts_at(const char *str, uint8_t x, uint8_t y, uint8_t color) {
	//Put a string at give position
	//The string is not put into buffer, so will lost when scroll
	uint8_t *v = VBASE+(x*C+y)*2;
	while(*str) {
		*(v++) = *str;
		*(v++) = color;
		if (v >= VBASE+R*C*2)
			//Don't go over the end
			break;
		str++;
	}
}
