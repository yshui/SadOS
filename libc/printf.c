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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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

const char *fmt_err = "Invalid printf format string\n";

void tmp_write(uint64_t fd, const void* buf, size_t count)
{
    ;
}
int printf(const char *format, ...) {
	va_list val;
	int num;
	char *str;
	char chr;
	const char*tfmt = format;

	va_start(val, format);

	//first, calculate how much space we need
	size_t len = 0;
	while(*format) {
		if (*format == '%') {
			format++;
			if (!format) {
				tmp_write(1, fmt_err, strlen(fmt_err));
				return 0;
			}
			switch(*format) {
			case 'd':
				num = va_arg(val, int);
				len += itoa(num, 10, NULL, 0, 1);
				break;
			case 'x':
				num = va_arg(val, int);
				len += itoa(num, 16, NULL, 0, 0);
				break;
			case 's':
				str = va_arg(val, char *);
				if (!str)
					len += 6; // for (null)
				else
					len += strlen(str);
				break;
			case 'c':
				chr = va_arg(val, int);
				len++;
				break;
			case '%':
				len++;
				break;
			default:
				tmp_write(1, fmt_err, strlen(fmt_err));
				return 0;
			}

		} else
			len++;
		format++;
	}
	va_end(val);

	char *res = malloc(len);
	char *pos = res;
	va_start(val, format);
	format = tfmt;
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
	tmp_write(1, res, len);
	free(res);

	return len;
}
