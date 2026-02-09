#pragma once

#include "Phase4GUISyscalls.h"
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <stdint.h>
#include <string.h>

// ONLY Haiku OS Be API - No X11/Wayland
// Real Haiku Be API headers for true window creation
#include <InterfaceKit.h>
#include <AppKit.h>
#include <StorageKit.h>
#include <support/SupportDefs.h>
#include <kernel/OS.h>

// Real GUI backend using TRUE Haiku Be API
// Bridges UserlandVM BWindow to REAL Haiku BWindow instances

class RealGUIBackend {
public:
    RealGUIBackend() : app(nullptr), running(false), event_thread(nullptr), next_window_id(1) {
        printf("[RealGUI] Initializing REAL Haiku Be API backend...\n");
    }
    
    ~RealGUIBackend() {
        Shutdown();
    }
    
    bool Initialize();
    void Shutdown();
    
    // REAL window creation using Haiku BWindow
    bool CreateRealWindow(const char *title, uint32_t width, uint32_t height, 
                         uint32_t x, uint32_t y, uint32_t *window_id);
    bool DestroyRealWindow(uint32_t window_id);
    bool ShowWindow(uint32_t window_id);
    bool HideWindow(uint32_t window_id);
    bool ResizeWindow(uint32_t window_id, uint32_t width, uint32_t height);
    bool MoveWindow(uint32_t window_id, uint32_t x, uint32_t y);
    bool FocusWindow(uint32_t window_id);
    
    // REAL graphics using Haiku BView and drawing functions
    bool BeginPaint(uint32_t window_id, BView **view);
    bool EndPaint(uint32_t window_id);
    bool ClearWindow(uint32_t window_id, rgb_color color);
    bool DrawLine(uint32_t window_id, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, rgb_color color);
    bool DrawRect(uint32_t window_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h, rgb_color color);
    bool FillRect(uint32_t window_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h, rgb_color color);
    bool DrawText(uint32_t window_id, uint32_t x, uint32_t y, const char *text, rgb_color color);
    
    // REAL event handling through Haiku BMessage and BLooper
    bool PollEvents();
    bool GetNextEvent(uint32_t *window_id, uint32_t *event_type, uint32_t *x, uint32_t *y, uint32_t *data);
    
    // REAL display information through Haiku BScreen
    uint32_t GetScreenWidth() const;
    uint32_t GetScreenHeight() const;
    bool SupportsTrueColor() const;
    
    // REAL App Server integration with Haiku app_server
    bool ConnectToAppServer();
    bool DisconnectFromAppServer();
    status_t SendMessageToAppServer(BMessage *message);
    status_t ReceiveMessageFromAppServer(BMessage **message);
    
private:
    // Haiku Be API application
    BApplication *app;
    BScreen *screen;
    
    // Window management using REAL Haiku BWindow
    struct RealWindow {
        BWindow *be_window;        // REAL Haiku BWindow
        BView *be_view;           // REAL Haiku BView  
        BBitmap *bitmap;           // Offscreen drawing bitmap
        BLocker *draw_lock;        // Thread safety for drawing
        
        uint32_t haiku_window_id;  // UserlandVM window ID
        uint32_t width, height;
        uint32_t x, y;
        bool visible;
        bool focused;
        char title[256];
        rgb_color bg_color;
        rgb_color fg_color;
        
        // Drawing state
        bool drawing_active;
        BView *current_drawing_view;
    };
    
    std::map<uint32_t, std::unique_ptr<RealWindow>> windows;
    std::atomic<uint32_t> next_window_id;
    
    // REAL event handling using Haiku BMessage and BLooper
    std::atomic<bool> running;
    std::unique_ptr<std::thread> event_thread;
    std::mutex event_mutex;
    std::mutex window_mutex;
    
    // Event queue for UserlandVM
    struct GUIEvent {
        uint32_t window_id;
        uint32_t event_type; // 1=mouse_down, 2=mouse_up, 3=mouse_move, 4=key_down, 5=key_up, 6=focus, 7=resize
        uint32_t x, y;
        uint32_t data; // button, key_code, etc.
        BMessage *original_message; // Original Haiku BMessage
    };
    
    std::vector<GUIEvent> event_queue;
    
    // Custom Haiku BWindow class that handles events
    class HaikuRealWindow : public BWindow {
    public:
        HaikuRealWindow(BRect frame, const char *title, window_type type, uint32 flags, uint32 workspace);
        virtual ~HaikuRealWindow();
        
        // Event handlers
        virtual void MessageReceived(BMessage *message);
        virtual void MouseDown(BPoint point);
        virtual void MouseUp(BPoint point);
        virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
        virtual void KeyDown(const char *bytes, int32 numBytes);
        virtual void KeyUp(const char *bytes, int32 numBytes);
        virtual void WindowActivated(bool active);
        virtual void FrameResized(float width, float height);
        
        // Set backend reference
        void SetBackend(RealGUIBackend *backend);
        void SetWindowID(uint32_t window_id);
        
    private:
        RealGUIBackend *backend;
        uint32_t window_id;
    };
    
    // Custom Haiku BView class for drawing
    class HaikuRealView : public BView {
    public:
        HaikuRealView(BRect frame, const char *name, uint32 resizingMode, uint32 flags);
        virtual ~HaikuRealView();
        
        // Drawing overrides
        virtual void Draw(BRect updateRect);
        virtual void MouseDown(BPoint point);
        virtual void MouseUp(BPoint point);
        virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
        
        // Set backend reference
        void SetBackend(RealGUIBackend *backend);
        void SetWindowID(uint32_t window_id);
        
    private:
        RealGUIBackend *backend;
        uint32_t window_id;
    };
    
    // Internal methods
    RealWindow* GetWindow(uint32_t window_id);
    bool CreateHaikuWindow(RealWindow *window, const char *title);
    void DestroyHaikuWindow(RealWindow *window);
    void EventLoop();
    void ProcessBMessage(BMessage *message, uint32_t window_id);
    void QueueEvent(uint32_t window_id, uint32_t event_type, uint32_t x, uint32_t y, uint32_t data, BMessage *msg = nullptr);
    
    // Graphics helpers using REAL Haiku Be API
    void SetViewColor(BView *view, rgb_color color);
    void DrawLineOnView(BView *view, BPoint start, BPoint end, rgb_color color);
    void DrawRectOnView(BView *view, BRect rect, rgb_color color);
    void FillRectOnView(BView *view, BRect rect, rgb_color color);
    void DrawTextOnView(BView *view, BPoint point, const char *text, rgb_color color);
    
    // Color conversion utilities
    rgb_color ColorToRGB(uint32_t color);
    uint32_t RGBToColor(rgb_color rgb);
    BRect MakeRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    BPoint MakePoint(uint32_t x, uint32_t y);
    
    // Haiku App Server communication
    status_t InitAppServerConnection();
    void CleanupAppServerConnection();
    bool IsAppServerConnected() const;
    
    // Message constants (Haiku standard messages)
    enum {
        MSG_MOUSE_DOWN = B_MOUSE_DOWN,
        MSG_MOUSE_UP = B_MOUSE_UP, 
        MSG_MOUSE_MOVED = B_MOUSE_MOVED,
        MSG_KEY_DOWN = B_KEY_DOWN,
        MSG_KEY_UP = B_KEY_UP,
        MSG_WINDOW_ACTIVATED = B_WINDOW_ACTIVATED,
        MSG_WINDOW_DEACTIVATED = B_WINDOW_DEACTIVATED,
        MSG_WINDOW_RESIZED = B_WINDOW_RESIZED
    };
    
    // App Server connection status
    bool app_server_connected;
    port_id app_server_port;
    port_id reply_port;
};