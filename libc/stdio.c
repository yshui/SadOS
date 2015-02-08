#include <stdio.h>
#include <stdlib.h>

char getchar(void) {
	char tmp;
	int ret = read(0, &tmp, 1);
	if (ret < 0)
		return -1;
	return tmp;
}
