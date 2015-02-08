#include <stdio.h>
#include <stdlib.h>

char getchar(void) {
	char tmp;
	read(0, &tmp, 1);
	return tmp;
}
