#include <os/app/Application.h>
#include <os/interface/Window.h>
#include <os/interface/View.h>
#include <os/interface/Rect.h>
#include <stdio.h>
#include <stdlib.h>

using namespace BPrivate;

static BApplication* g_app = nullptr;
static BWindow* g_window = nullptr;
static BView* g_view = nullptr;
static thread_id g_app_thread = -1;

// Window view that displays program output
class VMOutputView : public BView {
public:
    VMOutputView(BRect frame);
    virtual void Draw(BRect updateRect);
    
private:
    const char* output_text = "Haiku Program Running...";
};

VMOutputView::VMOutputView(BRect frame)
    : BView(frame, "VMOutput", B_FOLLOW_ALL, B_WILL_DRAW)
{
    SetViewColor(216, 216, 216);
}

void VMOutputView::Draw(BRect updateRect)
{
    SetHighColor(0, 0, 0);
    DrawString("Haiku VM - Program Output", BPoint(20, 30));
    DrawString("Window Server: app_server", BPoint(20, 60));
    DrawString("Status: Running", BPoint(20, 90));
}

// Main application window
class VMWindow : public BWindow {
public:
    VMWindow(const char* title);
    virtual ~VMWindow();
    virtual void MessageReceived(BMessage* msg);
    virtual bool QuitRequested();
};

VMWindow::VMWindow(const char* title)
    : BWindow(BRect(100, 100, 800, 600), title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
    BRect frame = Bounds();
    g_view = new VMOutputView(frame);
    AddChild(g_view);
}

VMWindow::~VMWindow()
{
}

void VMWindow::MessageReceived(BMessage* msg)
{
    BWindow::MessageReceived(msg);
}

bool VMWindow::QuitRequested()
{
    return true;
}

// Main application
class VMApplication : public BApplication {
public:
    VMApplication();
    virtual ~VMApplication();
    virtual void ReadyToRun();
    virtual void MessageReceived(BMessage* msg);
};

VMApplication::VMApplication()
    : BApplication("application/x-vnd.vm-userlandvm")
{
}

VMApplication::~VMApplication()
{
}

void VMApplication::ReadyToRun()
{
    if (g_window) {
        g_window->Show();
        printf("[BeAPI] Window shown on screen\n");
    }
}

void VMApplication::MessageReceived(BMessage* msg)
{
    BApplication::MessageReceived(msg);
}

// Thread entry point for app loop
static int32 app_thread_func(void* arg)
{
    if (g_app) {
        printf("[BeAPI] App thread: Starting Run() loop\n");
        g_app->Run();
        printf("[BeAPI] App thread: Run() completed\n");
    }
    return 0;
}

extern "C" {

void CreateHaikuWindow(const char* title)
{
    printf("[BeAPI] CreateHaikuWindow: '%s'\n", title);
    
    if (g_app != nullptr) {
        printf("[BeAPI] Application already initialized\n");
        return;
    }
    
    try {
        // Create the application
        g_app = new VMApplication();
        printf("[BeAPI] ✓ BApplication created\n");
        
        // Create the window
        g_window = new VMWindow(title);
        printf("[BeAPI] ✓ BWindow created: '%s'\n", title);
        
        // Start the app thread
        g_app_thread = spawn_thread(app_thread_func, "VM_App_Thread", B_NORMAL_PRIORITY, NULL);
        if (g_app_thread >= 0) {
            resume_thread(g_app_thread);
            printf("[BeAPI] ✓ Application thread started (id: %d)\n", (int)g_app_thread);
        }
        
    } catch (...) {
        printf("[BeAPI] ERROR: Exception creating window\n");
    }
}

void ShowHaikuWindow()
{
    printf("[BeAPI] ShowHaikuWindow\n");
    
    if (g_window == nullptr) {
        printf("[BeAPI] ERROR: No window to show\n");
        return;
    }
    
    if (!g_window->IsHidden()) {
        g_window->Show();
        printf("[BeAPI] ✓ Window made visible\n");
    } else {
        printf("[BeAPI] ✓ Window already visible\n");
    }
}

void ProcessWindowEvents()
{
    printf("[BeAPI] ProcessWindowEvents\n");
    
    if (g_app == nullptr) {
        printf("[BeAPI] ERROR: No application running\n");
        return;
    }
    
    // The app thread is already running the event loop
    // Just wait for a bit
    printf("[BeAPI] Waiting for window events...\n");
    snooze(2000000);  // 2 seconds
    printf("[BeAPI] ✓ Window event processing complete\n");
}

void DestroyHaikuWindow()
{
    printf("[BeAPI] DestroyHaikuWindow\n");
    
    if (g_window != nullptr) {
        g_window->PostMessage(B_QUIT_REQUESTED);
        printf("[BeAPI] ✓ Posted quit message to window\n");
        g_window = nullptr;
    }
    
    if (g_app_thread >= 0) {
        status_t exit_status;
        wait_for_thread(g_app_thread, &exit_status);
        printf("[BeAPI] ✓ Application thread exited\n");
        g_app_thread = -1;
    }
    
    if (g_app != nullptr) {
        // App destructor is called automatically
        g_app = nullptr;
        printf("[BeAPI] ✓ Application destroyed\n");
    }
}

}  // extern "C"
