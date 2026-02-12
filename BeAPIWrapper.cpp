#include "BeAPIWrapper.h"
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <cstdio>

static BApplication* g_app = nullptr;
static BWindow* g_window = nullptr;

void CreateHaikuWindow(const char* title) {
    if (!g_app) {
        g_app = new BApplication("application/x-userlandvm");
    }
    
    // Optimized size for WebPositive
    BRect rect(50, 50, 1074, 818);  // 1024x768 content area
    g_window = new BWindow(rect, title ? title : "UserlandVM", B_TITLED_WINDOW, 
                          B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
    
    BView* view = new BView(g_window->Bounds(), "content", B_FOLLOW_ALL, B_WILL_DRAW);
    view->SetViewColor(240, 240, 240);  // Light gray for WebPositive
    g_window->AddChild(view);
    
    printf("[BeAPIWrapper] Ventana Haiku GUI creada (non-headless): '%s'\n", title);
    printf("[BeAPIWrapper] ✅ WebPositive window ready for browser content\n");
}

void ShowHaikuWindow() {
    if (g_window) {
        g_window->Show();
        printf("[BeAPIWrapper] ✅ Ventana GUI mostrada (non-headless)\n");
        printf("[BeAPIWrapper] 🎯 WebPositive ready for user interaction\n");
    }
}

void ProcessWindowEvents() {
    if (g_app) {
        g_app->Run();
    }
}
