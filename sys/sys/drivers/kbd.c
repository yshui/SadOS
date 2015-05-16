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
#include <sys/interrupt.h>
#include <sys/i8259.h>
#include <sys/portio.h>
#include <sys/defs.h>
#include <sys/printk.h>
#include <sys/drivers/kbd.h>
#include <sys/drivers/vga_text.h>

#define KBD_DISPLAY_COLOR 0x70

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
extern int ptsetup;

static inline void rctrl(int up) {
	if (up)
		state2 &= ~RCTRL_DOWN;
	else
		state2 |= RCTRL_DOWN;
	if (!ptsetup)
		ptsetup = 1;
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

static inline void caps(int up) {
	if (up)
		return;
	state2 ^= CAPS_LOCK;
	if (state2 & CAPS_LOCK)
		vga_puts_at("CAPS", 22, 76, 0x4f);
	else
		vga_puts_at("    ", 22, 76, 0x0);
}

static inline void press(void) {
	vga_puts_at("Pressing ", 23, 67, KBD_DISPLAY_COLOR);
}

static inline void release(void) {
	vga_puts_at("Released ", 23, 67, KBD_DISPLAY_COLOR);
}

static inline void do_key(int symbol, int up) {
	char c[2];
	c[1] = 0;
	if (up) {
		release();
		return;
	}
	vga_puts_at("   ", 23, 77, 0x0);
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
		vga_puts_at("^", 23, 78, KBD_DISPLAY_COLOR);
	vga_puts_at(c, 23, 79, KBD_DISPLAY_COLOR);
	press();
}

static void do_control_key(int symbol, int up) {
	if (up) {
		release();
		return;
	}
	vga_puts_at("   ", 23, 77, 0x0);
	if (symbol >= 0x3B && symbol < 0x44) {
		char c[2] = {(symbol-0x3a)+'0', 0};
		vga_puts_at("F", 23, 78, KBD_DISPLAY_COLOR);
		vga_puts_at(c, 23, 79, KBD_DISPLAY_COLOR);
	} else {
		switch(symbol) {
		case 0x0e :
			vga_puts_at("^H", 23, 78, KBD_DISPLAY_COLOR);
			break;
		case 0x1c :
			vga_puts_at("^M", 23, 78, KBD_DISPLAY_COLOR);
			break;
		case 0x0f :
			vga_puts_at("^I", 23, 78, KBD_DISPLAY_COLOR);
			break;
		case 0x01 :
			vga_puts_at("^[", 23, 78, KBD_DISPLAY_COLOR);
			break;
		case 0x44 :
			vga_puts_at("F10", 23, 77, KBD_DISPLAY_COLOR);
		}
	}
	press();
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

uint64_t kbd_handler(int v) {
	uint8_t x = inb(0x64);
	while(x&1) {
		uint8_t c = inb(0x60);
		x = inb(0x64);
		//printk("!!%x\n", c);
		state_transition(c);
	}
	return 0;
}

void kbd_init(void) {
	//Empty the buffer
	disable_interrupts();
	uint8_t kstatus = inb(0x64), x;
	while(kstatus&1)
		x = inb(0x60);
	//Test 8042
	outb(0x64, 0xAA);
	kbd_wait_output();
	x = inb(0x60);
	if (x != 0x55) {
		printk("PS/2 Controller malfunctioning\n");
		return;
	}

	//Test PS/2 port 1
	outb(0x64, 0xAB);
	kbd_wait_output();
	x = inb(0x60);
	if (x) {
		printk("First PS/2 port malfunctioning\n");
		return;
	}

	//Check interrupt status
	outb(0x64, 0x20);
	kbd_wait_output();
	x = inb(0x60);
	if (!(x&1)) {
		printk("Enabling first PS/2 port interrupt");
		x |= 1;
		outb(0x64, 0x60);
		kbd_wait_input();
		outb(0x60, x);
	}

	int_register_handler(33, kbd_handler);
	outb(0x64, 0xAE);

	enable_interrupts();
}
