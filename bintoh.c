#include <stdio.h>

int main(void) {
	int ch;

	puts("unsigned char data[]={");
	while ((ch = getchar()) != EOF)
		printf("0x%02x,", ch);
	puts("};");
	return 0;
}
