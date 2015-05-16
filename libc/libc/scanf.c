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
#if 0
static inline long
atoi(const char *str, int base) {
	if (base > 36)
		return 0;
	long tres = 0;
	while(*str) {
		if (*str < '0')
			return tres;
		char tmp = *str;
		if (tmp >= 'A' && tmp <= 'Z')
			tmp -= 32;

		if (base <= 10) {
			if (*str >= base+'0')
				return tres;
		} else {
			if (tmp >= base-10+'a')
				return tres;
			if (tmp > '9' && tmp < 'a')
				return tres;
		}

		if (tmp <= '9')
			tres = tres*10+tmp-'9';
		else
			tres = tres*10+tmp-'a'+10;
		str++;
	}
	return tres;
}
#endif
static inline int isspace(char c){
	return c == ' ' || c == '\n' || c == '\t';
}

#define errq(ret) if (ret <= 0) { \
	if (ret < 0) \
		errno = -ret; \
	return 0; \
}

int scanf(const char *fmt, ...) {
	va_list ap;
	static char readahead = 0;
	int read_count = 0;
	va_start(ap, fmt);
	if (!readahead) {
		int ret = my_read(1, &readahead, 1);
		errq(ret);
	}

	while(*fmt){
		if (isspace(*fmt)) {
			while (isspace(readahead)) {
				int ret = my_read(1, &readahead, 1);
				errq(ret);
			}
		} else if (*fmt == '%') {
			int ret, res, *num, base;
			char *chr;
			char *str;
			fmt++;
			switch(*fmt) {
			case '%':
				if (readahead != '%')
					return read_count;
				ret = my_read(1, &readahead, 1);
				errq(ret);
				break;
			case 's':case 'd':case 'x':
				while (isspace(readahead)) {
					ret = my_read(1, &readahead, 1);
					errq(ret);
				}
				if (*fmt == 's') {
					str = va_arg(ap, char *);
					read_count++;
					while(!isspace(readahead)) {
						*(str++) = readahead;
						ret = my_read(1, &readahead, 1);
						if (ret <= 0) {
							*str = '\0';
							if (ret < 0)
								errno = -ret;
							return read_count;
						}
					}
					*str = '\0';
				} else {
					num = va_arg(ap, int *);
					*num = res = 0;
					read_count++;
					if (*fmt == 'x') {
						base = 16;
						if (readahead == '0') {
							ret = my_read(1, &readahead, 1);
							errq(ret);
							if (readahead == 'x') {
								ret = my_read(1, &readahead, 1);
								errq(ret);
							}
						}
					} else
						base = 10;
					while(1) {
						if (base == 10) {
							if (readahead > '9' || readahead < '0')
								break;
						} else {
							if (readahead >= 'A' && readahead <= 'Z')
								readahead += 32;
							if ((readahead < '0' || readahead > '9') &&
							    (readahead < 'a' || readahead > 'f'))
							    break;
						}
						res *= base;
						if (readahead >= 'a')
							res += readahead-'a'+10;
						else
							res += readahead-'0';
						ret = my_read(1, &readahead, 1);
						if (ret <= 0) {
							*num = res;
							if (ret < 0)
								errno = -ret;
							return read_count;
						}
					}
					*num = res;
				}
				break;
			case 'c':
				chr = va_arg(ap, char *);
				*chr = readahead;
				read_count++;
				ret = my_read(1, &readahead, 1);
				errq(ret);
				break;
			default:
				return read_count;
			}
		} else {
			if (readahead != *fmt)
				return read_count;
			int ret = my_read(1, &readahead, 1);
			errq(ret);
		}
		fmt++;
	}
	return read_count;
}
