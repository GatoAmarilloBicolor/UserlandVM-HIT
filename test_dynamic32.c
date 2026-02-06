#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Hello from dynamic 32-bit program!\n");
    printf("Testing dynamic linking...\n");
    
    // This will use dynamic linking
    malloc(100);
    
    printf("Dynamic test completed successfully!\n");
    return 42;
}