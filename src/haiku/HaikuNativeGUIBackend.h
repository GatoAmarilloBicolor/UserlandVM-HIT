/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * Haiku OS Native GUI Backend Implementation
 * Uses native Haiku app_server for optimal performance
 */

#ifndef _HAIKU_GUI_BACKEND_H
#define _HAIKU_GUI_BACKEND_H

// Haiku OS native includes - only when compiling on Haiku
#ifdef __HAIKU__
// Haiku OS native includes - only when available
#ifdef __HAIKU__
#include <InterfaceKit.h>
#include <AppKit.h>
#include <StorageKit.h>
#include <SupportKit.h>
#include "../../platform/haiku/gui/HaikuGUIBackend.h"
#else
// Stub definitions when not on Haiku
#include "../../Phase4GUISyscalls.h"
typedef void* BWindow;
typedef void* BView;
typedef void* BApplication;
typedef uint32_t WindowHandle;
typedef uint32_t color_space;
typedef void* BBitmap;
typedef void* BLooper;
typedef void* BFont;
typedef void* BMessage;
typedef uint32_t rgb_color;
typedef enum { INPUT_MOUSE, INPUT_KEYBOARD } InputEventType;
typedef struct { int32_t left; int32_t top; int32_t right; int32_t bottom; } Rect;
typedef struct { InputEventType type; union { struct { int32_t x; int32_t y; int32_t button; } mouse; struct { uint16_t key_code; uint8_t modifiers; } keyboard; } data; } InputEvent;
typedef int32_t status_t;
#endif

// Forward declarations
class ScreenRenderer;
#else
// Stub definitions when not on Haiku
#include "../../Phase4GUISyscalls.h"
typedef void* BWindow;
typedef void* BView;
typedef void* BApplication;
typedef uint32_t WindowHandle;
typedef uint32_t color_space;
typedef void* BBitmap;
typedef void* BLooper;
typedef void* BFont;
typedef void* BMessage;
typedef uint32_t rgb_color;
typedef enum { INPUT_MOUSE, INPUT_KEYBOARD } InputEventType;
typedef struct { int32_t left; int32_t top; int32_t right; int32_t bottom; } Rect;
typedef struct { InputEventType type; union { struct { int32_t x; int32_t y; int32_t button; } mouse; struct { uint16_t key_code; uint8_t modifiers; } keyboard; } data; } InputEvent;
#endif

class HaikuNativeGUIBackend : public HaikuGUIBackend {
public:
    HaikuNativeGUIBackend();
    virtual ~HaikuNativeGUIBackend();
    
    // Initialize GUI backend
    virtual status_t Initialize(uint32_t width, uint32_t height, const char* title);
    
    // Window Management
    virtual status_t CreateWindow(uint32_t width, uint32_t height, const char* title, WindowHandle& handle);
    virtual status_t SetWindowTitle(WindowHandle handle, const char* title);
    virtual status_t ResizeWindow(WindowHandle handle, uint32_t width, uint32_t height);
    virtual status_t ShowWindow(WindowHandle handle);
    virtual status_t HideWindow(WindowHandle handle);
    virtual status_t CloseWindow(WindowHandle handle);
    
    // Drawing Operations
    virtual status_t ClearScreen();
    virtual status_t DrawPixel(WindowHandle handle, int32_t x, int32_t y, uint32_t color);
    virtual status_t DrawLine(WindowHandle handle, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    virtual status_t DrawRect(WindowHandle handle, const Rect& rect, uint32_t color);
    virtual status_t FillRect(WindowHandle handle, const Rect& rect, uint32_t color);
    virtual status_t DrawString(WindowHandle handle, int32_t x, int32_t y, const char* text, uint32_t color);
    
    // Event Handling
    virtual status_t HandleEvents();
    virtual status_t HandleInputEvent(const InputEvent& event);
    
    // Synchronization
    virtual status_t Flush();
    virtual status_t WaitVSync();

private:
    // Haiku native GUI components
    BApplication*    fApplication;
    BWindow*         fWindow;
    BView*           fView;
    BBitmap*          fBitmap;
    BView*           fDrawingView;
    
    // Display properties
    uint32_t          fWidth;
    uint32_t          fHeight;
    uint32_t          fDepth;
    color_space       fColorSpace;
    
    // Haiku message handling
    BLooper*          fMessageLooper;
    
    // Font management
    BFont*            fDefaultFont;
    
    // Internal initialization
    status_t InitializeApplication();
    status_t InitializeWindow(const char* title);
    status_t InitializeDrawing();
    status_t SetupMessageHandling();
    
    // Event processing helpers
    void ProcessBMessage(BMessage* message);
    void ProcessInputEvents();
    
    // Drawing helpers
    void ConvertToHaikuColor(uint32_t color, rgb_color& haikuColor);
    status_t UpdateDisplay();
};

#endif // _HAIKU_GUI_BACKEND_H