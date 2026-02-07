#pragma once

#include "PlatformTypes.h"
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

// Phase 4: GUI Syscalls for Haiku window rendering
// Implements minimal BMessage/BWindow/BView syscalls

class Phase4GUISyscallHandler {
public:
    // Haiku GUI syscall numbers (estimated from kernel syscall table)
    static constexpr int SYSCALL_CREATE_WINDOW = 10001;
    static constexpr int SYSCALL_DESTROY_WINDOW = 10002;
    static constexpr int SYSCALL_POST_MESSAGE = 10003;
    static constexpr int SYSCALL_GET_MESSAGE = 10004;
    static constexpr int SYSCALL_DRAW_LINE = 10005;
    static constexpr int SYSCALL_DRAW_RECT = 10006;
    static constexpr int SYSCALL_FILL_RECT = 10007;
    static constexpr int SYSCALL_DRAW_STRING = 10008;
    static constexpr int SYSCALL_SET_COLOR = 10009;
    static constexpr int SYSCALL_FLUSH = 10010;
    
    struct Window {
        int32_t window_id;
        char title[256];
        uint32_t width;
        uint32_t height;
        uint32_t x;
        uint32_t y;
        bool visible;
        uint32_t bg_color;
    };
    
    Phase4GUISyscallHandler() : next_window_id(1), output_lines(0) {
        printf("[GUI] Initialized GUI Syscall Handler\n");
    }
    
    ~Phase4GUISyscallHandler() {}
    
    bool HandleGUISyscall(int syscall_num, uint32_t *args, uint32_t *result) {
        printf("[GUI] Syscall %d", syscall_num);
        fflush(stdout);
        
        switch (syscall_num) {
            case SYSCALL_CREATE_WINDOW:
                return HandleCreateWindow(args, result);
                
            case SYSCALL_DESTROY_WINDOW:
                return HandleDestroyWindow(args, result);
                
            case SYSCALL_POST_MESSAGE:
                return HandlePostMessage(args, result);
                
            case SYSCALL_DRAW_RECT:
                return HandleDrawRect(args, result);
                
            case SYSCALL_FILL_RECT:
                return HandleFillRect(args, result);
                
            case SYSCALL_DRAW_STRING:
                return HandleDrawString(args, result);
                
            case SYSCALL_SET_COLOR:
                return HandleSetColor(args, result);
                
            case SYSCALL_FLUSH:
                return HandleFlush(args, result);
                
            default:
                printf(" (UNIMPLEMENTED)\n");
                *result = -1;
                return false;
        }
    }
    
    void PrintWindowInfo() {
        if (windows.empty()) {
            printf("[GUI] No windows created\n");
            return;
        }
        
        printf("[GUI] ============================================\n");
        printf("[GUI] Active Windows:\n");
        for (const auto &w : windows) {
            printf("[GUI]   Window %d: \"%s\" (%ux%u) at (%u,%u) %s\n",
                   w.second.window_id, w.second.title,
                   w.second.width, w.second.height,
                   w.second.x, w.second.y,
                   w.second.visible ? "VISIBLE" : "HIDDEN");
        }
        printf("[GUI] ============================================\n");
    }
    
private:
    std::map<int32_t, Window> windows;
    int32_t next_window_id;
    int output_lines;
    
    bool HandleCreateWindow(uint32_t *args, uint32_t *result) {
        const char *title = (const char *)args[0];
        uint32_t width = args[1];
        uint32_t height = args[2];
        
        Window w;
        w.window_id = next_window_id++;
        strncpy(w.title, title ? title : "Window", sizeof(w.title) - 1);
        w.width = width;
        w.height = height;
        w.x = 100;
        w.y = 100;
        w.visible = true;
        w.bg_color = 0xFFFFFFFF;  // White
        
        windows[w.window_id] = w;
        
        printf(" create_window(\"%s\", %ux%u) -> ID=%d\n", 
               title ? title : "Window", width, height, w.window_id);
        
        *result = w.window_id;
        return false;
    }
    
    bool HandleDestroyWindow(uint32_t *args, uint32_t *result) {
        int32_t window_id = args[0];
        
        if (windows.find(window_id) != windows.end()) {
            printf(" destroy_window(%d) -> OK\n", window_id);
            windows.erase(window_id);
            *result = 0;
        } else {
            printf(" destroy_window(%d) -> NOT_FOUND\n", window_id);
            *result = -1;
        }
        
        return false;
    }
    
    bool HandlePostMessage(uint32_t *args, uint32_t *result) {
        int32_t window_id = args[0];
        uint32_t message_code = args[1];
        
        printf(" post_message(win=%d, msg=0x%08x)\n", window_id, message_code);
        *result = 0;
        return false;
    }
    
    bool HandleDrawRect(uint32_t *args, uint32_t *result) {
        int32_t window_id = args[0];
        uint32_t x = args[1];
        uint32_t y = args[2];
        uint32_t w = args[3];
        uint32_t h = args[4];
        
        printf(" draw_rect(win=%d, rect=(%u,%u,%u,%u))\n", 
               window_id, x, y, w, h);
        
        if (output_lines++ < 10) {
            // Print a visual representation (first 10 draws only)
            printf("[GUI] [DRAW_RECT at (%u,%u) size %ux%u]\n", x, y, w, h);
        }
        
        *result = 0;
        return false;
    }
    
    bool HandleFillRect(uint32_t *args, uint32_t *result) {
        int32_t window_id = args[0];
        uint32_t x = args[1];
        uint32_t y = args[2];
        uint32_t w = args[3];
        uint32_t h = args[4];
        uint32_t color = args[5];
        
        printf(" fill_rect(win=%d, rect=(%u,%u,%u,%u), color=0x%08x)\n",
               window_id, x, y, w, h, color);
        
        if (output_lines++ < 10) {
            printf("[GUI] [FILL_RECT color=0x%08x at (%u,%u) size %ux%u]\n",
                   color, x, y, w, h);
        }
        
        *result = 0;
        return false;
    }
    
    bool HandleDrawString(uint32_t *args, uint32_t *result) {
        int32_t window_id = args[0];
        uint32_t x = args[1];
        uint32_t y = args[2];
        const char *text = (const char *)args[3];
        
        printf(" draw_string(win=%d, pos=(%u,%u), text=\"%s\")\n",
               window_id, x, y, text ? text : "");
        
        if (output_lines++ < 10) {
            printf("[GUI] [TEXT at (%u,%u): \"%s\"]\n", x, y, text ? text : "");
        }
        
        *result = 0;
        return false;
    }
    
    bool HandleSetColor(uint32_t *args, uint32_t *result) {
        int32_t window_id = args[0];
        uint32_t color = args[1];
        
        printf(" set_color(win=%d, color=0x%08x)\n", window_id, color);
        *result = 0;
        return false;
    }
    
    bool HandleFlush(uint32_t *args, uint32_t *result) {
        int32_t window_id = args[0];
        printf(" flush(win=%d)\n", window_id);
        *result = 0;
        return false;
    }
};
