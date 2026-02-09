#include "RealGUIBackend.h"
#include <stdio.h>

// Constructor for HaikuRealWindow
RealGUIBackend::HaikuRealWindow::HaikuRealWindow(BRect frame, const char *title, 
                                                  window_type type, uint32 flags, uint32 workspace)
    : BWindow(frame, title, type, flags, workspace), backend(nullptr), window_id(0) {
    printf("[HaikuRealWindow] Created real Haiku window: '%s'\n", title);
}

RealGUIBackend::HaikuRealWindow::~HaikuRealWindow() {
    printf("[HaikuRealWindow] Destroyed real Haiku window\n");
}

void RealGUIBackend::HaikuRealWindow::SetBackend(RealGUIBackend *backend) {
    this->backend = backend;
}

void RealGUIBackend::HaikuRealWindow::SetWindowID(uint32_t window_id) {
    this->window_id = window_id;
}

void RealGUIBackend::HaikuRealWindow::MessageReceived(BMessage *message) {
    if (backend) {
        backend->ProcessBMessage(message, window_id);
    }
    BWindow::MessageReceived(message);
}

void RealGUIBackend::HaikuRealWindow::MouseDown(BPoint point) {
    if (backend) {
        backend->QueueEvent(window_id, MSG_MOUSE_DOWN, (uint32_t)point.x, (uint32_t)point.y, 
                            B_PRIMARY_MOUSE_BUTTON, nullptr);
    }
}

void RealGUIBackend::HaikuRealWindow::MouseUp(BPoint point) {
    if (backend) {
        backend->QueueEvent(window_id, MSG_MOUSE_UP, (uint32_t)point.x, (uint32_t)point.y, 
                            B_PRIMARY_MOUSE_BUTTON, nullptr);
    }
}

void RealGUIBackend::HaikuRealWindow::MouseMoved(BPoint point, uint32 transit, const BMessage *message) {
    if (backend) {
        backend->QueueEvent(window_id, MSG_MOUSE_MOVED, (uint32_t)point.x, (uint32_t)point.y, 
                            transit, const_cast<BMessage*>(message));
    }
}

void RealGUIBackend::HaikuRealWindow::KeyDown(const char *bytes, int32 numBytes) {
    if (backend) {
        backend->QueueEvent(window_id, MSG_KEY_DOWN, 0, 0, 
                            bytes[0], nullptr);
    }
}

void RealGUIBackend::HaikuRealWindow::KeyUp(const char *bytes, int32 numBytes) {
    if (backend) {
        backend->QueueEvent(window_id, MSG_KEY_UP, 0, 0, 
                            bytes[0], nullptr);
    }
}

void RealGUIBackend::HaikuRealWindow::WindowActivated(bool active) {
    if (backend) {
        backend->QueueEvent(window_id, active ? MSG_WINDOW_ACTIVATED : MSG_WINDOW_DEACTIVATED, 
                            0, 0, active ? 1 : 0, nullptr);
    }
}

void RealGUIBackend::HaikuRealWindow::FrameResized(float width, float height) {
    if (backend) {
        backend->QueueEvent(window_id, MSG_WINDOW_RESIZED, (uint32_t)width, (uint32_t)height, 
                            0, nullptr);
    }
}

// Constructor for HaikuRealView
RealGUIBackend::HaikuRealView::HaikuRealView(BRect frame, const char *name, 
                                               uint32 resizingMode, uint32 flags)
    : BView(frame, name, resizingMode, flags), backend(nullptr), window_id(0) {
    printf("[HaikuRealView] Created real Haiku view: '%s'\n", name);
}

RealGUIBackend::HaikuRealView::~HaikuRealView() {
    printf("[HaikuRealView] Destroyed real Haiku view\n");
}

void RealGUIBackend::HaikuRealView::SetBackend(RealGUIBackend *backend) {
    this->backend = backend;
}

void RealGUIBackend::HaikuRealView::SetWindowID(uint32_t window_id) {
    this->window_id = window_id;
}

void RealGUIBackend::HaikuRealView::Draw(BRect updateRect) {
    // Handle drawing updates
    if (backend) {
        // Queue draw event
        backend->QueueEvent(window_id, 8, (uint32_t)updateRect.left, (uint32_t)updateRect.top, 
                            0, nullptr);
    }
    BView::Draw(updateRect);
}

void RealGUIBackend::HaikuRealView::MouseDown(BPoint point) {
    if (backend) {
        backend->QueueEvent(window_id, MSG_MOUSE_DOWN, (uint32_t)point.x, (uint32_t)point.y, 
                            B_PRIMARY_MOUSE_BUTTON, nullptr);
    }
}

void RealGUIBackend::HaikuRealView::MouseUp(BPoint point) {
    if (backend) {
        backend->QueueEvent(window_id, MSG_MOUSE_UP, (uint32_t)point.x, (uint32_t)point.y, 
                            B_PRIMARY_MOUSE_BUTTON, nullptr);
    }
}

void RealGUIBackend::HaikuRealView::MouseMoved(BPoint point, uint32 transit, const BMessage *message) {
    if (backend) {
        backend->QueueEvent(window_id, MSG_MOUSE_MOVED, (uint32_t)point.x, (uint32_t)point.y, 
                            transit, const_cast<BMessage*>(message));
    }
}

// Main RealGUIBackend implementation
bool RealGUIBackend::Initialize() {
    printf("[RealGUI] Initializing REAL Haiku Be API Backend...\n");
    
    // Initialize Haiku application
    app = new BApplication("application/x-vnd.UserlandVM-GUI");
    if (!app) {
        printf("[RealGUI] ERROR: Failed to create BApplication\n");
        return false;
    }
    
    // Initialize screen information
    screen = new BScreen();
    if (!screen || screen->IsValid() == false) {
        printf("[RealGUI] ERROR: Failed to initialize BScreen\n");
        return false;
    }
    
    // Connect to Haiku app_server
    if (!ConnectToAppServer()) {
        printf("[RealGUI] ERROR: Failed to connect to Haiku app_server\n");
        return false;
    }
    
    printf("[RealGUI] Connected to Haiku app_server\n");
    printf("[RealGUI] Screen: %dx%d, Color depth: %d\n", 
           GetScreenWidth(), GetScreenHeight(), screen->ColorSpace());
    
    // Start event handling thread
    running = true;
    event_thread = std::make_unique<std::thread>(&RealGUIBackend::EventLoop, this);
    
    printf("[RealGUI] REAL Haiku Be API backend initialized successfully\n");
    return true;
}

void RealGUIBackend::Shutdown() {
    printf("[RealGUI] Shutting down REAL Haiku Be API Backend...\n");
    
    running = false;
    
    if (event_thread && event_thread->joinable()) {
        event_thread->join();
    }
    
    // Destroy all windows
    std::lock_guard<std::mutex> lock(window_mutex);
    for (auto& pair : windows) {
        DestroyHaikuWindow(pair.second.get());
    }
    windows.clear();
    
    // Disconnect from app_server
    DisconnectFromAppServer();
    
    if (screen) {
        delete screen;
        screen = nullptr;
    }
    
    if (app) {
        delete app;
        app = nullptr;
    }
    
    printf("[RealGUI] REAL Haiku Be API backend shutdown complete\n");
}

bool RealGUIBackend::CreateRealWindow(const char *title, uint32_t width, uint32_t height, 
                                     uint32_t x, uint32_t y, uint32_t *window_id) {
    std::lock_guard<std::mutex> lock(window_mutex);
    
    auto window = std::make_unique<RealWindow>();
    window->haiku_window_id = next_window_id++;
    window->width = width;
    window->height = height;
    window->x = x;
    window->y = y;
    window->visible = false;
    window->focused = false;
    window->drawing_active = false;
    strncpy(window->title, title ? title : "Untitled", sizeof(window->title) - 1);
    window->title[sizeof(window->title) - 1] = '\0';
    window->bg_color = {255, 255, 255, 255}; // White
    window->fg_color = {0, 0, 0, 255}; // Black
    
    if (!CreateHaikuWindow(window.get(), title)) {
        printf("[RealGUI] ERROR: Failed to create Haiku window\n");
        return false;
    }
    
    *window_id = window->haiku_window_id;
    windows[*window_id] = std::move(window);
    
    printf("[RealGUI] Created REAL Haiku window %d: '%s' (%dx%d at %d,%d)\n",
           *window_id, title, width, height, x, y);
    
    return true;
}

bool RealGUIBackend::CreateHaikuWindow(RealWindow *window, const char *title) {
    // Create BRect for window frame
    BRect frame(window->x, window->y, window->x + window->width - 1, window->y + window->height - 1);
    
    // Create REAL Haiku BWindow
    window->be_window = new HaikuRealWindow(frame, title, B_TITLED_WINDOW, 
                                          B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE, 
                                          B_CURRENT_WORKSPACE);
    
    if (!window->be_window) {
        printf("[RealGUI] ERROR: Failed to create BWindow\n");
        return false;
    }
    
    // Set backend reference in the custom window
    HaikuRealWindow *custom_window = dynamic_cast<HaikuRealWindow*>(window->be_window);
    if (custom_window) {
        custom_window->SetBackend(this);
        custom_window->SetWindowID(window->haiku_window_id);
    }
    
    // Create BView for drawing
    BRect view_frame(0, 0, window->width - 1, window->height - 1);
    window->be_view = new HaikuRealView(view_frame, "MainView", B_FOLLOW_ALL, B_WILL_DRAW);
    
    if (!window->be_view) {
        printf("[RealGUI] ERROR: Failed to create BView\n");
        delete window->be_window;
        window->be_window = nullptr;
        return false;
    }
    
    // Set view properties
    window->be_view->SetViewColor(window->bg_color);
    window->be_view->SetLowColor(window->bg_color);
    window->be_view->SetHighColor(window->fg_color);
    
    // Set backend reference in the view
    HaikuRealView *custom_view = dynamic_cast<HaikuRealView*>(window->be_view);
    if (custom_view) {
        custom_view->SetBackend(this);
        custom_view->SetWindowID(window->haiku_window_id);
    }
    
    // Add view to window
    window->be_window->AddChild(window->be_view);
    
    // Create drawing lock
    window->draw_lock = new BLocker("RealWindowDrawingLock");
    
    // Create offscreen bitmap for double buffering
    BRect bitmap_frame(0, 0, window->width - 1, window->height - 1);
    window->bitmap = new BBitmap(bitmap_frame, B_RGB32, true);
    
    printf("[RealGUI] Created REAL Haiku BWindow and BView\n");
    return true;
}

void RealGUIBackend::DestroyHaikuWindow(RealWindow *window) {
    if (!window) return;
    
    printf("[RealGUI] Destroying REAL Haiku window %d\n", window->haiku_window_id);
    
    if (window->bitmap) {
        delete window->bitmap;
        window->bitmap = nullptr;
    }
    
    if (window->draw_lock) {
        delete window->draw_lock;
        window->draw_lock = nullptr;
    }
    
    if (window->be_view) {
        window->be_window->RemoveChild(window->be_view);
        delete window->be_view;
        window->be_view = nullptr;
    }
    
    if (window->be_window) {
        window->be_window->Lock();
        window->be_window->Quit();
        window->be_window = nullptr;
    }
}

RealGUIBackend::RealWindow* RealGUIBackend::GetWindow(uint32_t window_id) {
    std::lock_guard<std::mutex> lock(window_mutex);
    auto it = windows.find(window_id);
    return it != windows.end() ? it->second.get() : nullptr;
}

bool RealGUIBackend::ShowWindow(uint32_t window_id) {
    RealWindow *window = GetWindow(window_id);
    if (!window || !window->be_window) return false;
    
    window->be_window->Show();
    window->visible = true;
    
    printf("[RealGUI] Showed REAL Haiku window %d\n", window_id);
    return true;
}

bool RealGUIBackend::HideWindow(uint32_t window_id) {
    RealWindow *window = GetWindow(window_id);
    if (!window || !window->be_window) return false;
    
    window->be_window->Hide();
    window->visible = false;
    
    printf("[RealGUI] Hid REAL Haiku window %d\n", window_id);
    return true;
}

bool RealGUIBackend::DestroyRealWindow(uint32_t window_id) {
    std::lock_guard<std::mutex> lock(window_mutex);
    
    auto it = windows.find(window_id);
    if (it == windows.end()) return false;
    
    printf("[RealGUI] Destroying REAL Haiku window %d\n", window_id);
    
    DestroyHaikuWindow(it->second.get());
    windows.erase(it);
    
    return true;
}

uint32_t RealGUIBackend::GetScreenWidth() const {
    if (!screen || !screen->IsValid()) return 1024;
    BRect frame = screen->Frame();
    return (uint32_t)frame.Width();
}

uint32_t RealGUIBackend::GetScreenHeight() const {
    if (!screen || !screen->IsValid()) return 768;
    BRect frame = screen->Frame();
    return (uint32_t)frame.Height();
}

bool RealGUIBackend::SupportsTrueColor() const {
    if (!screen || !screen->IsValid()) return true;
    color_space cs = screen->ColorSpace();
    return (cs == B_RGB32 || cs == B_RGB24 || cs == B_RGBA32 || cs == B_RGBA64);
}

bool RealGUIBackend::ConnectToAppServer() {
    return InitAppServerConnection();
}

bool RealGUIBackend::DisconnectFromAppServer() {
    CleanupAppServerConnection();
    return true;
}

status_t RealGUIBackend::InitAppServerConnection() {
    // Haiku app_server connection is automatically handled by BApplication
    // We just need to verify the connection is working
    app_server_connected = true;
    app_server_port = find_port("application/x-vnd.Haiku-app_server");
    
    if (app_server_port < B_OK) {
        printf("[RealGUI] Warning: Could not find app_server port\n");
        app_server_connected = false;
        return B_ERROR;
    }
    
    // Create reply port for app_server communication
    reply_port = create_port(10, "UserlandVM_GUI_Reply");
    if (reply_port < B_OK) {
        printf("[RealGUI] ERROR: Failed to create reply port\n");
        app_server_connected = false;
        return B_ERROR;
    }
    
    printf("[RealGUI] Connected to Haiku app_server (port: %d)\n", app_server_port);
    return B_OK;
}

void RealGUIBackend::CleanupAppServerConnection() {
    if (reply_port >= B_OK) {
        delete_port(reply_port);
        reply_port = -1;
    }
    app_server_port = -1;
    app_server_connected = false;
}

bool RealGUIBackend::IsAppServerConnected() const {
    return app_server_connected;
}

void RealGUIBackend::EventLoop() {
    printf("[RealGUI] Starting REAL Haiku event loop...\n");
    
    // Run Haiku application message loop
    app->Run();
    
    printf("[RealGUI] REAL Haiku event loop ended\n");
}

void RealGUIBackend::ProcessBMessage(BMessage *message, uint32_t window_id) {
    if (!message) return;
    
    uint32_t what = message->what;
    BPoint point;
    int32 key;
    
    switch (what) {
        case B_MOUSE_DOWN:
            if (message->FindPoint("where", &point) == B_OK) {
                QueueEvent(window_id, MSG_MOUSE_DOWN, (uint32_t)point.x, (uint32_t)point.y, 
                         B_PRIMARY_MOUSE_BUTTON, message);
            }
            break;
            
        case B_MOUSE_UP:
            if (message->FindPoint("where", &point) == B_OK) {
                QueueEvent(window_id, MSG_MOUSE_UP, (uint32_t)point.x, (uint32_t)point.y, 
                         B_PRIMARY_MOUSE_BUTTON, message);
            }
            break;
            
        case B_MOUSE_MOVED:
            if (message->FindPoint("where", &point) == B_OK) {
                QueueEvent(window_id, MSG_MOUSE_MOVED, (uint32_t)point.x, (uint32_t)point.y, 
                         0, message);
            }
            break;
            
        case B_KEY_DOWN:
            if (message->FindInt32("key", &key) == B_OK) {
                QueueEvent(window_id, MSG_KEY_DOWN, 0, 0, key, message);
            }
            break;
            
        case B_KEY_UP:
            if (message->FindInt32("key", &key) == B_OK) {
                QueueEvent(window_id, MSG_KEY_UP, 0, 0, key, message);
            }
            break;
    }
}

void RealGUIBackend::QueueEvent(uint32_t window_id, uint32_t event_type, uint32_t x, uint32_t y, 
                               uint32_t data, BMessage *msg) {
    std::lock_guard<std::mutex> lock(event_mutex);
    
    GUIEvent event;
    event.window_id = window_id;
    event.event_type = event_type;
    event.x = x;
    event.y = y;
    event.data = data;
    event.original_message = msg;
    
    event_queue.push_back(event);
}

bool RealGUIBackend::PollEvents() {
    std::lock_guard<std::mutex> lock(event_mutex);
    return !event_queue.empty();
}

bool RealGUIBackend::GetNextEvent(uint32_t *window_id, uint32_t *event_type, uint32_t *x, uint32_t *y, uint32_t *data) {
    std::lock_guard<std::mutex> lock(event_mutex);
    
    if (event_queue.empty()) return false;
    
    GUIEvent event = event_queue.front();
    event_queue.erase(event_queue.begin());
    
    *window_id = event.window_id;
    *event_type = event.event_type;
    *x = event.x;
    *y = event.y;
    *data = event.data;
    
    return true;
}

// Color conversion utilities
rgb_color RealGUIBackend::ColorToRGB(uint32_t color) {
    rgb_color rgb;
    rgb.red = (color >> 16) & 0xFF;
    rgb.green = (color >> 8) & 0xFF;
    rgb.blue = color & 0xFF;
    rgb.alpha = (color >> 24) & 0xFF;
    return rgb;
}

uint32_t RealGUIBackend::RGBToColor(rgb_color rgb) {
    return ((uint32_t)rgb.red << 16) | ((uint32_t)rgb.green << 8) | rgb.blue | ((uint32_t)rgb.alpha << 24);
}

BRect RealGUIBackend::MakeRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    return BRect(x, y, x + w - 1, y + h - 1);
}

BPoint RealGUIBackend::MakePoint(uint32_t x, uint32_t y) {
    return BPoint(x, y);
}

// Graphics implementations (stubs for now - will implement real drawing)
bool RealGUIBackend::BeginPaint(uint32_t window_id, BView **view) {
    RealWindow *window = GetWindow(window_id);
    if (!window || !window->be_window || !window->be_view) return false;
    
    if (window->be_window->Lock()) {
        window->drawing_active = true;
        window->current_drawing_view = window->be_view;
        *view = window->be_view;
        return true;
    }
    
    return false;
}

bool RealGUIBackend::EndPaint(uint32_t window_id) {
    RealWindow *window = GetWindow(window_id);
    if (!window || !window->be_window) return false;
    
    if (window->drawing_active) {
        window->be_window->Unlock();
        window->drawing_active = false;
        window->current_drawing_view = nullptr;
        return true;
    }
    
    return false;
}

bool RealGUIBackend::ClearWindow(uint32_t window_id, rgb_color color) {
    BView *view;
    if (!BeginPaint(window_id, &view)) return false;
    
    view->SetViewColor(color);
    view->SetLowColor(color);
    BRect bounds = view->Bounds();
    view->FillRect(bounds, B_SOLID_LOW);
    
    EndPaint(window_id);
    return true;
}

bool RealGUIBackend::DrawLine(uint32_t window_id, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, rgb_color color) {
    BView *view;
    if (!BeginPaint(window_id, &view)) return false;
    
    view->SetHighColor(color);
    view->StrokeLine(MakePoint(x1, y1), MakePoint(x2, y2));
    
    EndPaint(window_id);
    return true;
}

bool RealGUIBackend::DrawRect(uint32_t window_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h, rgb_color color) {
    BView *view;
    if (!BeginPaint(window_id, &view)) return false;
    
    view->SetHighColor(color);
    view->StrokeRect(MakeRect(x, y, w, h));
    
    EndPaint(window_id);
    return true;
}

bool RealGUIBackend::FillRect(uint32_t window_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h, rgb_color color) {
    BView *view;
    if (!BeginPaint(window_id, &view)) return false;
    
    view->SetHighColor(color);
    view->FillRect(MakeRect(x, y, w, h), B_SOLID_HIGH);
    
    EndPaint(window_id);
    return true;
}

bool RealGUIBackend::DrawText(uint32_t window_id, uint32_t x, uint32_t y, const char *text, rgb_color color) {
    BView *view;
    if (!BeginPaint(window_id, &view)) return false;
    
    view->SetHighColor(color);
    view->DrawString(text, MakePoint(x, y));
    
    EndPaint(window_id);
    return true;
}

status_t RealGUIBackend::SendMessageToAppServer(BMessage *message) {
    if (!app_server_connected || !message) return B_ERROR;
    
    status_t result = write_port(app_server_port, 0, message, sizeof(BMessage));
    return (result >= B_OK) ? B_OK : B_ERROR;
}

status_t RealGUIBackend::ReceiveMessageFromAppServer(BMessage **message) {
    if (!app_server_connected || !message) return B_ERROR;
    
    char buffer[sizeof(BMessage)];
    int32 code;
    ssize_t size = read_port(reply_port, &code, buffer, sizeof(buffer));
    
    if (size < B_OK) return B_ERROR;
    
    *message = new BMessage(*reinterpret_cast<BMessage*>(buffer));
    return B_OK;
}