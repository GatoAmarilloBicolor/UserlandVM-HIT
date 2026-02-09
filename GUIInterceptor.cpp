// GUI Syscall Interceptor for UserlandVM
// Maps guest GUI syscalls to real Be API calls

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <String.h>

// Global application and window management
static BApplication* g_app = nullptr;
static std::map<int, BWindow*> g_windows;
static int g_window_counter = 1;

extern "C" {

// Initialize GUI subsystem
void gui_init(const char* app_signature) {
    if (!g_app) {
        printf("[GUI_INIT] Creating BApplication with signature: %s\n", app_signature);
        g_app = new BApplication(app_signature);
        printf("[GUI_INIT] ✓ BApplication created\n");
    }
}

// Create a window - guest syscall 0x2710
int gui_create_window(const char* title, int x, int y, int width, int height) {
    if (!g_app) {
        printf("[GUI_SYSCALL] ERROR: GUI not initialized\n");
        return -1;
    }
    
    printf("[GUI_SYSCALL] CREATE_WINDOW: '%s' at (%d, %d) size %dx%d\n", 
           title, x, y, width, height);
    
    BRect rect(x, y, x + width - 1, y + height - 1);
    BWindow* window = new BWindow(rect, title, B_TITLED_WINDOW, 0);
    
    // Add a view to the window
    BView* view = new BView(window->Bounds(), "content", B_FOLLOW_ALL, B_WILL_DRAW);
    view->SetViewColor(255, 255, 255);  // White background
    window->AddChild(view);
    
    // Show the window
    window->Show();
    
    // Store window reference
    int window_id = g_window_counter++;
    g_windows[window_id] = window;
    
    printf("[GUI_SYSCALL] ✓ Window created with ID: %d\n", window_id);
    return window_id;
}

// Show a window - guest syscall 0x2711
int gui_show_window(int window_id) {
    printf("[GUI_SYSCALL] SHOW_WINDOW: window_id=%d\n", window_id);
    
    if (g_windows.find(window_id) == g_windows.end()) {
        printf("[GUI_SYSCALL] ERROR: Window not found: %d\n", window_id);
        return -1;
    }
    
    BWindow* window = g_windows[window_id];
    if (!window->IsHidden()) {
        window->Show();
        printf("[GUI_SYSCALL] ✓ Window shown: %d\n", window_id);
    }
    return 0;
}

// Draw rectangle - guest syscall 0x2712
int gui_draw_rect(int window_id, int x, int y, int w, int h, uint32_t color) {
    printf("[GUI_SYSCALL] DRAW_RECT: window_id=%d, pos=(%d,%d) size=%dx%d color=0x%06x\n",
           window_id, x, y, w, h, color);
    
    if (g_windows.find(window_id) == g_windows.end()) {
        printf("[GUI_SYSCALL] ERROR: Window not found: %d\n", window_id);
        return -1;
    }
    
    BWindow* window = g_windows[window_id];
    BView* view = window->FindView("content");
    if (!view) {
        printf("[GUI_SYSCALL] ERROR: View not found\n");
        return -1;
    }
    
    // Extract RGB components
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    view->SetHighColor(r, g, b);
    view->FillRect(BRect(x, y, x + w - 1, y + h - 1));
    
    printf("[GUI_SYSCALL] ✓ Rectangle drawn\n");
    return 0;
}

// Draw text - guest syscall 0x2713
int gui_draw_text(int window_id, int x, int y, const char* text) {
    printf("[GUI_SYSCALL] DRAW_TEXT: window_id=%d, pos=(%d,%d) text='%s'\n",
           window_id, x, y, text);
    
    if (g_windows.find(window_id) == g_windows.end()) {
        printf("[GUI_SYSCALL] ERROR: Window not found: %d\n", window_id);
        return -1;
    }
    
    BWindow* window = g_windows[window_id];
    BView* view = window->FindView("content");
    if (!view) {
        printf("[GUI_SYSCALL] ERROR: View not found\n");
        return -1;
    }
    
    view->SetHighColor(0, 0, 0);  // Black text
    view->DrawString(text, BPoint(x, y));
    
    printf("[GUI_SYSCALL] ✓ Text drawn\n");
    return 0;
}

// Process window events
void gui_process_events() {
    if (g_app) {
        printf("[GUI_EVENTS] Running application event loop\n");
        g_app->Run();
    }
}

// Cleanup GUI
void gui_cleanup() {
    printf("[GUI_CLEANUP] Cleaning up GUI resources\n");
    
    for (auto& pair : g_windows) {
        if (pair.second) {
            pair.second->Quit();
        }
    }
    g_windows.clear();
    
    if (g_app) {
        g_app->Quit();
        g_app = nullptr;
    }
    
    printf("[GUI_CLEANUP] ✓ GUI cleaned up\n");
}

}  // extern "C"
