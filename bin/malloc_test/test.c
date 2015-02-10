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

#include <stdlib.h>
#include <stdio.h>
void *sbrk(size_t);
int main(){
	printf("%x\n", sbrk(0));
	char *a1 = malloc(10);
	char *a2 = malloc(1);
	char *a3 = malloc(4);
	free(a1);
	free(a2);
	free(a3);
	printf("%x\n", sbrk(0));
}
