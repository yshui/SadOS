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
#include <ipc.h>
#include <uapi/mem.h>
#include <uapi/portio.h>
#include <uapi/ioreq.h>
#include <stdlib.h>
#include <sendpage.h>
#include <bitops.h>
#include <errno.h>
#define R (25)
#define C (80)
#define VBUF_SIZE sizeof(buffer)
const char q[30] = "Hello world from user space";
static uint16_t video_port = 0;
static uint8_t pos_x, pos_y;
static uint8_t *voff = NULL;
static uint8_t *vbase = NULL;
static uint8_t buffer[4096];
static uint16_t offset;
static int portio_fd;
char *line_buffer;
char *readable_lines[1024];
int rl_off;
int lbend, rlh, rlt;
static void outb(uint16_t port, uint8_t byte) {
	struct portio_req preq;
	int cookie;
	preq.data = byte;
	preq.type = 1;
	preq.len = 1;
	preq.port = port;
	cookie = request(portio_fd, sizeof(preq), &preq);
	close(cookie);
}
static uint8_t inb(uint16_t port) {
	struct portio_req preq;
	int cookie;
	preq.type = 2;
	preq.port = port;
	preq.len = 1;
	cookie = request(portio_fd, sizeof(preq), &preq);

	struct response res;
	get_response(cookie, &res);
	return (uint8_t)(uint64_t)res.buf;
}

static unsigned char key_map[] = {
	  0,   27,  '1',  '2',  '3',  '4',  '5',  '6',
	'7',  '8',  '9',  '0',  '-',  '=',  127,    9,
	'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
	'o',  'p',  '[',  ']',   13,    0,  'a',  's',
	'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
	'\'', '`',    0, '\\',  'z',  'x',  'c',  'v',
	'b',  'n',  'm',  ',',  '.',  '/',    0,  '*',
	  0,   32,    0,    0,    0,    0,    0,    0,
	  0,    0,    0,    0,    0,    0,    0,    0,
	  0,    0,  '-',    0,    0,    0,  '+',    0,
	  0,    0,    0,    0,    0,    0,  '<',    0,
	  0,    0,    0,    0,    0,    0,    0,    0,
	  0 };

static unsigned char shift_map[] = {
	  0,   27,  '!',  '@',  '#',  '$',  '%',  '^',
	'&',  '*',  '(',  ')',  '_',  '+',  127,    9,
	'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
	'O',  'P',  '{',  '}',   13,    0,  'A',  'S',
	'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
	'"',  '~',  '0',  '|',  'Z',  'X',  'C',  'V',
	'B',  'N',  'M',  '<',  '>',  '?',    0,  '*',
	  0,   32,    0,    0,    0,    0,    0,    0,
	  0,    0,    0,    0,    0,    0,    0,    0,
	  0,    0,  '-',    0,    0,    0,  '+',    0,
	  0,    0,    0,    0,    0,    0,  '>',    0,
	  0,    0,    0,    0,    0,    0,    0,    0,
	  0 };

static inline void kbd_wait_output(void) {
	uint8_t x;
	do {
		x = inb(0x64);
	} while(!(x&1));
}

static inline void kbd_wait_input(void) {
	uint8_t x;
	do {
		x = inb(0x64);
	} while(!(x&2));
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

int vga_puts(const char *str) {
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
enum {
	IDLE,
	E0_WAIT,
	E1_WAIT1,
	E1_WAIT2,
}state;
#define LCTRL_DOWN 1
#define RCTRL_DOWN 2
#define LSHIFT_DOWN 4
#define RSHIFT_DOWN 8
#define CAPS_LOCK 16
static int state2 = 0;
static inline void rctrl(int up) {
	if (up)
		state2 &= ~RCTRL_DOWN;
	else
		state2 |= RCTRL_DOWN;
};

static inline void lctrl(int up) {
	if (up)
		state2 &= ~LCTRL_DOWN;
	else
		state2 |= LCTRL_DOWN;
};
static inline void rshift(int up) {
	if (up)
		state2 &= ~RSHIFT_DOWN;
	else
		state2 |= RSHIFT_DOWN;
};

static inline void lshift(int up) {
	if (up)
		state2 &= ~LSHIFT_DOWN;
	else
		state2 |= LSHIFT_DOWN;
};
static inline void do_key(int symbol, int up) {
	char c[2];
	c[1] = 0;
	if (up)
		return;
	c[0] = key_map[symbol];
	if ((state2&12))
		c[0] = shift_map[symbol];
	if (!c[0])
		//Not printable
		return;
	if ((c[0]>='a' && c[0]<='z') ||
	    (c[0]>='A' && c[0]<='Z'))
		if (state2 & CAPS_LOCK)
			c[0] ^= 32;
	if (state2&3)
		return;
	if (lbend >= 1023)
		return;
	line_buffer[lbend++] = c[0];
	line_buffer[lbend] = 0;
	vga_puts(c);
}

static void do_control_key(int symbol, int up) {
	if (up)
		return;
	if (symbol >= 0x3B && symbol < 0x44) {
		//char c[2] = {(symbol-0x3a)+'0', 0};
		return;
	} else {
		switch(symbol) {
		case 0x0e :
			//vga_puts_at("^H", 23, 78, KBD_DISPLAY_COLOR);
			break;
		case 0x1c :
			if (((rlt+1)&1023) == rlh)
				return;
			if (lbend < 1024)
				line_buffer[lbend++] = '\n';
			readable_lines[rlt] = line_buffer;
			rlt = (rlt+1)&1023;
			line_buffer = malloc(1024);
			lbend = 0;
			line_feed();
			break;
		case 0x0f :
			//vga_puts_at("^I", 23, 78, KBD_DISPLAY_COLOR);
			if (lbend < 1024)
				line_buffer[lbend++] = '\t';
			break;
		case 0x01 :
			break;
		case 0x44 :
			break;
		}
	}
}
static inline void caps(int up) {
	if (up)
		return;
	state2 ^= CAPS_LOCK;
}
static inline void state_transition(uint8_t symbol) {
	if (state == E1_WAIT1)
		state = E1_WAIT2;
	else if (state == E1_WAIT2)
		state = IDLE;
	else if (state == E0_WAIT) {
		int up = symbol&0x80;
		symbol = symbol&0x7f;
		switch(symbol) {
		case 0x1d : //RCTRL
			rctrl(up);
			break;
		default:
			//Ignore
			break;
		}
		state = IDLE;
	} else {
		uint8_t up = symbol&0x80;
		if (symbol == 0xe0)
			state = E0_WAIT;
		else if (symbol == 0xe1)
			state = E1_WAIT1;
		else if (symbol >= 0x3b && symbol <= 0x58)
			do_control_key(symbol&0x7f, up);
		else {
			symbol &= 0x7f;
			switch(symbol) {
			case 0x2a :
				lshift(up);
				break;
			case 0x36 :
				rshift(up);
				break;
			case 0x1d :
				lctrl(up);
				break;
			case 0x3a :
				caps(up);
				break;
			case 0x0e :
			case 0x1c :
			case 0x0f :
			case 0x1 :
				do_control_key(symbol, up);
				break;
			default:
				do_key(symbol, up);
			}
		}
	}
}
uint64_t handle_kbd(void) {
	uint8_t x = inb(0x64);
	while(x&1) {
		uint8_t c = inb(0x60);
		x = inb(0x64);
		//printk("!!%x\n", c);
		state_transition(c);
	}
	return 0;
}

void send_result(int fd, int len) {
	char *src = readable_lines[rlh]+rl_off;
	int need_free = 0;
	if (len >= strlen(src)) {
		len = strlen(src);
		rlh = (rlh+1)&1023;
		rl_off = 0;
		need_free = 1;
	} else
		rl_off += len;
	struct io_res *rx = malloc(sizeof(struct io_res)+len+1);
	rx->len = len;
	rx->err = 0;
	strncpy((char *)(rx+1), src, len);
	if (need_free)
		free(src);
	respond(fd, sizeof(struct io_res)+len+1, rx);
	free(rx);
}
int pending_read[1024], pending_read_len[1024];
int ph, pt;
int main() {
	struct response res;
	struct mem_req req;
	req.phys_addr = 0xb8000;
	req.len = 80*25*2;
	req.dest_addr = 0;
	int rd = port_connect(1, sizeof(req), &req);
	get_response(rd, &res);
	voff = vbase = res.buf;

	int bda_fd = port_connect(3, 0, NULL);
	get_response(bda_fd, &res);

	uint8_t *bda = res.buf;
	video_port = *(uint16_t *)(bda+0x463);
	close(bda_fd);

	portio_fd = port_connect(4, 0, NULL);
	int i;
	for (i=0; q[i]; i++) {
		vbase[i*2+4] = q[i];
		vbase[i*2+5] = 0x7;
	}

	long pd = open_port(5);
	struct fd_set fds;
	fds.nfds = 0;
	fd_set_set(&fds, pd);
	wait_on(&fds, NULL, 0);

	struct urequest ureq;
	int handle = pop_request(pd, &ureq);

	int kbd_int_num = 33;
	int kbd_int = port_connect(10, sizeof(int), &kbd_int_num);


	//Check interrupt status
	outb(0x64, 0x20);
	kbd_wait_output();
	uint8_t x = inb(0x60);
	if (!(x&1)) {
		x |= 1;
		outb(0x64, 0x60);
		kbd_wait_input();
		outb(0x60, x);
	}
	line_buffer= malloc(1024);

	int control_pid = 1;
	while(1) {
		fd_zero(&fds);
		fd_set_set(&fds, handle);
		fd_set_set(&fds, kbd_int);
		wait_on(&fds, NULL, 0);
		if (fd_is_set(&fds, kbd_int)) {
			handle_kbd();
		}
		while(rlt != rlh && pt != ph) {
			int rh = pending_read[ph];
			int rlen = pending_read_len[ph];
			ph = (ph+1)&1023;
			send_result(rh, rlen);
		}
		if (fd_is_set(&fds, handle)) {
			int rhandle = pop_request(handle, &ureq);
			struct io_req *x = ureq.buf;
			struct io_res rx;
			if (x->type == IO_READ) {
				if (ureq.pid != control_pid) {
					rx.err = EIO;
					rx.len = 0;
					respond(rhandle, sizeof(rx), &rx);
				} else if (rlt == rlh) {
					pending_read[pt] = rhandle;
					pending_read_len[pt] = x->len;
					pt = (pt+1)&1023;
				} else {
					send_result(rhandle, x->len);
				}
			} else if (x->type == IO_WRITE){
				vga_puts((void *)(x+1));
				rx.err = 0;
				rx.len = strlen((void *)(x+1));
				respond(rhandle, sizeof(rx), &rx);
			} else if (x->type == 3) {
				//Change control process
				control_pid = ureq.pid;
				vga_puts("Request to switch foreground task");
				line_feed();
			}
			uint64_t base = ALIGN((uint64_t)x, 12);
			munmap((void *)base, ureq.len);
		}
	}
}
