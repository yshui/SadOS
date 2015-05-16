#pragma once

#include <sys/printk.h>
#include <sys/cpu.h>

#define panic(...) \
	do { \
		_printk("Panic! [" __FILE__ ":%d] ", __LINE__ ); \
		_printk(__VA_ARGS__); \
		__asm__ volatile ("cli"); \
		__idle(); \
	}while(0);
