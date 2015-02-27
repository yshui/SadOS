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
#include <stdarg.h>
#include <string.h>
#include <sys/mm.h>
#include <sys/drivers/vga_text.h>

static inline int itoa(long a, int base, char *str, int width, int sign) {
	int n = 0;
	unsigned long tmp;
	if (sign)
		tmp = a > 0 ? a : -a;
	else
		tmp = a;
	while(tmp) {
		tmp /= base;
		n++;
	}
	if (n == 0) {
		if (!width)
			width = 1;
		int t = width;
		if (str)
			while(t--)
				*(str++) = '0';
		return width;
	}

	int ret = n;
	if (ret < width)
		ret = width;
	if (a<0 && sign)
		ret++;
	if (!str)
		return ret;

	if (a<0 && sign) {
		*str = '-';
		str++;
	}

	while((width--) > n)
		*(str++) = '0';

	if (sign)
		tmp = a > 0 ? a : -a;
	else
		tmp = a;

	while(tmp) {
		str[--n] = "0123456789abcdef"[tmp%base];
		tmp /= base;
	}
	return ret;
}

void printf(const char *format, ...) {
	va_list val;
	int num;
	size_t ptr;
	char *str;
	char chr;

	//We don't have memory allocation yet
	//So we just use as many memory as we like from kernend
	char *pos = &kernend;
	va_start(val, format);
	while(*format) {
		if (*format == '%') {
			format++;
			switch(*format) {
			case 'd':
				num = va_arg(val, int);
				pos += itoa(num, 10, pos, 0, 1);
				break;
			case 'x':
				num = va_arg(val, int);
				pos += itoa(num, 16, pos, 0, 0);
				break;
			case 'p':
				ptr = va_arg(val, size_t);
				*(pos++) = '0';
				*(pos++) = 'x';
				pos += itoa(ptr, 16, pos, 8, 0);
				break;
			case 's':
				str = va_arg(val, char *);
				if (!str) {
					strcpy(pos, "(null)");
					pos += 6;
				}
				else {
					strcpy(pos, str);
					pos += strlen(str);
				}
				break;
			case 'c':
				chr = va_arg(val, int);
				*(pos++) = chr;
				break;
			case '%':
				*(pos++) = '%';
				break;
			}

		} else
			*(pos++) = *format;
		format++;
	}
	*pos = 0;
	puts(&kernend);
}
