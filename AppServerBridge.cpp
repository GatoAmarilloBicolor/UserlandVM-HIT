#include "AppServerBridge.h"
#include "include/HaikuLogging.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

///////////////////////////////////////////////////////////////////////////
// AppServerBridge Implementation
///////////////////////////////////////////////////////////////////////////

AppServerBridge* AppServerBridge::instance = nullptr;

AppServerBridge& AppServerBridge::GetInstance()
{
    if (!instance) {
        instance = new AppServerBridge();
    }
    return *instance;
}

AppServerBridge::AppServerBridge()
    : connected(false),
      total_windows(0),
      total_applications(0),
      app_server_connection(nullptr),
      app_server_port(0)
{
    HAIKU_LOG_BEAPI("Initializing AppServer Bridge");
}

AppServerBridge::~AppServerBridge()
{
    Shutdown();
}

bool AppServerBridge::Initialize()
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    if (connected) {
        HAIKU_LOG_BEAPI_WARN("AppServer Bridge already initialized");
        return true;
    }
    
    HAIKU_LOG_BEAPI("Connecting to Haiku app_server...");
    
    if (!ConnectToAppServer()) {
        HAIKU_LOG_BEAPI_WARN("Failed to connect to app_server, using simulation mode");
        // Continue in simulation mode for development/testing
    }
    
    connected = true;
    HAIKU_LOG_BEAPI("AppServer Bridge initialized successfully");
    
    return true;
}

void AppServerBridge::Shutdown()
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    if (!connected) {
        return;
    }
    
    HAIKU_LOG_BEAPI("Shutting down AppServer Bridge");
    
    // Close all windows
    for (auto& pair : windows) {
        if (pair.second.native_window) {
            // Close native window
        }
    }
    
    // Clear registries
    windows.clear();
    registered_applications.clear();
    
    // Disconnect from app_server
    DisconnectFromAppServer();
    
    connected = false;
    HAIKU_LOG_BEAPI("AppServer Bridge shutdown complete");
}

bool AppServerBridge::RegisterApplication(haiku_id_t app_id, haiku_const_string_t app_name)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    if (!connected) {
        HAIKU_LOG_BEAPI_ERROR("Bridge not initialized");
        return false;
    }
    
    HAIKU_LOG_BEAPI("Registering application: #%u (%s)", app_id, app_name);
    
    registered_applications[app_id] = app_name;
    total_applications++;
    
    return true;
}

bool AppServerBridge::UnregisterApplication(haiku_id_t app_id)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    auto it = registered_applications.find(app_id);
    if (it == registered_applications.end()) {
        HAIKU_LOG_BEAPI_WARN("Application not registered: #%u", app_id);
        return false;
    }
    
    HAIKU_LOG_BEAPI("Unregistering application: #%u (%s)", app_id, it->second.c_str());
    
    // Close all windows for this application
    auto window_it = windows.begin();
    while (window_it != windows.end()) {
        if (window_it->second.app_id == app_id) {
            window_it = windows.erase(window_it);
        } else {
            ++window_it;
        }
    }
    
    registered_applications.erase(it);
    total_applications--;
    
    return true;
}

haiku_id_t AppServerBridge::CreateWindow(haiku_id_t app_id, haiku_const_string_t title,
                                         haiku_param_t x, haiku_param_t y, haiku_param_t w, haiku_param_t h)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    if (!ValidateWindow(app_id, 0)) {
        HAIKU_LOG_BEAPI_ERROR("Invalid application ID: #%u", app_id);
        return 0;
    }
    
    haiku_id_t window_id = GenerateWindowID();
    
    HAIKU_LOG_BEAPI("Creating window: app=%u window=%u title='%s' pos=(%u,%u) size=%ux%u",
           app_id, window_id, title, x, y, w, h);
    
    WindowInfo window_info;
    window_info.app_id = app_id;
    window_info.window_id = window_id;
    window_info.title = title;
    window_info.x = x;
    window_info.y = y;
    window_info.w = w;
    window_info.h = h;
    window_info.visible = false;
    window_info.focused = false;
    window_info.native_window = nullptr;
    
    // In a real implementation, this would create a native Haiku window
    // For now, we simulate it
    if (app_server_connection) {
        // Send create window message to app_server
        // This would be implemented with real BWindow creation
    }
    
    windows[{app_id, window_id}] = window_info;
    total_windows++;
    
    HAIKU_LOG_BEAPI("Window created successfully: app=%u window=%u", app_id, window_id);
    
    return window_id;
}

bool AppServerBridge::DestroyWindow(haiku_id_t app_id, haiku_id_t window_id)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    WindowInfo* window = GetWindowInfo(app_id, window_id);
    if (!window) {
        HAIKU_LOG_BEAPI_ERROR("Window not found: app=%u window=%u", app_id, window_id);
        return false;
    }
    
    HAIKU_LOG_BEAPI("Destroying window: app=%u window=%u", app_id, window_id);
    
    // In a real implementation, this would destroy the native Haiku window
    if (window->native_window) {
        // Send destroy window message to app_server
    }
    
    windows.erase({app_id, window_id});
    total_windows--;
    
    return true;
}

bool AppServerBridge::ShowWindow(haiku_id_t app_id, haiku_id_t window_id)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    WindowInfo* window = GetWindowInfo(app_id, window_id);
    if (!window) {
        HAIKU_LOG_BEAPI_ERROR("Window not found: app=%u window=%u", app_id, window_id);
        return false;
    }
    
    HAIKU_LOG_BEAPI("Showing window: app=%u window=%u", app_id, window_id);
    
    window->visible = true;
    
    // In a real implementation, this would show the native Haiku window
    if (window->native_window) {
        // Send show window message to app_server
    }
    
    return true;
}

bool AppServerBridge::HideWindow(haiku_id_t app_id, haiku_id_t window_id)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    WindowInfo* window = GetWindowInfo(app_id, window_id);
    if (!window) {
        HAIKU_LOG_BEAPI_ERROR("Window not found: app=%u window=%u", app_id, window_id);
        return false;
    }
    
    HAIKU_LOG_BEAPI("Hiding window: app=%u window=%u", app_id, window_id);
    
    window->visible = false;
    
    // In a real implementation, this would hide the native Haiku window
    if (window->native_window) {
        // Send hide window message to app_server
    }
    
    return true;
}

bool AppServerBridge::SetWindowFrame(haiku_id_t app_id, haiku_id_t window_id,
                                    haiku_param_t x, haiku_param_t y, haiku_param_t w, haiku_param_t h)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    WindowInfo* window = GetWindowInfo(app_id, window_id);
    if (!window) {
        HAIKU_LOG_BEAPI_ERROR("Window not found: app=%u window=%u", app_id, window_id);
        return false;
    }
    
    HAIKU_LOG_BEAPI("Setting window frame: app=%u window=%u pos=(%u,%u) size=%ux%u",
           app_id, window_id, x, y, w, h);
    
    window->x = x;
    window->y = y;
    window->w = w;
    window->h = h;
    
    // In a real implementation, this would resize/move the native Haiku window
    if (window->native_window) {
        // Send set frame message to app_server
    }
    
    return true;
}

bool AppServerBridge::GetWindowFrame(haiku_id_t app_id, haiku_id_t window_id,
                                    haiku_param_t* x, haiku_param_t* y, haiku_param_t* w, haiku_param_t* h)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    const WindowInfo* window = GetWindowInfo(app_id, window_id);
    if (!window) {
        HAIKU_LOG_BEAPI_ERROR("Window not found: app=%u window=%u", app_id, window_id);
        return false;
    }
    
    if (x) *x = window->x;
    if (y) *y = window->y;
    if (w) *w = window->w;
    if (h) *h = window->h;
    
    return true;
}

bool AppServerBridge::ActivateWindow(haiku_id_t app_id, haiku_id_t window_id)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    WindowInfo* window = GetWindowInfo(app_id, window_id);
    if (!window) {
        HAIKU_LOG_BEAPI_ERROR("Window not found: app=%u window=%u", app_id, window_id);
        return false;
    }
    
    HAIKU_LOG_BEAPI("Activating window: app=%u window=%u", app_id, window_id);
    
    // In a real implementation, this would activate the native Haiku window
    if (window->native_window) {
        // Send activate window message to app_server
    }
    
    return true;
}

haiku_id_t AppServerBridge::GetFocusedWindow() const
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    // In a real implementation, this would query app_server for the focused window
    for (const auto& pair : windows) {
        if (pair.second.focused) {
            return pair.second.window_id;
        }
    }
    
    return 0;
}

bool AppServerBridge::InvalidateWindow(haiku_id_t app_id, haiku_id_t window_id,
                                       haiku_param_t x, haiku_param_t y, haiku_param_t w, haiku_param_t h)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    WindowInfo* window = GetWindowInfo(app_id, window_id);
    if (!window) {
        HAIKU_LOG_BEAPI_ERROR("Window not found: app=%u window=%u", app_id, window_id);
        return false;
    }
    
    // In a real implementation, this would invalidate the native Haiku window
    if (window->native_window) {
        // Send invalidate message to app_server
    }
    
    return true;
}

bool AppServerBridge::FlushWindow(haiku_id_t app_id, haiku_id_t window_id)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    WindowInfo* window = GetWindowInfo(app_id, window_id);
    if (!window) {
        HAIKU_LOG_BEAPI_ERROR("Window not found: app=%u window=%u", app_id, window_id);
        return false;
    }
    
    // In a real implementation, this would flush the native Haiku window
    if (window->native_window) {
        // Send flush message to app_server
    }
    
    return true;
}

bool AppServerBridge::GetNextEvent(AppServerEvent* event)
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    if (!event || event_queue.empty()) {
        return false;
    }
    
    *event = event_queue.front();
    event_queue.pop();
    
    return true;
}

bool AppServerBridge::HasPendingEvents() const
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    return !event_queue.empty();
}

bool AppServerBridge::GetScreenInfo(ScreenInfo* info) const
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    if (!info) {
        return false;
    }
    
    // In a real implementation, this would query app_server for screen info
    // For now, we provide default values
    info->width = 1024;
    info->height = 768;
    info->color_depth = 32;
    info->refresh_rate = 60;
    
    return true;
}

void AppServerBridge::PrintStatus() const
{
    std::lock_guard<std::mutex> lock(bridge_mutex);
    
    HAIKU_LOG_BEAPI("=================================================");
    HAIKU_LOG_BEAPI("         APPSERVER BRIDGE STATUS");
    HAIKU_LOG_BEAPI("=================================================");
    HAIKU_LOG_BEAPI("Connection Status: %s", connected ? "[OK] Connected" : "[FAIL] Disconnected");
    HAIKU_LOG_BEAPI("Total Applications: %u", total_applications);
    HAIKU_LOG_BEAPI("Total Windows: %u", total_windows);
    HAIKU_LOG_BEAPI("Pending Events: %zu", event_queue.size());
    
    if (!registered_applications.empty()) {
        HAIKU_LOG_BEAPI("Registered Applications:");
        for (const auto& pair : registered_applications) {
            HAIKU_LOG_BEAPI("  #%u: %s", pair.first, pair.second.c_str());
        }
    }
    
    HAIKU_LOG_BEAPI("=================================================");
}

///////////////////////////////////////////////////////////////////////////
// Private Methods
///////////////////////////////////////////////////////////////////////////

bool AppServerBridge::ConnectToAppServer()
{
    // In a real implementation, this would connect to Haiku's app_server
    // For now, we simulate the connection
    
    // Try to find app_server port
    const char* display = getenv("DISPLAY");
    if (!display) {
        display = "/tmp/app_server_socket";
    }
    
    // Create Unix domain socket connection
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        HAIKU_LOG_BEAPI_WARN("Failed to create socket for app_server connection");
        return false;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, display, sizeof(addr.sun_path) - 1);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        HAIKU_LOG_BEAPI_WARN("Failed to connect to app_server socket");
        return false;
    }
    
    app_server_connection = (void*)(intptr_t)sock;
    app_server_port = 1; // Simulated port
    
    HAIKU_LOG_BEAPI("Connected to app_server successfully");
    return true;
}

void AppServerBridge::DisconnectFromAppServer()
{
    if (app_server_connection) {
        int sock = (int)(intptr_t)app_server_connection;
        close(sock);
        app_server_connection = nullptr;
    }
    
    app_server_port = 0;
}

haiku_id_t AppServerBridge::GenerateWindowID()
{
    static haiku_id_t next_id = 1000;
    return next_id++;
}

bool AppServerBridge::ValidateWindow(haiku_id_t app_id, haiku_id_t window_id) const
{
    if (app_id == 0) {
        return false;
    }
    
    if (window_id == 0) {
        // Only checking app_id
        return registered_applications.find(app_id) != registered_applications.end();
    }
    
    return windows.find({app_id, window_id}) != windows.end();
}

AppServerBridge::WindowInfo* AppServerBridge::GetWindowInfo(haiku_id_t app_id, haiku_id_t window_id)
{
    auto it = windows.find({app_id, window_id});
    return (it != windows.end()) ? &it->second : nullptr;
}

const AppServerBridge::WindowInfo* AppServerBridge::GetWindowInfo(haiku_id_t app_id, haiku_id_t window_id) const
{
    auto it = windows.find({app_id, window_id});
    return (it != windows.end()) ? &it->second : nullptr;
}

void AppServerBridge::QueueEvent(const AppServerEvent& event)
{
    event_queue.push(event);
}

bool AppServerBridge::ProcessAppServerEvents()
{
    // In a real implementation, this would process events from app_server
    // For now, we simulate basic events
    
    return true;
}