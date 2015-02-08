#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static inline int itoa(long a, char *str) {
	int n = 0;
	long tmp = a > 0 ? a : -a;
	while(tmp) {
		tmp /= 10;
		n++;
	}
	if (n == 0) {
		if (str)
			*str = '0';
		return 1;
	}

	int ret = a < 0 ? n+1 : n;
	if (!str)
		return ret;

	if (a < 0) {
		*str = '-';
		str++;
	}
	tmp = a > 0 ? a : -a;
	while(tmp) {
		str[--n] = tmp%10;
		tmp /= 10;
	}
	return ret;
}

const char *fmt_err = "Invalid printf format string\n";

int printf(const char *format, ...) {
	va_list val;
	int num;
	char *str;
	const char*tfmt = format;

	va_start(val, format);

	//first, calculate how much space we need
	size_t len = 0;
	while(*format) {
		if (*format == '%') {
			format++;
			if (!format) {
				write(1, fmt_err, strlen(fmt_err));
				return 0;
			}
			switch(*format) {
			case 'd':
				num = va_arg(val, int);
				len += itoa(num, NULL);
				break;
			case 's':
				str = va_arg(val, char *);
				if (!str)
					len += 6; // for (null)
				else
					len += strlen(str);
				break;
			case '%':
				len++;
				break;
			default:
				write(1, fmt_err, strlen(fmt_err));
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
				pos += itoa(num, pos);
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
			case '%':
				*(pos++) = '%';
				break;
			}

		} else
			*(pos++) = *format;
		format++;
	}
	write(1, res, len);

	return len;
}
