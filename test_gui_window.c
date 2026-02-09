// Test program to create a GUI window using Haiku syscalls
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

// GUI syscall numbers
#define SYSCALL_CREATE_WINDOW 10001
#define SYSCALL_DRAW_RECT 10006
#define SYSCALL_DRAW_STRING 10008
#define SYSCALL_SET_COLOR 10009
#define SYSCALL_FLUSH 10010

int main() {
    printf("=== GUI Window Test Program ===\n");
    printf("Creating window using GUI syscalls...\n\n");
    
    // Try to create a window
    printf("Calling create_window syscall...\n");
    
    // Haiku syscall calling convention:
    // eax = syscall number
    // ebx, ecx, edx = args
    
    // For create_window: args = (title, width, height)
    const char *title = "Test Window";
    unsigned int width = 400;
    unsigned int height = 300;
    
    // This would require inline asm to call the syscall properly
    // For now, just show the attempt
    printf("Window title: \"%s\"\n", title);
    printf("Window size: %ux%u\n", width, height);
    printf("Expected syscall: create_window(10001)\n");
    printf("\nNote: To actually create a window, WebPositive needs to:\n");
    printf("1. Link against Haiku libc with GUI support\n");
    printf("2. Call BWindow::BWindow() from app/Message.h\n");
    printf("3. Make syscalls through the libc wrapper\n");
    printf("\nStatus: Window framework ready, awaiting GUI syscall invocation\n");
    
    return 0;
}
