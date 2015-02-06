#pragma once
#include <stdio.h>
static inline char getchar(void){
	char t;
	int ret = scanf("%c", &t);
	if (ret < 0)
		return -1;
	return t;
}
