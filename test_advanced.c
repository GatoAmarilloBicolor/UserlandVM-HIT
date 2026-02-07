#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    printf("=== Advanced Test Program ===\n");
    printf("Arguments received: %d\n", argc);
    
    for (int i = 0; i < argc; i++) {
        printf("Arg[%d]: %s\n", i, argv[i]);
    }
    
    printf("Testing dynamic allocation...\n");
    int* buffer = (int*)malloc(100 * sizeof(int));
    if (buffer) {
        for (int i = 0; i < 100; i++) {
            buffer[i] = i * i;
        }
        int sum = 0;
        for (int i = 0; i < 100; i++) {
            sum += buffer[i];
        }
        printf("Sum of squares 0-99: %d\n", sum);
        free(buffer);
        printf("Memory test passed!\n");
    }
    
    printf("Advanced test completed successfully!\n");
    return 123;
}