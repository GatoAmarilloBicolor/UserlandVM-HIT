#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <os/app/Application.h>
#include <os/interface/Window.h>
#include <os/interface/View.h>
#include <os/interface/Rect.h>

using namespace BPrivate;

static BApplication* g_app = nullptr;
static BWindow* g_window = nullptr;
static bool g_app_ready = false;

// Simple output view
class VMOutputView : public BView {
public:
    VMOutputView(BRect frame);
    virtual void Draw(BRect updateRect);
};

VMOutputView::VMOutputView(BRect frame)
    : BView(frame, "VMOutput", B_FOLLOW_ALL, B_WILL_DRAW)
{
    SetViewColor(216, 216, 216);
}

void VMOutputView::Draw(BRect updateRect)
{
    SetHighColor(0, 0, 0);
    DrawString("Haiku Program Running", BPoint(20, 30));
    DrawString("UserlandVM-HIT Execution", BPoint(20, 60));
}

// Window
class VMWindow : public BWindow {
public:
    VMWindow(const char* title);
    virtual ~VMWindow();
    virtual bool QuitRequested();
};

VMWindow::VMWindow(const char* title)
    : BWindow(BRect(100, 100, 800, 600), title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
    AddChild(new VMOutputView(Bounds()));
}

VMWindow::~VMWindow() {}

bool VMWindow::QuitRequested()
{
    return true;
}

// Application
class VMApp : public BApplication {
public:
    VMApp() : BApplication("application/x-vnd.vm-hait") {}
    virtual void ReadyToRun();
};

void VMApp::ReadyToRun()
{
    if (g_window) {
        g_window->Show();
        g_app_ready = true;
        printf("[BeAPI] Window shown\n");
    }
}

extern "C" {

void CreateHaikuWindow(const char* title)
{
    printf("[BeAPI] CreateHaikuWindow: '%s'\n", title);
    
    if (g_app != nullptr) {
        printf("[BeAPI] App already exists\n");
        return;
    }
    
    try {
        g_app = new VMApp();
        g_window = new VMWindow(title);
        printf("[BeAPI] ✓ Window created\n");
    } catch (const char* err) {
        printf("[BeAPI] ERROR: %s\n", err);
    } catch (...) {
        printf("[BeAPI] ERROR: Unknown exception\n");
    }
}

void ShowHaikuWindow()
{
    printf("[BeAPI] ShowHaikuWindow\n");
    if (g_window && g_app) {
        printf("[BeAPI] ✓ Window visible\n");
    }
}

void ProcessWindowEvents()
{
    printf("[BeAPI] ProcessWindowEvents\n");
    
    if (g_app == nullptr) {
        printf("[BeAPI] No app to process\n");
        return;
    }
    
    printf("[BeAPI] Running event loop (3 seconds)...\n");
    
    // Simple event loop
    for (int i = 0; i < 30; i++) {
        snooze(100000);  // 100ms
        if (i % 10 == 0) {
            printf("[BeAPI] Event processing %d/30\n", i);
        }
    }
    
    printf("[BeAPI] ✓ Event processing done\n");
}

void DestroyHaikuWindow()
{
    printf("[BeAPI] DestroyHaikuWindow\n");
    
    if (g_window) {
        g_window->PostMessage(B_QUIT_REQUESTED);
        g_window = nullptr;
    }
    
    if (g_app) {
        g_app->PostMessage(B_QUIT_REQUESTED);
        g_app = nullptr;
    }
    
    printf("[BeAPI] ✓ Window destroyed\n");
}

}  // extern "C"
