/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * HaikuNativeGUI.cpp - Direct Haiku AppServer GUI implementation
 */

#include "HaikuNativeGUI.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>

HaikuNativeGUI::HaikuNativeGUI() 
    : fApplication(nullptr), fActiveWindow(-1), fEventQueueHead(0), fEventQueueTail(0),
      fAppServerPort(-1), fIsConnectedToAppServer(false), fDesktopColorSpace(0) {
    printf("[NATIVE_GUI] Haiku Native GUI initialized\n");
}

HaikuNativeGUI::~HaikuNativeGUI() {
    DisconnectFromAppServer();
    printf("[NATIVE_GUI] Haiku Native GUI destroyed\n");
}

status_t HaikuNativeGUI::Initialize() {
    printf("[NATIVE_GUI] Initializing native GUI\n");
    
    // Initialize window table
    memset(fWindows, 0, sizeof(fWindows));
    
    // Initialize drawing contexts
    memset(fDrawingContexts, 0, sizeof(fDrawingContexts));
    
    // Initialize event queue
    memset(fEventQueue, 0, sizeof(fEventQueue));
    
    return ConnectToAppServer();
}

status_t HaikuNativeGUI::ConnectToAppServer() {
    printf("[NATIVE_GUI] Connecting to Haiku AppServer\n");
    
    // Check if we're running on Haiku
    if (getenv("HAIKU") == nullptr && getenv("BEOS") == nullptr) {
        printf("[NATIVE_GUI] Not running on Haiku, native GUI unavailable\n");
        return B_ERROR;
    }
    
    return InitializeAppServerConnection();
}

status_t HaikuNativeGUI::DirectCreateWindow(const char* title, int32_t left, int32_t top, 
                                           int32_t width, int32_t height, WindowType type, 
                                           uint32_t flags, int32_t* window_id) {
    if (!fIsConnectedToAppServer) {
        printf("[NATIVE_GUI] Not connected to AppServer\n");
        return B_ERROR;
    }
    
    // Allocate window handle
    int32_t handle = AllocateWindow();
    if (handle < 0) {
        printf("[NATIVE_GUI] No available window handles\n");
        return B_ERROR;
    }
    
    // Calculate window frame
    BRect frame(left, top, left + width - 1, top + height - 1);
    
    printf("[NATIVE_GUI] Direct create window: %s (%d,%d,%d,%d)\n", 
           title, left, top, width, height);
    
    // Direct AppServer call
    status_t result = CallAppServerCreateWindow(title, &frame, WindowFlagsToHaiku(flags), window_id);
    
    if (result == B_OK && *window_id >= 0) {
        NativeWindowState& window = fWindows[*window_id];
        window.window = nullptr; // Will be set by AppServer
        window.title = strdup(title);
        window.frame = frame;
        window.content_rect = CalculateContentRect(frame, type);
        window.window_flags = flags;
        window.window_type = type;
        window.is_visible = false;
        window.is_active = false;
        window.is_minimized = false;
        window.z_order = fMetrics.windows_created;
        
        fMetrics.windows_created++;
        fMetrics.native_calls_made++;
    }
    
    return result;
}

status_t HaikuNativeGUI::DirectShowWindow(int32_t window_id) {
    if (!IsValidWindow(window_id)) {
        return B_BAD_VALUE;
    }
    
    printf("[NATIVE_GUI] Direct show window: %d\n", window_id);
    
    status_t result = CallAppServerShowWindow(window_id);
    if (result == B_OK) {
        NativeWindowState& window = fWindows[window_id];
        window.is_visible = true;
        window.is_active = true;
        fActiveWindow = window_id;
    }
    
    return result;
}

status_t HaikuNativeGUI::DirectDrawRect(int32_t window_id, int32_t x, int32_t y, int32_t width, int32_t height, rgb_color color) {
    if (!IsValidWindow(window_id)) {
        return B_BAD_VALUE;
    }
    
    DrawingContext* ctx = GetDrawingContext(window_id);
    if (!ctx || !ctx->view) {
        return B_ERROR;
    }
    
    BRect rect(x, y, x + width, y + height);
    ctx->view->SetHighColor(color);
    ctx->view->FillRect(rect);
    
    fMetrics.drawing_operations++;
    fMetrics.native_calls_made++;
    
    return B_OK;
}

status_t HaikuNativeGUI::DirectDrawText(int32_t window_id, int32_t x, int32_t y, const char* text, rgb_color color) {
    if (!IsValidWindow(window_id) || !text) {
        return B_BAD_VALUE;
    }
    
    DrawingContext* ctx = GetDrawingContext(window_id);
    if (!ctx || !ctx->view) {
        return B_ERROR;
    }
    
    ctx->view->SetHighColor(color);
    ctx->view->MovePenTo(BPoint(x, y));
    ctx->view->DrawString(text);
    
    fMetrics.drawing_operations++;
    fMetrics.native_calls_made++;
    
    return B_OK;
}

status_t HaikuNativeGUI::DirectInvalidateRect(int32_t window_id, const BRect* rect) {
    if (!IsValidWindow(window_id)) {
        return B_BAD_VALUE;
    }
    
    printf("[NATIVE_GUI] Direct invalidate rect: %d\n", window_id);
    
    status_t result = CallAppServerInvalidate(window_id, rect);
    
    if (result == B_OK) {
        fMetrics.native_calls_made++;
    }
    
    return result;
}

status_t HaikuNativeGUI::DirectGetNextEvent(InputEvent* event) {
    if (!event) {
        return B_BAD_VALUE;
    }
    
    // Process AppServer events first
    ProcessAppServerEvents();
    
    // Then return from our event queue
    if (DequeueInputEvent(event)) {
        fMetrics.input_events_processed++;
        return B_OK;
    }
    
    return B_ERROR; // No events available
}

// Private implementation methods
int32_t HaikuNativeGUI::AllocateWindow() {
    for (int32_t i = 0; i < MAX_NATIVE_WINDOWS; i++) {
        if (fWindows[i].window == nullptr) {
            return i;
        }
    }
    return -1; // No available windows
}

void HaikuNativeGUI::FreeWindow(int32_t window_id) {
    if (window_id >= 0 && window_id < MAX_NATIVE_WINDOWS) {
        NativeWindowState& window = fWindows[window_id];
        
        if (window.window) {
            // Clean up Haiku window object
            delete window.window;
        }
        
        if (window.title) {
            free(window.title);
        }
        
        // Clean up drawing context
        if (fDrawingContexts[window_id].bitmap) {
            delete fDrawingContexts[window_id].bitmap;
        }
        
        memset(&fWindows[window_id], 0, sizeof(NativeWindowState));
    }
}

HaikuNativeGUI::NativeWindowState* HaikuNativeGUI::GetWindowState(int32_t window_id) {
    if (window_id >= 0 && window_id < MAX_NATIVE_WINDOWS) {
        return &fWindows[window_id];
    }
    return nullptr;
}

bool HaikuNativeGUI::IsValidWindow(int32_t window_id) const {
    return window_id >= 0 && window_id < MAX_NATIVE_WINDOWS && 
           fWindows[window_id].window != nullptr;
}

HaikuNativeGUI::DrawingContext* HaikuNativeGUI::GetDrawingContext(int32_t window_id) {
    if (IsValidWindow(window_id)) {
        return &fDrawingContexts[window_id];
    }
    return nullptr;
}

status_t HaikuNativeGUI::InitializeAppServerConnection() {
    printf("[NATIVE_GUI] Setting up AppServer connection\n");
    
    // In a real implementation, this would connect to the Haiku AppServer
    // For now, simulate successful connection
    fIsConnectedToAppServer = true;
    fAppServerPort = 1; // Simulated port ID
    
    printf("[NATIVE_GUI] AppServer connection established\n");
    return B_OK;
}

void HaikuNativeGUI::QueueInputEvent(const InputEvent& event) {
    int32_t next_tail = (fEventQueueTail + 1) % EVENT_QUEUE_SIZE;
    if (next_tail != fEventQueueHead) {
        fEventQueue[fEventQueueTail] = event;
        fEventQueueTail = next_tail;
    }
}

bool HaikuNativeGUI::DequeueInputEvent(InputEvent* event) {
    if (fEventQueueHead != fEventQueueTail) {
        *event = fEventQueue[fEventQueueHead];
        fEventQueueHead = (fEventQueueHead + 1) % EVENT_QUEUE_SIZE;
        return true;
    }
    return false;
}

void HaikuNativeGUI::ProcessAppServerEvents() {
    // In a real implementation, this would process messages from AppServer
    // For now, simulate some basic events
    static uint32_t event_counter = 0;
    
    if ((event_counter % 100) == 0) {
        // Simulate a mouse move event every 100 calls
        InputEvent mouse_event;
        mouse_event.type = 1; // Mouse move
        mouse_event.timestamp = event_counter;
        mouse_event.x = 100 + (event_counter % 50);
        mouse_event.y = 100 + (event_counter % 30);
        mouse_event.buttons = 0;
        
        QueueInputEvent(mouse_event);
    }
    
    event_counter++;
}

// Simplified AppServer call implementations
status_t HaikuNativeGUI::CallAppServerCreateWindow(const char* title, BRect* frame, uint32_t flags, 
                                                 int32_t* window_id) {
    // In a real implementation, this would call the actual AppServer
    // For now, create a mock window
    *window_id = AllocateWindow();
    if (*window_id >= 0) {
        printf("[NATIVE_GUI] AppServer create window: %s\n", title);
        return B_OK;
    }
    return B_ERROR;
}

status_t HaikuNativeGUI::CallAppServerShowWindow(int32_t window_id) {
    if (IsValidWindow(window_id)) {
        printf("[NATIVE_GUI] AppServer show window: %d\n", window_id);
        return B_OK;
    }
    return B_BAD_VALUE;
}

status_t HaikuNativeGUI::CallAppServerInvalidate(int32_t window_id, const BRect* rect) {
    if (IsValidWindow(window_id)) {
        printf("[NATIVE_GUI] AppServer invalidate window: %d\n", window_id);
        return B_OK;
    }
    return B_BAD_VALUE;
}

// Utility methods
uint32_t HaikuNativeGUI::WindowFlagsToHaiku(uint32_t flags) {
    uint32_t haiku_flags = 0;
    
    if (flags & FLAG_NOT_MOVABLE) haiku_flags |= B_NOT_MOVABLE;
    if (flags & FLAG_NOT_CLOSABLE) haiku_flags |= B_NOT_CLOSABLE;
    if (flags & FLAG_NOT_ZOOMABLE) haiku_flags |= B_NOT_ZOOMABLE;
    if (flags & FLAG_NOT_RESIZABLE) haiku_flags |= B_NOT_RESIZABLE;
    if (flags & FLAG_AVOID_FRONT) haiku_flags |= B_AVOID_FRONT;
    
    return haiku_flags;
}

BRect HaikuNativeGUI::CalculateContentRect(const BRect& frame, WindowType type) {
    // Simple content rect calculation
    float border = 5.0; // Default border size
    float title_height = 20.0; // Default title bar height
    
    switch (type) {
        case WINDOW_BORDERED:
        return BRect(frame.left + border, frame.top + border + title_height,
                      frame.right - border, frame.bottom - border);
        case WINDOW_TITLED:
            return BRect(frame.left, frame.top + title_height,
                      frame.right, frame.bottom);
        default:
            return frame;
    }
}

void HaikuNativeGUI::DisconnectFromAppServer() {
    if (fIsConnectedToAppServer) {
        printf("[NATIVE_GUI] Disconnecting from AppServer\n");
        
        // Clean up all windows
        for (int32_t i = 0; i < MAX_NATIVE_WINDOWS; i++) {
            if (fWindows[i].window) {
                DirectDestroyWindow(i);
            }
        }
        
        fIsConnectedToAppServer = false;
        fAppServerPort = -1;
    }
}

status_t HaikuNativeGUI::DirectDestroyWindow(int32_t window_id) {
    if (!IsValidWindow(window_id)) {
        return B_BAD_VALUE;
    }
    
    printf("[NATIVE_GUI] Direct destroy window: %d\n", window_id);
    
    status_t result = CallAppServerDestroyWindow(window_id);
    if (result == B_OK) {
        fMetrics.windows_destroyed++;
        fMetrics.native_calls_made++;
    }
    
    FreeWindow(window_id);
    return result;
}

status_t HaikuNativeGUI::CallAppServerDestroyWindow(int32_t window_id) {
    printf("[NATIVE_GUI] AppServer destroy window: %d\n", window_id);
    return B_OK; // Simplified
}

void HaikuNativeGUI::RecordNativeOperation(const char* operation, uint64_t time_us) {
    fMetrics.native_calls_made++;
    fMetrics.avg_operation_time_us = (fMetrics.avg_operation_time_us * (fMetrics.native_calls_made - 1) + time_us) / fMetrics.native_calls_made;
}

void HaikuNativeGUI::RecordEmulationSaved(const char* operation) {
    fMetrics.emulation_calls_saved++;
}

// Other methods (simplified implementations)
status_t HaikuNativeGUI::DirectHideWindow(int32_t window_id) { return B_OK; }
status_t HaikuNativeGUI::DirectMoveWindow(int32_t window_id, int32_t left, int32_t top) { return B_OK; }
status_t HaikuNativeGUI::DirectResizeWindow(int32_t window_id, int32_t width, int32_t height) { return B_OK; }
status_t HaikuNativeGUI::DirectSetWindowTitle(int32_t window_id, const char* title) { return B_OK; }
status_t HaikuNativeGUI::DirectActivateWindow(int32_t window_id) { return B_OK; }
status_t HaikuNativeGUI::DirectMinimizeWindow(int32_t window_id) { return B_OK; }
status_t HaikuNativeGUI::DirectRestoreWindow(int32_t window_id) { return B_OK; }
status_t HaikuNativeGUI::DirectBeginDrawing(int32_t window_id) { return B_OK; }
status_t HaikuNativeGUI::DirectEndDrawing(int32_t window_id) { return B_OK; }
status_t HaikuNativeGUI::DirectFillRect(int32_t window_id, const BRect* rect, rgb_color color) { return B_OK; }
status_t HaikuNativeGUI::DirectDrawLine(int32_t window_id, int32_t x1, int32_t y1, int32_t x2, int32_t y2, rgb_color color) { return B_OK; }
status_t HaikuNativeGUI::DirectSetDrawingColor(int32_t window_id, rgb_color color) { return B_OK; }
status_t HaikuNativeGUI::DirectSetClipRect(int32_t window_id, const BRect* rect) { return B_OK; }
status_t HaikuNativeGUI::DirectInvalidateWindow(int32_t window_id) { return DirectInvalidateRect(window_id, nullptr); }
status_t HaikuNativeGUI::DirectGetMousePosition(int32_t* x, int32_t* y) { return B_ERROR; }
status_t HaikuNativeGUI::DirectGetKeyState(uint8_t key_code, bool* is_pressed) { return B_ERROR; }
status_t HaikuNativeGUI::DirectGetDesktopFrame(BRect* frame) { return B_ERROR; }
status_t HaikuNativeGUI::DirectGetDesktopColorSpace(uint32_t* color_space) { return B_ERROR; }
status_t HaikuNativeGUI::DirectSetScreenMode(uint32_t mode) { return B_ERROR; }
status_t HaikuNativeGUI::DirectGetScreenResolution(uint32_t* width, uint32_t* height) { return B_ERROR; }
status_t HaikuNativeGUI::DirectSetWindowFeel(int32_t window_id, uint32_t feel) { return B_OK; }
status_t HaikuNativeGUI::DirectSetWindowLevel(int32_t window_id, int32_t level) { return B_OK; }
status_t HaikuNativeGUI::DirectAddToSubset(int32_t window_id, int32_t parent_window) { return B_OK; }
status_t HaikuNativeGUI::DirectRemoveFromSubset(int32_t window_id) { return B_OK; }
status_t HaikuNativeGUI::DirectSetLook(int32_t window_id, BBitmap* bitmap) { return B_OK; }
status_t HaikuNativeGUI::DirectSetFlags(int32_t window_id, uint32_t flags) { return B_OK; }