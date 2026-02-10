#ifndef APPSERVER_BRIDGE_H
#define APPSERVER_BRIDGE_H

#include "include/PlatformTypes.h"
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

///////////////////////////////////////////////////////////////////////////
// AppServer Bridge - Direct communication with Haiku's app_server
// Provides proper integration with Haiku's window management system
///////////////////////////////////////////////////////////////////////////

class AppServerBridge {
public:
    static AppServerBridge& GetInstance();
    
    // Lifecycle management
    bool Initialize();
    void Shutdown();
    bool IsConnected() const { return connected; }
    
    // Application management
    bool RegisterApplication(haiku_id_t app_id, haiku_const_string_t app_name);
    bool UnregisterApplication(haiku_id_t app_id);
    
    // Window management
    haiku_id_t CreateWindow(haiku_id_t app_id, haiku_const_string_t title,
                           haiku_param_t x, haiku_param_t y, haiku_param_t w, haiku_param_t h);
    bool DestroyWindow(haiku_id_t app_id, haiku_id_t window_id);
    bool ShowWindow(haiku_id_t app_id, haiku_id_t window_id);
    bool HideWindow(haiku_id_t app_id, haiku_id_t window_id);
    
    // Window properties
    bool SetWindowFrame(haiku_id_t app_id, haiku_id_t window_id,
                      haiku_param_t x, haiku_param_t y, haiku_param_t w, haiku_param_t h);
    bool GetWindowFrame(haiku_id_t app_id, haiku_id_t window_id,
                       haiku_param_t* x, haiku_param_t* y, haiku_param_t* w, haiku_param_t* h);
    
    // Focus management
    bool ActivateWindow(haiku_id_t app_id, haiku_id_t window_id);
    haiku_id_t GetFocusedWindow() const;
    
    // Drawing operations
    bool InvalidateWindow(haiku_id_t app_id, haiku_id_t window_id,
                         haiku_param_t x, haiku_param_t y, haiku_param_t w, haiku_param_t h);
    bool FlushWindow(haiku_id_t app_id, haiku_id_t window_id);
    
    // Event handling
    struct AppServerEvent {
        enum Type {
            WINDOW_ACTIVATED,
            WINDOW_DEACTIVATED,
            WINDOW_MOVED,
            WINDOW_RESIZED,
            WINDOW_CLOSED,
            KEY_DOWN,
            KEY_UP,
            MOUSE_DOWN,
            MOUSE_UP,
            MOUSE_MOVED
        };
        
        Type type;
        haiku_id_t app_id;
        haiku_id_t window_id;
        haiku_param_t x, y;
        haiku_param_t w, h;
        haiku_param_t key_code;
        haiku_param_t buttons;
        uint64_t timestamp;
    };
    
    bool GetNextEvent(AppServerEvent* event);
    bool HasPendingEvents() const;
    
    // Screen information
    struct ScreenInfo {
        haiku_param_t width;
        haiku_param_t height;
        haiku_param_t color_depth;
        haiku_param_t refresh_rate;
    };
    
    bool GetScreenInfo(ScreenInfo* info) const;
    
    // Status and diagnostics
    void PrintStatus() const;
    haiku_id_t GetWindowCount() const { return total_windows; }
    haiku_id_t GetApplicationCount() const { return total_applications; }

private:
    AppServerBridge();
    ~AppServerBridge();
    
    // Singleton
    static AppServerBridge* instance;
    
    // Connection state
    bool connected;
    haiku_id_t total_windows;
    haiku_id_t total_applications;
    
    // Real app_server communication
    void* app_server_connection;
    haiku_id_t app_server_port;
    
    // Application registry
    std::map<haiku_id_t, std::string> registered_applications;
    
    // Window registry
    struct WindowInfo {
        haiku_id_t app_id;
        haiku_id_t window_id;
        std::string title;
        haiku_param_t x, y, w, h;
        bool visible;
        bool focused;
        void* native_window;
    };
    
    std::map<std::pair<haiku_id_t, haiku_id_t>, WindowInfo> windows;
    
    // Event queue
    std::queue<AppServerEvent> event_queue;
    mutable std::mutex bridge_mutex;
    
    // Private methods
    bool ConnectToAppServer();
    void DisconnectFromAppServer();
    bool SendAppServerMessage(const void* message, size_t size);
    bool ReceiveAppServerMessage(void* buffer, size_t size, size_t* received);
    
    // Window management helpers
    haiku_id_t GenerateWindowID();
    bool ValidateWindow(haiku_id_t app_id, haiku_id_t window_id) const;
    WindowInfo* GetWindowInfo(haiku_id_t app_id, haiku_id_t window_id);
    const WindowInfo* GetWindowInfo(haiku_id_t app_id, haiku_id_t window_id) const;
    
    // Event handling helpers
    void QueueEvent(const AppServerEvent& event);
    bool ProcessAppServerEvents();
    
    // Prevent copying
    AppServerBridge(const AppServerBridge&) = delete;
    AppServerBridge& operator=(const AppServerBridge&) = delete;
};

///////////////////////////////////////////////////////////////////////////
// Convenience Macros
///////////////////////////////////////////////////////////////////////////

#define APPSERVER_BRIDGE AppServerBridge::GetInstance()

#define APPSERVER_CREATE_WINDOW(app_id, title, x, y, w, h) \
    APPSERVER_BRIDGE.CreateWindow(app_id, title, x, y, w, h)

#define APPSERVER_DESTROY_WINDOW(app_id, window_id) \
    APPSERVER_BRIDGE.DestroyWindow(app_id, window_id)

#define APPSERVER_SHOW_WINDOW(app_id, window_id) \
    APPSERVER_BRIDGE.ShowWindow(app_id, window_id)

#define APPSERVER_HIDE_WINDOW(app_id, window_id) \
    APPSERVER_BRIDGE.HideWindow(app_id, window_id)

#endif // APPSERVER_BRIDGE_H