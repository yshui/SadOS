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
#include <sys/printk.h>
#include <sys/mm.h>
#define R (25)
#define C (80)
#define VBASE (0xb8000)
#define VBUF_SIZE sizeof(buffer)
static uint16_t video_port = 0;
static uint8_t pos_x, pos_y;
static uint8_t *voff = NULL;
static uint8_t *vbase = NULL;
static uint8_t buffer[4096];
static uint16_t offset;
static int vga_puts(const char *);
void vga_text_init(void) {
	uint8_t *video_info_base = (void *)KERN_VMBASE;
	video_port = *(uint16_t *)(video_info_base+0x463);
	printk("%x\n", video_port);
	map_range(VBASE, VBASE+KERN_VMBASE, (R*C*2+0xfff)&~0xfff, 0, PTE_W);
	vbase = (uint8_t *)(VBASE+KERN_VMBASE);
	voff = vbase;
	uint8_t *v = vbase;
	while(v-vbase <= R*C*2) {
		*(v++) = 0;
		*(v++) = 0x07;
	}
//	print_handler = vga_puts;
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
	uint8_t *v = vbase;
	while(start != offset) {
		*(v++) = buffer[start];
		start = (start+1)%VBUF_SIZE;
	}
	voff = v;
	while(v-vbase <= R*C*2) {
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

__attribute__((unused)) static int vga_puts(const char *str) {
	int c = 0;
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
		c++;
	}
	set_cursor();
	return c;
}

void vga_puts_at(const char *str, uint8_t x, uint8_t y, uint8_t color) {
	//Put a string at give position
	//The string is not put into buffer, so will lost when scroll
	uint8_t *v = vbase+(x*C+y)*2;
	while(*str) {
		*(v++) = *str;
		*(v++) = color;
		if (v >= vbase+R*C*2)
			//Don't go over the end
			break;
		str++;
	}
}
