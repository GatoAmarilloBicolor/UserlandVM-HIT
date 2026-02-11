#include "HaikuGUIBackend.h"
#include <os/interface/Rect.h>
#include <cstring>

static VMApplication* g_app = nullptr;
static VMApplicationWindow* g_window = nullptr;

// ==================== VMApplicationWindow ====================

VMApplicationWindow::VMApplicationWindow(const char* title) 
    : BWindow(BRect(100, 100, 800, 600), title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
    view = new BView(Bounds(), "VMView", B_FOLLOW_ALL, B_WILL_DRAW);
    view->SetViewColor(216, 216, 216);
    AddChild(view);
}

VMApplicationWindow::~VMApplicationWindow() {
}

void VMApplicationWindow::MessageReceived(BMessage* msg) {
    BWindow::MessageReceived(msg);
}

bool VMApplicationWindow::QuitRequested() {
    return true;
}

// ==================== VMApplication ====================

VMApplication::VMApplication(const char* app_name)
    : BApplication(app_name)
{
    main_window = nullptr;
}

VMApplication::~VMApplication() {
}

void VMApplication::ReadyToRun() {
    if (main_window) {
        main_window->Show();
    }
}

// ==================== Helper Functions ====================

void CreateHaikuWindow(const char* title) {
    if (!g_app) {
        std::cout << "[GUI] Creating BApplication..." << std::endl;
        g_app = new VMApplication("application/x-vnd.vm-executor");
    }
    
    if (!g_window) {
        std::cout << "[GUI] Creating window: " << title << std::endl;
        g_window = new VMApplicationWindow(title);
    }
}

void ShowHaikuWindow() {
    if (g_window && !g_window->IsHidden()) {
        std::cout << "[GUI] Showing window..." << std::endl;
        g_window->Show();
    }
}

void ProcessWindowEvents() {
    if (g_app) {
        std::cout << "[GUI] Processing window events..." << std::endl;
        g_app->Run();
    }
}

void DestroyHaikuWindow() {
    if (g_window) {
        g_window->PostMessage(B_QUIT_REQUESTED);
        g_window = nullptr;
    }
    if (g_app) {
        g_app->PostMessage(B_QUIT_REQUESTED);
        g_app = nullptr;
    }
}
