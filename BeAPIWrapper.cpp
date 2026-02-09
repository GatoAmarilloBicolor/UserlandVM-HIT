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
    
    BRect rect(50, 50, 1100, 850);
    g_window = new BWindow(rect, title ? title : "UserlandVM", B_TITLED_WINDOW, 0);
    
    BView* view = new BView(g_window->Bounds(), "content", B_FOLLOW_ALL, B_WILL_DRAW);
    view->SetViewColor(255, 255, 255);
    g_window->AddChild(view);
    
    printf("[BeAPIWrapper] Ventana Haiku creada: '%s'\n", title);
}

void ShowHaikuWindow() {
    if (g_window) {
        g_window->Show();
        printf("[BeAPIWrapper] Ventana mostrada en desktop\n");
    }
}

void ProcessWindowEvents() {
    if (g_app) {
        g_app->Run();
    }
}
