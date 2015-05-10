#include <syscall.h>
char X;
void bootstrap(void){
	//The kernel doesn't really load elf properly
	//So let's bootstrap ourself
}
int main() {
	char *vbase = (void *)0x10000;
	char q[30] = "Hello world from user space";
	int i;
	for (i=0; q[i]; i++) {
		vbase[i*2+4] = q[i];
		vbase[i*2+5] = 0x7;
	}
	syscall_1(0, 42);
	//*(char *)0 = 'a';

/*
	//Global variable doesn't work yet
	//Because got is not properly loaded
	vbase[0] = X+'0';
	vbase[1] = 7;
	X='a';
	vbase[2]=X;
	vbase[3]=7;
*/
	for(;;);
}
