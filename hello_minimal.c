// Minimal hello world - calls syscalls directly
// For Haiku x86 32-bit emulator testing

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	printf("Hello from Haiku emulator!\n");
	printf("argc=%d\n", argc);
	
	if (argc > 1) {
		printf("arg1=%s\n", argv[1]);
	}
	
	return 0;
}
