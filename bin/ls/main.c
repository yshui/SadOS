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
int main(int argc, const char **argv){
	if (argc < 2) {
		printf("Usage: %s <path>\n", argv[0]);
		return 1;
	}
	void *fd = opendir(argv[1]);
	struct dirent *de;
	do {
		de = readdir(fd);
		if (de)
			printf("%s\n", de->d_name);
	}while(de);
}
