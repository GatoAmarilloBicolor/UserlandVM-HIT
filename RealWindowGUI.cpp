#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <os/interface/Window.h>
#include <os/interface/View.h>
#include <os/interface/Rect.h>
#include <os/app/Application.h>

using namespace BPrivate;

static BApplication* g_app = nullptr;
static BWindow* g_window = nullptr;
static thread_id g_app_thread = -1;
static volatile bool g_window_shown = false;

// Simple view that draws content
class OutputView : public BView {
public:
    OutputView(BRect rect);
    virtual void Draw(BRect rect);
};

OutputView::OutputView(BRect rect)
    : BView(rect, "output", B_FOLLOW_ALL, B_WILL_DRAW)
{
    SetViewColor(216, 216, 216);
}

void OutputView::Draw(BRect rect)
{
    SetHighColor(0, 0, 0);
    
    // Draw title
    BFont font;
    GetFont(&font);
    SetFont(be_bold_font);
    
    DrawString("Haiku Program Execution", BPoint(30, 40));
    
    SetFont(&font);
    DrawString("UserlandVM-HIT", BPoint(30, 70));
    DrawString("Executing Haiku 32-bit application", BPoint(30, 100));
    DrawString("Window Server: Active", BPoint(30, 130));
}

// Main window
class OutputWindow : public BWindow {
public:
    OutputWindow(const char* title);
    virtual ~OutputWindow();
    virtual void MessageReceived(BMessage* msg);
    virtual bool QuitRequested();
};

OutputWindow::OutputWindow(const char* title)
    : BWindow(BRect(50, 50, 950, 550), title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE)
{
    AddChild(new OutputView(Bounds()));
}

OutputWindow::~OutputWindow()
{
}

void OutputWindow::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        default:
            BWindow::MessageReceived(msg);
            break;
    }
}

bool OutputWindow::QuitRequested()
{
    return true;
}

// Application
class VMApplication : public BApplication {
public:
    VMApplication() : BApplication("application/x-vnd.vm-hit") {}
    virtual void ReadyToRun();
};

void VMApplication::ReadyToRun()
{
    printf("[RealWindowGUI] ReadyToRun: Showing window\n");
    if (g_window) {
        g_window->Show();
        g_window_shown = true;
        printf("[RealWindowGUI] ✓ Window shown on screen\n");
    }
}

// Application thread function
static int32 app_thread_func(void* arg)
{
    printf("[RealWindowGUI] App thread: Calling Run()\n");
    if (g_app) {
        g_app->Run();
    }
    printf("[RealWindowGUI] App thread: Run() returned\n");
    return 0;
}

extern "C" {

void CreateHaikuWindow(const char* title)
{
    printf("[RealWindowGUI] CreateHaikuWindow: '%s'\n", title);
    
    if (g_app != nullptr) {
        printf("[RealWindowGUI] App already initialized\n");
        return;
    }
    
    try {
        g_app = new VMApplication();
        printf("[RealWindowGUI] ✓ BApplication created\n");
        
        g_window = new OutputWindow(title);
        printf("[RealWindowGUI] ✓ BWindow created\n");
        
        // Spawn application thread
        g_app_thread = spawn_thread(app_thread_func, "VM_AppThread", B_NORMAL_PRIORITY, NULL);
        if (g_app_thread >= 0) {
            resume_thread(g_app_thread);
            printf("[RealWindowGUI] ✓ App thread spawned (id: %ld)\n", g_app_thread);
        }
        
    } catch (...) {
        printf("[RealWindowGUI] ERROR: Exception creating window\n");
    }
}

void ShowHaikuWindow()
{
    printf("[RealWindowGUI] ShowHaikuWindow\n");
    
    // Window is shown by app's ReadyToRun()
    // Wait for it to be shown
    for (int i = 0; i < 50; i++) {
        if (g_window_shown) {
            printf("[RealWindowGUI] ✓ Window is visible\n");
            return;
        }
        snooze(100000);  // 100ms
    }
    
    printf("[RealWindowGUI] WARNING: Window may not be visible\n");
}

void ProcessWindowEvents()
{
    printf("[RealWindowGUI] ProcessWindowEvents: Processing for 5 seconds\n");
    
    // Let the app run and process events
    snooze(5000000);  // 5 seconds
    
    printf("[RealWindowGUI] ✓ Event processing complete\n");
}

void DestroyHaikuWindow()
{
    printf("[RealWindowGUI] DestroyHaikuWindow\n");
    
    if (g_window) {
        g_window->PostMessage(B_QUIT_REQUESTED);
        printf("[RealWindowGUI] ✓ Posted quit message\n");
    }
    
    if (g_app) {
        g_app->PostMessage(B_QUIT_REQUESTED);
        printf("[RealWindowGUI] ✓ Posted app quit message\n");
    }
    
    if (g_app_thread >= 0) {
        status_t exit_code;
        wait_for_thread(g_app_thread, &exit_code);
        printf("[RealWindowGUI] ✓ App thread exited\n");
        g_app_thread = -1;
    }
    
    g_window = nullptr;
    g_app = nullptr;
    g_window_shown = false;
    
    printf("[RealWindowGUI] ✓ Window destroyed\n");
}

}  // extern "C"
