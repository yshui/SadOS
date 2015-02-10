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
int main() {
	int a;
	char c;
	char b[100];
	printf("char\n");
	scanf("%c", &c);
	printf("num\n");
	scanf("%d", &a);
	printf("%c %d\n", c, a);
	printf("hex\n");
	scanf("%x", &a);
	printf("%x %d\n", a);
	printf("string\n");
	scanf("%s", b);
	printf("%s\n", b);
	printf("num string\n");
	scanf("%d%s", &a, b);
	printf("%s\n", b);
}
