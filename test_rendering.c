// Test program for rendering in UserlandVM
// Demonstrates the drawing syscall interface

#include <stdio.h>

// Syscall numbers
#define SYSCALL_DRAW_RECT   0x2712
#define SYSCALL_DRAW_TEXT   0x2713
#define SYSCALL_DRAW_LINE   0x2714
#define SYSCALL_CLEAR       0x2715

// Inline syscall macro for x86-32
#define SYSCALL(n) \
    __asm__ volatile("mov $" #n ", %%eax; int $0x80" : : : "eax")

int main() {
    printf("Testing rendering syscalls...\n");
    
    // Clear the window
    __asm__ volatile("mov $0x2715, %eax; int $0x80");
    printf("Cleared window\n");
    
    // Draw a blue background rect (0, 0, 1000, 700, 0x0000FF)
    __asm__ volatile(
        "mov $0x2712, %%eax\n"
        "mov $0, %%ebx\n"
        "mov $0, %%ecx\n"
        "mov $1000, %%edx\n"
        "mov $700, %%esi\n"
        "mov $0x0000FF, %%edi\n"
        "int $0x80"
        : : : "eax", "ebx", "ecx", "edx", "esi", "edi"
    );
    printf("Drew blue background\n");
    
    // Draw a white rect for title bar (0, 0, 1000, 50, 0xFFFFFF)
    __asm__ volatile(
        "mov $0x2712, %%eax\n"
        "mov $0, %%ebx\n"
        "mov $0, %%ecx\n"
        "mov $1000, %%edx\n"
        "mov $50, %%esi\n"
        "mov $0xFFFFFF, %%edi\n"
        "int $0x80"
        : : : "eax", "ebx", "ecx", "edx", "esi", "edi"
    );
    printf("Drew title bar\n");
    
    // Draw a green button (100, 300, 200, 50, 0x00FF00)
    __asm__ volatile(
        "mov $0x2712, %%eax\n"
        "mov $100, %%ebx\n"
        "mov $300, %%ecx\n"
        "mov $200, %%edx\n"
        "mov $50, %%esi\n"
        "mov $0x00FF00, %%edi\n"
        "int $0x80"
        : : : "eax", "ebx", "ecx", "edx", "esi", "edi"
    );
    printf("Drew button\n");
    
    // Draw title text (10, 20, "WebPositive - UserlandVM")
    // This would require passing a string pointer via ecx
    
    printf("Rendering test complete\n");
    
    return 0;
}
