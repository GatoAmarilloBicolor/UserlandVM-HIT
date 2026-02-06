#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Hello from 32-bit UserlandVM test!\n");
    printf("Testing basic syscalls...\n");
    
    // Test write syscall
    fwrite("Test write to stdout\n", 1, 22, stdout);
    
    // Test exit
    printf("Exiting with code 42\n");
    return 42;
}