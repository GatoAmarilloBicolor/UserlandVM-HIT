// Simple x86-32 Haiku window test application
// Tests Be API window creation through UserlandVM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Haiku-style syscalls for window creation
// These will be intercepted by our VM and forwarded to Be API

// Mock Haiku syscalls (these would normally come from OS.h)
#define B_OK 0
#define B_TITLED_WINDOW 1

typedef unsigned int uint32;

// Simulated Be API syscalls that our VM wrapper will handle
int create_window_syscall(const char* title, int x, int y, int width, int height) {
    // In a real scenario, this would be a syscall (e.g., 0x2710 + offset)
    printf("WINDOW_CREATE: '%s' at (%d,%d) size %dx%d\n", title, x, y, width, height);
    return 0;
}

int show_window_syscall(int window_id) {
    printf("WINDOW_SHOW: id=%d\n", window_id);
    return 0;
}

int draw_rect_syscall(int window_id, int x, int y, int w, int h, int color) {
    printf("WINDOW_DRAW_RECT: id=%d, (%d,%d) %dx%d color=%06x\n", window_id, x, y, w, h, color);
    return 0;
}

int draw_text_syscall(int window_id, int x, int y, const char* text) {
    printf("WINDOW_DRAW_TEXT: id=%d at (%d,%d) text='%s'\n", window_id, x, y, text);
    return 0;
}

int main() {
    printf("=== UserlandVM Window Test Application ===\n");
    printf("Testing Be API window creation through VM\n\n");
    
    // Create a window
    printf("[APP] Creating window...\n");
    int window = create_window_syscall("Test Window - UserlandVM", 100, 100, 640, 480);
    
    // Draw some content
    printf("[APP] Drawing background...\n");
    draw_rect_syscall(window, 0, 0, 640, 480, 0xFFFFFF);  // White background
    
    printf("[APP] Drawing title...\n");
    draw_text_syscall(window, 50, 50, "WebPositive Test");
    
    printf("[APP] Drawing content...\n");
    draw_rect_syscall(window, 50, 100, 540, 350, 0xEEEEEE);  // Light gray box
    
    printf("[APP] Drawing text...\n");
    draw_text_syscall(window, 60, 120, "Rendering test content");
    draw_text_syscall(window, 60, 150, "from 32-bit guest app");
    
    // Show the window
    printf("[APP] Showing window...\n");
    show_window_syscall(window);
    
    printf("[APP] Test complete\n");
    printf("=== Window should now be visible on desktop ===\n");
    
    return 0;
}
