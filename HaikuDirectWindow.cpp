#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <os/interface/Screen.h>
#include <os/interface/Bitmap.h>
#include <os/interface/View.h>
#include <os/interface/Window.h>
#include <os/interface/Rect.h>
#include <os/app/Application.h>

using namespace BPrivate;

static BApplication* g_app = nullptr;
static BWindow* g_window = nullptr;
static thread_id g_app_thread = -1;
static BDirectWindow* g_direct_window = nullptr;

class VMDirectWindow : public BDirectWindow {
public:
    VMDirectWindow(const char* title);
    virtual ~VMDirectWindow();
    virtual void DirectConnected(direct_buffer_info* info);
    virtual bool QuitRequested();

private:
    direct_buffer_info* buffer_info;
};

VMDirectWindow::VMDirectWindow(const char* title)
    : BDirectWindow(BRect(100, 100, 800, 600), title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
    buffer_info = nullptr;
}

VMDirectWindow::~VMDirectWindow()
{
}

void VMDirectWindow::DirectConnected(direct_buffer_info* info)
{
    buffer_info = info;
    if (info) {
        printf("[DirectWindow] Connected to graphics buffer\n");
        printf("[DirectWindow] Buffer bits_per_pixel: %d\n", info->bits_per_pixel);
    }
}

bool VMDirectWindow::QuitRequested()
{
    return true;
}

class VMApplication : public BApplication {
public:
    VMApplication();
    virtual void ReadyToRun();
};

VMApplication::VMApplication()
    : BApplication("application/x-vnd.vm-hait")
{
}

void VMApplication::ReadyToRun()
{
    printf("[VMApp] ReadyToRun called\n");
    if (g_direct_window) {
        g_direct_window->Show();
        printf("[VMApp] ✓ DirectWindow shown\n");
    }
}

static int32 app_thread_func(void* arg)
{
    printf("[AppThread] Starting BApplication::Run()\n");
    if (g_app) {
        g_app->Run();
    }
    printf("[AppThread] BApplication::Run() completed\n");
    return 0;
}

extern "C" {

void CreateHaikuWindow(const char* title)
{
    printf("[HaikuWindow] CreateHaikuWindow: '%s'\n", title);
    
    if (g_app != nullptr) {
        printf("[HaikuWindow] Application already exists\n");
        return;
    }
    
    try {
        // Create application
        g_app = new VMApplication();
        printf("[HaikuWindow] ✓ BApplication created\n");
        
        // Create direct window for raw rendering
        g_direct_window = new VMDirectWindow(title);
        printf("[HaikuWindow] ✓ BDirectWindow created\n");
        
        // Start app thread
        g_app_thread = spawn_thread(app_thread_func, "VMApp", B_NORMAL_PRIORITY, NULL);
        if (g_app_thread >= 0) {
            resume_thread(g_app_thread);
            printf("[HaikuWindow] ✓ Application thread started (id: %ld)\n", g_app_thread);
        }
        
        // Wait for app to initialize
        snooze(500000);  // 500ms
        printf("[HaikuWindow] ✓ Window initialization complete\n");
        
    } catch (...) {
        printf("[HaikuWindow] ERROR: Exception during window creation\n");
    }
}

void ShowHaikuWindow()
{
    printf("[HaikuWindow] ShowHaikuWindow\n");
    
    if (g_direct_window == nullptr) {
        printf("[HaikuWindow] ERROR: No window to show\n");
        return;
    }
    
    if (g_direct_window->IsHidden()) {
        g_direct_window->Show();
        printf("[HaikuWindow] ✓ Window made visible\n");
    } else {
        printf("[HaikuWindow] ✓ Window already visible\n");
    }
}

void ProcessWindowEvents()
{
    printf("[HaikuWindow] ProcessWindowEvents\n");
    
    if (g_app == nullptr) {
        printf("[HaikuWindow] ERROR: No application\n");
        return;
    }
    
    printf("[HaikuWindow] Processing events for 3 seconds...\n");
    snooze(3000000);  // 3 seconds
    printf("[HaikuWindow] ✓ Event processing complete\n");
}

void DestroyHaikuWindow()
{
    printf("[HaikuWindow] DestroyHaikuWindow\n");
    
    if (g_direct_window) {
        g_direct_window->PostMessage(B_QUIT_REQUESTED);
        g_direct_window = nullptr;
    }
    
    if (g_app) {
        g_app->PostMessage(B_QUIT_REQUESTED);
    }
    
    if (g_app_thread >= 0) {
        status_t exit_status;
        wait_for_thread(g_app_thread, &exit_status);
        g_app_thread = -1;
    }
    
    g_app = nullptr;
    printf("[HaikuWindow] ✓ Window destroyed\n");
}

}  // extern "C"
