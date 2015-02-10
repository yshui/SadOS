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
