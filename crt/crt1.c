#include <stdlib.h>

int main(int argc, char* argv[], char* envp[]);

void _start(void) {
	int argc = 1;
	char* argv[0];
	char* envp[0];
	int res;
	res = main(argc, argv, envp);
	exit(res);
}
