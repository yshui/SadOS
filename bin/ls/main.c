#include <stdlib.h>
#include <stdio.h>
int main(int argc, const char **argv){
	if (argc < 2) {
		printf("Usage: %s <path>\n", argv[0]);
		return 1;
	}
	void *p = opendir(argv[1]);
	struct dirent *de;
	do {
		de = readdir(p);
		if (de)
			printf("%s\n", de->d_name);
	}while(de);
}
