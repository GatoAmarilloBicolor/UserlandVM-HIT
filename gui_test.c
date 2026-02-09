// Simple GUI test program - creates a window using direct syscalls
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// GUI syscall numbers
#define SYSCALL_CREATE_WINDOW 10001
#define SYSCALL_DRAW_RECT 10006
#define SYSCALL_FILL_RECT 10007
#define SYSCALL_DRAW_STRING 10008
#define SYSCALL_SET_COLOR 10009
#define SYSCALL_FLUSH 10010

// Helper function to make syscalls
static inline long syscall3(int number, long arg1, long arg2, long arg3) {
    long result;
    asm volatile(
        "movl %1, %%eax\n\t"     // syscall number -> eax
        "movl %2, %%ebx\n\t"     // arg1 -> ebx
        "movl %3, %%ecx\n\t"     // arg2 -> ecx
        "movl %4, %%edx\n\t"     // arg3 -> edx
        "int $0x63\n\t"          // Haiku syscall interrupt
        "movl %%eax, %0\n\t"     // result <- eax
        : "=m" (result)
        : "rm" (number), "rm" (arg1), "rm" (arg2), "rm" (arg3)
        : "eax", "ebx", "ecx", "edx", "memory"
    );
    return result;
}

int main() {
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║   GUI Test Program - Creating Window                ║\n");
    printf("║   UserlandVM-HIT Direct Syscall Demo                ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    // Create window using GUI syscall
    printf("[GUI] Creating window...\n");
    printf("  Title: \"Test GUI Window\"\n");
    printf("  Size: 400x300 pixels\n");
    printf("  Position: (100, 100)\n\n");
    
    // Call create_window syscall
    // Args: (const char* title, int width, int height)
    long window_id = syscall3(
        SYSCALL_CREATE_WINDOW,
        (long)"Test GUI Window",
        400,      // width
        300       // height
    );
    
    printf("[SYSCALL] create_window(10001) returned: %ld\n", window_id);
    
    if (window_id > 0) {
        printf("✅ Window created successfully!\n");
        printf("   Window ID: %ld\n\n", window_id);
        
        // Set color to blue (RGB)
        printf("[GUI] Setting color to blue...\n");
        syscall3(SYSCALL_SET_COLOR, 0x0000FF, 0, 0);
        printf("✅ Color set\n\n");
        
        // Draw a rectangle
        printf("[GUI] Drawing rectangle...\n");
        printf("  Position: (50, 50)\n");
        printf("  Size: 300x200\n");
        syscall3(SYSCALL_DRAW_RECT, (50 << 16) | 50, (300 << 16) | 200, 0);
        printf("✅ Rectangle drawn\n\n");
        
        // Set color to white for text
        printf("[GUI] Setting color to white for text...\n");
        syscall3(SYSCALL_SET_COLOR, 0xFFFFFF, 0, 0);
        printf("✅ Color set\n\n");
        
        // Draw text
        printf("[GUI] Drawing text...\n");
        syscall3(SYSCALL_DRAW_STRING, (long)"Hello from UserlandVM!", 100, 150);
        printf("✅ Text drawn\n\n");
        
        // Flush to display
        printf("[GUI] Flushing display...\n");
        syscall3(SYSCALL_FLUSH, window_id, 0, 0);
        printf("✅ Display updated\n\n");
        
        printf("════════════════════════════════════════════════════\n");
        printf("Window should now be visible on your Haiku desktop!\n");
        printf("Press Ctrl+C to close this program.\n");
        printf("════════════════════════════════════════════════════\n\n");
        
        // Keep window open
        sleep(10);
        printf("\nProgram ending, window will close.\n");
    } else {
        printf("❌ Failed to create window\n");
        printf("   Window ID: %ld\n", window_id);
        return 1;
    }
    
    return 0;
}
