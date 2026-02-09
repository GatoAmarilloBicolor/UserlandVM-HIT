// Real-Time Renderer for UserlandVM
// Captures and renders guest drawing commands to host window

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <thread>
#include <mutex>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <String.h>

// Drawing command types
enum DrawCommand {
    CMD_RECT = 1,
    CMD_TEXT = 2,
    CMD_LINE = 3,
    CMD_FILL = 4,
    CMD_CLEAR = 5,
    CMD_UPDATE = 6
};

struct DrawOp {
    DrawCommand cmd;
    int x, y, w, h;
    uint32_t color;
    char text[256];
};

// Global drawing queue
std::queue<DrawOp> g_draw_queue;
std::mutex g_draw_mutex;

// Global window and view
static BApplication* g_app = nullptr;
static BWindow* g_window = nullptr;
static BView* g_content_view = nullptr;

// Custom view for rendering
class ContentView : public BView {
public:
    ContentView(BRect frame) : BView(frame, "content", B_FOLLOW_ALL, B_WILL_DRAW) {
        SetViewColor(255, 255, 255);
    }
    
    void Draw(BRect updateRect) {
        // Process all pending draw commands
        std::lock_guard<std::mutex> lock(g_draw_mutex);
        
        while (!g_draw_queue.empty()) {
            DrawOp op = g_draw_queue.front();
            g_draw_queue.pop();
            
            switch (op.cmd) {
                case CMD_RECT: {
                    uint8_t r = (op.color >> 16) & 0xFF;
                    uint8_t g = (op.color >> 8) & 0xFF;
                    uint8_t b = op.color & 0xFF;
                    SetHighColor(r, g, b);
                    FillRect(BRect(op.x, op.y, op.x + op.w - 1, op.y + op.h - 1));
                    printf("[RENDER] FillRect(%d,%d,%dx%d) color=%06x\n", 
                           op.x, op.y, op.w, op.h, op.color);
                    break;
                }
                case CMD_TEXT: {
                    SetHighColor(0, 0, 0);
                    DrawString(op.text, BPoint(op.x, op.y + 12));
                    printf("[RENDER] DrawText(%d,%d) text='%s'\n", op.x, op.y, op.text);
                    break;
                }
                case CMD_LINE: {
                    uint8_t r = (op.color >> 16) & 0xFF;
                    uint8_t g = (op.color >> 8) & 0xFF;
                    uint8_t b = op.color & 0xFF;
                    SetHighColor(r, g, b);
                    StrokeLine(BPoint(op.x, op.y), BPoint(op.w, op.h));
                    printf("[RENDER] StrokeLine(%d,%d to %d,%d)\n", op.x, op.y, op.w, op.h);
                    break;
                }
                case CMD_CLEAR: {
                    SetViewColor(255, 255, 255);
                    FillRect(Bounds());
                    printf("[RENDER] ClearView\n");
                    break;
                }
                case CMD_UPDATE: {
                    Invalidate();
                    printf("[RENDER] InvalidateView\n");
                    break;
                }
            }
        }
    }
};

extern "C" {

// Initialize renderer with GUI
void renderer_init(const char* app_title) {
    if (!g_app) {
        printf("[RENDERER] Initializing real-time renderer\n");
        g_app = new BApplication("application/x-webpositive");
        
        BRect frame(100, 100, 1100, 850);
        g_window = new BWindow(frame, app_title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS);
        
        g_content_view = new ContentView(g_window->Bounds());
        g_window->AddChild(g_content_view);
        
        g_window->Show();
        printf("[RENDERER] ✓ Renderer initialized, window shown\n");
    }
}

// Queue a rectangle draw command
void renderer_draw_rect(int x, int y, int w, int h, uint32_t color) {
    DrawOp op;
    op.cmd = CMD_RECT;
    op.x = x;
    op.y = y;
    op.w = w;
    op.h = h;
    op.color = color;
    
    {
        std::lock_guard<std::mutex> lock(g_draw_mutex);
        g_draw_queue.push(op);
    }
    
    // Trigger redraw
    if (g_content_view) {
        g_content_view->Invalidate();
    }
}

// Queue a text draw command
void renderer_draw_text(int x, int y, const char* text) {
    DrawOp op;
    op.cmd = CMD_TEXT;
    op.x = x;
    op.y = y;
    op.color = 0x000000;
    strncpy(op.text, text, sizeof(op.text) - 1);
    
    {
        std::lock_guard<std::mutex> lock(g_draw_mutex);
        g_draw_queue.push(op);
    }
    
    // Trigger redraw
    if (g_content_view) {
        g_content_view->Invalidate();
    }
}

// Queue a line draw command
void renderer_draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    DrawOp op;
    op.cmd = CMD_LINE;
    op.x = x1;
    op.y = y1;
    op.w = x2;
    op.h = y2;
    op.color = color;
    
    {
        std::lock_guard<std::mutex> lock(g_draw_mutex);
        g_draw_queue.push(op);
    }
    
    // Trigger redraw
    if (g_content_view) {
        g_content_view->Invalidate();
    }
}

// Clear the view
void renderer_clear() {
    DrawOp op;
    op.cmd = CMD_CLEAR;
    
    {
        std::lock_guard<std::mutex> lock(g_draw_mutex);
        g_draw_queue.push(op);
    }
    
    if (g_content_view) {
        g_content_view->Invalidate();
    }
}

// Process window events
void renderer_process_events() {
    if (g_app) {
        printf("[RENDERER] Starting event loop\n");
        g_app->Run();
    }
}

// Cleanup
void renderer_cleanup() {
    printf("[RENDERER] Cleaning up\n");
    if (g_window) {
        g_window->Quit();
    }
    if (g_app) {
        g_app->Quit();
    }
}

}
