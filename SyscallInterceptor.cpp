// Syscall Interceptor for UserlandVM
// Intercepts guest syscalls and routes them appropriately

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

// Forward declarations to renderer
extern "C" {
    void renderer_draw_rect(int x, int y, int w, int h, uint32_t color);
    void renderer_draw_text(int x, int y, const char* text);
    void renderer_draw_line(int x1, int y1, int x2, int y2, uint32_t color);
    void renderer_clear();
}

// Haiku syscall numbers (guest perspective)
#define SYSCALL_READ            0x01
#define SYSCALL_WRITE           0x04
#define SYSCALL_EXIT            0x01
#define SYSCALL_DRAW_RECT       0x2712
#define SYSCALL_DRAW_TEXT       0x2713
#define SYSCALL_DRAW_LINE       0x2714
#define SYSCALL_CLEAR           0x2715
#define SYSCALL_CREATE_WINDOW   0x2710
#define SYSCALL_SHOW_WINDOW     0x2711

struct GuestRegisters {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp;
};

extern "C" {

// Main syscall handler
int handle_guest_syscall(int syscall_num, GuestRegisters* regs, uint8_t* memory) {
    printf("[SYSCALL] Intercepted: 0x%04x\n", syscall_num);
    
    switch (syscall_num) {
        // Write syscall (0x04)
        case SYSCALL_WRITE: {
            int fd = regs->ebx;
            uint32_t buf_addr = regs->ecx;
            int count = regs->edx;
            printf("[SYSCALL] WRITE(fd=%d, buf=0x%08x, count=%d)\n", fd, buf_addr, count);
            
            if (fd == 1 || fd == 2) {  // stdout or stderr
                char buffer[1024];
                memcpy(buffer, memory + buf_addr, count < sizeof(buffer)-1 ? count : sizeof(buffer)-1);
                buffer[count < sizeof(buffer)-1 ? count : sizeof(buffer)-1] = '\0';
                printf("[GUEST] %s", buffer);
            }
            regs->eax = count;
            return 0;
        }
        
        // Draw rectangle (0x2712)
        case SYSCALL_DRAW_RECT: {
            int x = regs->ebx;
            int y = regs->ecx;
            int w = regs->edx;
            int h = regs->esi;
            uint32_t color = regs->edi;
            printf("[SYSCALL] DRAW_RECT: (%d,%d) %dx%d color=0x%06x\n", x, y, w, h, color);
            
            renderer_draw_rect(x, y, w, h, color);
            regs->eax = 0;
            return 0;
        }
        
        // Draw text (0x2713)
        case SYSCALL_DRAW_TEXT: {
            int x = regs->ebx;
            int y = regs->ecx;
            uint32_t text_addr = regs->edx;
            
            char text[256];
            strncpy(text, (const char*)(memory + text_addr), sizeof(text)-1);
            text[sizeof(text)-1] = '\0';
            
            printf("[SYSCALL] DRAW_TEXT: (%d,%d) text='%s'\n", x, y, text);
            renderer_draw_text(x, y, text);
            regs->eax = 0;
            return 0;
        }
        
        // Draw line (0x2714)
        case SYSCALL_DRAW_LINE: {
            int x1 = regs->ebx;
            int y1 = regs->ecx;
            int x2 = regs->edx;
            int y2 = regs->esi;
            uint32_t color = regs->edi;
            printf("[SYSCALL] DRAW_LINE: (%d,%d) -> (%d,%d) color=0x%06x\n", x1, y1, x2, y2, color);
            
            renderer_draw_line(x1, y1, x2, y2, color);
            regs->eax = 0;
            return 0;
        }
        
        // Clear view (0x2715)
        case SYSCALL_CLEAR: {
            printf("[SYSCALL] CLEAR_VIEW\n");
            renderer_clear();
            regs->eax = 0;
            return 0;
        }
        
        // Exit syscall
        case 0x01: {
            int status = regs->ebx;
            printf("[SYSCALL] EXIT(%d)\n", status);
            regs->eax = status;
            return -1;  // Signal to stop execution
        }
        
        default:
            printf("[SYSCALL] Unhandled syscall: 0x%04x (eax=%d)\n", syscall_num, regs->eax);
            regs->eax = -1;
            return 0;
    }
}

}
