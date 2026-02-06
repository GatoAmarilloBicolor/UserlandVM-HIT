/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * HaikuNativeGUI.h - Direct Haiku AppServer GUI integration
 */

#ifndef HAIKU_NATIVE_GUI_H
#define HAIKU_NATIVE_GUI_H

#include <SupportDefs.h>
#include <OS.h>
#include <interface/Window.h>
#include <interface/View.h>
#include <interface/Bitmap.h>
#include <interface/GraphicsDefs.h>
#include <interface/InterfaceDefs.h>
#include <app/Application.h>
#include <app/Message.h>
#include <interface/Rect.h>

class HaikuNativeGUI {
public:
    // Window creation and management
    enum WindowType {
        WINDOW_DOCUMENT = 0,
        WINDOW_MODAL = 1,
        WINDOW_BORDERED = 2,
        WINDOW_TITLED = 3,
        WINDOW_FLOATING = 4,
        WINDOW_DESKTOP = 5
    };

    // Window flags
    enum WindowFlags {
        FLAG_NOT_MOVABLE = 0x01,
        FLAG_NOT_CLOSABLE = 0x02,
        FLAG_NOT_ZOOMABLE = 0x04,
        FLAG_NOT_RESIZABLE = 0x08,
        FLAG_AVOID_FRONT = 0x10,
        FLAG_ACCEPT_FIRST_CLICK = 0x20,
        FLAG_OUTLINE_RESIZE = 0x40,
        FLAG_QUIT_ON_WINDOW_CLOSE = 0x80
    };

    // Drawing operations
    struct DrawingContext {
        BBitmap* bitmap;          // Drawing surface
        BView* view;             // Drawing view
        BRect clipping_rect;       // Current clipping rectangle
        rgb_color current_color;   // Current drawing color
        font_height current_font;  // Current font settings
        drawing_mode current_mode;  // Current drawing mode
        
        DrawingContext() : bitmap(nullptr), view(nullptr), 
                         clipping_rect(0, 0, 0, 0), current_color(0, 0, 0, 255),
                         current_font(B_FONT_SIZE), current_mode(B_OP_COPY) {}
    };

private:
    // GUI state management
    struct NativeWindowState {
        BWindow* window;         // Haiku Window object
        BView* main_view;        // Main drawing view
        char* title;             // Window title
        BRect frame;             // Window frame
        BRect content_rect;       // Content area
        uint32_t window_flags;    // Window behavior flags
        WindowType window_type;    // Window type
        bool is_visible;         // Window visibility
        bool is_active;           // Window has focus
        bool is_minimized;        // Window is minimized
        int32_t z_order;          // Z-order position
        
        NativeWindowState() : window(nullptr), main_view(nullptr), title(nullptr),
                            frame(0, 0, 0, 0), content_rect(0, 0, 0, 0), 
                            window_flags(0), window_type(WINDOW_DOCUMENT), 
                            is_visible(false), is_active(false), is_minimized(false), 
                            z_order(0) {}
    };

    // Application instance
    BApplication* fApplication;
    
    // Window management
    static constexpr int MAX_NATIVE_WINDOWS = 64;
    NativeWindowState fWindows[MAX_NATIVE_WINDOWS];
    int32_t fActiveWindow;
    
    // Drawing contexts
    DrawingContext fDrawingContexts[MAX_NATIVE_WINDOWS];
    
    // Input handling
    struct InputEvent {
        uint32_t type;          // Event type (mouse, keyboard, etc.)
        uint32_t timestamp;      // Event timestamp
        int32_t x, y;           // Mouse coordinates
        uint32_t buttons;        // Mouse button state
        uint8_t key_code;       // Keyboard key code
        uint32_t modifiers;      // Modifier keys state
        char* text;             // Text input (if any)
        
        InputEvent() : type(0), timestamp(0), x(0), y(0), buttons(0), 
                       key_code(0), modifiers(0), text(nullptr) {}
    };
    
    // Event queue
    static constexpr int EVENT_QUEUE_SIZE = 1024;
    InputEvent fEventQueue[EVENT_QUEUE_SIZE];
    int32_t fEventQueueHead;
    int32_t fEventQueueTail;
    
    // Performance metrics
    struct GUIMetrics {
        uint64_t windows_created;
        uint64_t windows_destroyed;
        uint64_t drawing_operations;
        uint64_t input_events_processed;
        uint64_t native_calls_made;
        uint64_t emulation_calls_saved;
        double avg_operation_time_us;
        double performance_improvement_factor;
        
        GUIMetrics() : windows_created(0), windows_destroyed(0),
                       drawing_operations(0), input_events_processed(0),
                       native_calls_made(0), emulation_calls_saved(0),
                       avg_operation_time_us(0.0), performance_improvement_factor(1.0) {}
    };
    
    GUIMetrics fMetrics;
    
    // Direct AppServer connection
    port_id fAppServerPort;
    BMessenger fAppServerMessenger;
    bool fIsConnectedToAppServer;
    uint32_t fDesktopColorSpace;

public:
    HaikuNativeGUI();
    ~HaikuNativeGUI();
    
    // Initialization and connection
    status_t Initialize();
    status_t ConnectToAppServer();
    void DisconnectFromAppServer();
    bool IsConnected() const { return fIsConnectedToAppServer; }
    
    // Direct window operations (bypassing GUI emulation)
    status_t DirectCreateWindow(const char* title, int32_t left, int32_t top, 
                              int32_t width, int32_t height, WindowType type, 
                              uint32_t flags, int32_t* window_id);
    status_t DirectDestroyWindow(int32_t window_id);
    status_t DirectShowWindow(int32_t window_id);
    status_t DirectHideWindow(int32_t window_id);
    status_t DirectMoveWindow(int32_t window_id, int32_t left, int32_t top);
    status_t DirectResizeWindow(int32_t window_id, int32_t width, int32_t height);
    status_t DirectSetWindowTitle(int32_t window_id, const char* title);
    status_t DirectActivateWindow(int32_t window_id);
    status_t DirectMinimizeWindow(int32_t window_id);
    status_t DirectRestoreWindow(int32_t window_id);
    
    // Direct drawing operations
    status_t DirectBeginDrawing(int32_t window_id);
    status_t DirectEndDrawing(int32_t window_id);
    status_t DirectDrawRect(int32_t window_id, int32_t x, int32_t y, int32_t width, int32_t height, rgb_color color);
    status_t DirectFillRect(int32_t window_id, const BRect* rect, rgb_color color);
    status_t DirectDrawLine(int32_t window_id, int32_t x1, int32_t y1, int32_t x2, int32_t y2, rgb_color color);
    status_t DirectDrawText(int32_t window_id, int32_t x, int32_t y, const char* text, rgb_color color);
    status_t DirectSetDrawingColor(int32_t window_id, rgb_color color);
    status_t DirectSetClipRect(int32_t window_id, const BRect* rect);
    status_t DirectInvalidateRect(int32_t window_id, const BRect* rect);
    status_t DirectInvalidateWindow(int32_t window_id);
    
    // Direct input handling
    status_t DirectGetNextEvent(InputEvent* event);
    status_t DirectGetMousePosition(int32_t* x, int32_t* y);
    status_t DirectGetKeyState(uint8_t key_code, bool* is_pressed);
    
    // Direct AppServer operations
    status_t DirectGetDesktopFrame(BRect* frame);
    status_t DirectGetDesktopColorSpace(uint32_t* color_space);
    status_t DirectSetScreenMode(uint32_t mode);
    status_t DirectGetScreenResolution(uint32_t* width, uint32_t* height);
    
    // Advanced native operations
    status_t DirectSetWindowFeel(int32_t window_id, uint32_t feel);
    status_t DirectSetWindowLevel(int32_t window_id, int32_t level);
    status_t DirectAddToSubset(int32_t window_id, int32_t parent_window);
    status_t DirectRemoveFromSubset(int32_t window_id);
    status_t DirectSetLook(int32_t window_id, BBitmap* bitmap);
    status_t DirectSetFlags(int32_t window_id, uint32_t flags);

private:
    // Window management helpers
    int32_t AllocateWindow();
    void FreeWindow(int32_t window_id);
    NativeWindowState* GetWindowState(int32_t window_id);
    bool IsValidWindow(int32_t window_id) const;
    
    // Drawing helpers
    status_t SetupDrawingContext(int32_t window_id);
    DrawingContext* GetDrawingContext(int32_t window_id);
    status_t UpdateDrawingSurface(int32_t window_id);
    
    // AppServer communication helpers
    status_t SendAppServerMessage(BMessage* message);
    status_t ReceiveAppServerMessage(BMessage* message);
    
    // Event handling helpers
    void QueueInputEvent(const InputEvent& event);
    bool DequeueInputEvent(InputEvent* event);
    void ProcessAppServerEvents();
    void HandleWindowMessage(BMessage* message, int32_t window_id);
    
    // Direct AppServer calls (bypassing GUI emulation)
    status_t CallAppServerCreateWindow(const char* title, BRect* frame, uint32_t flags, 
                                      int32_t* window_id);
    status_t CallAppServerDestroyWindow(int32_t window_id);
    status_t CallAppServerShowWindow(int32_t window_id);
    status_t CallAppServerHideWindow(int32_t window_id);
    status_t CallAppServerInvalidate(int32_t window_id, const BRect* rect);
    
    // Performance tracking
    void RecordNativeOperation(const char* operation, uint64_t time_us);
    void RecordEmulationSaved(const char* operation);
    
    // Connection management
    status_t InitializeAppServerConnection();
    status_t EstablishDesktopConnection();
    void CleanupAppServerConnection();
    
    // Utility methods
    uint32_t WindowFlagsToHaiku(uint32_t flags);
    uint32_t WindowTypeToHaiku(WindowType type);
    BRect CalculateContentRect(const BRect& frame, WindowType type);
};

#endif // HAIKU_NATIVE_GUI_H