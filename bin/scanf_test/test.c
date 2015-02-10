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
