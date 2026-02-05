/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * Haiku OS Native GUI Backend Implementation
 * Uses native Haiku app_server for optimal performance
 */

#include "HaikuNativeGUIBackend.h"
#include <cstdio>

HaikuNativeGUIBackend::HaikuNativeGUIBackend()
    : fApplication(nullptr),
      fWindow(nullptr),
      fView(nullptr),
      fBitmap(nullptr),
      fDrawingView(nullptr),
      fWidth(0),
      fHeight(0),
      fDepth(32),
      fColorSpace(B_RGB32),
      fMessageLooper(nullptr),
      fDefaultFont(nullptr)
{
    printf("[HAIKU_GUI] Native GUI backend initialized\n");
}

HaikuNativeGUIBackend::~HaikuNativeGUIBackend()
{
    delete fDrawingView;
    delete fBitmap;
    delete fView;
    delete fWindow;
    delete fApplication;
    delete fDefaultFont;
    delete fMessageLooper;
    
    printf("[HAIKU_GUI] Native GUI backend destroyed\n");
}

status_t
HaikuNativeGUIBackend::Initialize(uint32_t width, uint32_t height, const char* title)
{
    printf("[HAIKU_GUI] Initializing native Haiku GUI: %ux%u, title='%s'\n", width, height, title);
    
    fWidth = width;
    fHeight = height;
    
    status_t result;
    
    // Initialize Haiku application
    result = InitializeApplication();
    if (result != B_OK) {
        printf("[HAIKU_GUI] Failed to initialize application: %s\n", strerror(result));
        return result;
    }
    
    // Initialize Haiku window
    result = InitializeWindow(title);
    if (result != B_OK) {
        printf("[HAIKU_GUI] Failed to initialize window: %s\n", strerror(result));
        return result;
    }
    
    // Initialize drawing system
    result = InitializeDrawing();
    if (result != B_OK) {
        printf("[HAIKU_GUI] Failed to initialize drawing: %s\n", strerror(result));
        return result;
    }
    
    // Setup message handling
    result = SetupMessageHandling();
    if (result != B_OK) {
        printf("[HAIKU_GUI] Failed to setup message handling: %s\n", strerror(result));
        return result;
    }
    
    printf("[HAIKU_GUI] Haiku native GUI initialized successfully\n");
    return B_OK;
}

status_t
HaikuNativeGUIBackend::InitializeApplication()
{
    // Create Haiku application with proper signature
    fApplication = new BApplication("application/x-vnd.HaikuVM");
    
    if (!fApplication) {
        printf("[HAIKU_GUI] Failed to create BApplication\n");
        return B_NO_MEMORY;
    }
    
    // Check if application is properly initialized
    if (!fApplication->IsRunning()) {
        printf("[HAIKU_GUI] Application not running\n");
        return B_ERROR;
    }
    
    printf("[HAIKU_GUI] Haiku application initialized\n");
    return B_OK;
}

status_t
HaikuNativeGUIBackend::InitializeWindow(const char* title)
{
    // Create window with Haiku styling
    BRect windowFrame(100, 100, fWidth, fHeight);
    fWindow = new BWindow(windowFrame, title, B_TITLED_WINDOW, B_CURRENT_WORKSPACE);
    
    if (!fWindow) {
        printf("[HAIKU_GUI] Failed to create BWindow\n");
        return B_NO_MEMORY;
    }
    
    // Create main view for drawing
    fView = new BView(windowFrame, "main_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS);
    if (!fView) {
        printf("[HAIKU_GUI] Failed to create BView\n");
        return B_NO_MEMORY;
    }
    
    // Add view to window
    fWindow->AddChild(fView);
    
    printf("[HAIKU_GUI] Haiku window initialized\n");
    return B_OK;
}

status_t
HaikuNativeGUIBackend::InitializeDrawing()
{
    // Create bitmap for offscreen drawing
    BRect bitmapFrame(0, 0, fWidth, fHeight);
    fBitmap = new BBitmap(bitmapFrame, B_RGBA32, true);
    
    if (!fBitmap) {
        printf("[HAIKU_GUI] Failed to create BBitmap\n");
        return B_NO_MEMORY;
    }
    
    // Create drawing view
    fDrawingView = new BView(bitmapFrame, "drawing_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
    if (!fDrawingView) {
        printf("[HAIKU_GUI] Failed to create drawing BView\n");
        return B_NO_MEMORY;
    }
    
    fBitmap->AddChild(fDrawingView);
    
    // Set up default font
    fDefaultFont = new BFont(be_plain_font, 12);
    if (!fDefaultFont) {
        printf("[HAIKU_GUI] Failed to create default font\n");
        return B_NO_MEMORY;
    }
    
    fDrawingView->SetFont(fDefaultFont);
    
    printf("[HAIKU_GUI] Haiku drawing system initialized\n");
    return B_OK;
}

status_t
HaikuNativeGUIBackend::SetupMessageHandling()
{
    // Create message looper for event handling
    fMessageLooper = new BLooper("haiku_gui_events");
    if (!fMessageLooper) {
        printf("[HAIKU_GUI] Failed to create message looper\n");
        return B_NO_MEMORY;
    }
    
    fMessageLooper->Run();
    
    printf("[HAIKU_GUI] Message handling initialized\n");
    return B_OK;
}

status_t
HaikuNativeGUIBackend::CreateWindow(const char* title, uint32_t x, uint32_t y, 
                                uint32_t width, uint32_t height)
{
    if (!fApplication) {
        return B_NO_INIT;
    }
    
    // Create new window with specified parameters
    BRect frame(x, y, x + width, y + height);
    BWindow* newWindow = new BWindow(frame, title, B_TITLED_WINDOW, B_CURRENT_WORKSPACE);
    
    if (!newWindow) {
        return B_NO_MEMORY;
    }
    
    // Close old window and switch to new one
    if (fWindow) {
        fWindow->Hide();
        delete fWindow;
    }
    
    fWindow = newWindow;
    fWidth = width;
    fHeight = height;
    
    printf("[HAIKU_GUI] Window created: %s (%ux%u)\n", title, width, height);
    return B_OK;
}

status_t
HaikuNativeGUIBackend::SetWindowTitle(const char* title)
{
    if (!fWindow) {
        return B_NO_INIT;
    }
    
    fWindow->SetTitle(title);
    printf("[HAIKU_GUI] Window title set to: %s\n", title);
    return B_OK;
}

status_t
HaikuNativeGUIBackend::ResizeWindow(uint32_t newWidth, uint32_t newHeight)
{
    if (!fWindow) {
        return B_NO_INIT;
    }
    
    fWidth = newWidth;
    fHeight = newHeight;
    
    // Resize bitmap and view
    BRect newFrame(0, 0, newWidth, newHeight);
    
    if (fBitmap) {
        fBitmap->Resize(newFrame);
    }
    
    fWindow->ResizeTo(newFrame);
    
    printf("[HAIKU_GUI] Window resized: %ux%u\n", newWidth, newHeight);
    return B_OK;
}

status_t
HaikuNativeGUIBackend::ShowWindow()
{
    if (!fWindow) {
        return B_NO_INIT;
    }
    
    fWindow->Show();
    printf("[HAIKU_GUI] Window shown\n");
    return B_OK;
}

status_t
HaikuNativeGUIBackend::HideWindow()
{
    if (!fWindow) {
        return B_NO_INIT;
    }
    
    fWindow->Hide();
    printf("[HAIKU_GUI] Window hidden\n");
    return B_OK;
}

status_t
HaikuNativeGUIBackend::CloseWindow()
{
    if (!fWindow) {
        return B_NO_INIT;
    }
    
    fWindow->Hide();
    printf("[HAIKU_GUI] Window closed\n");
    return B_OK;
}

void
HaikuNativeGUIBackend::ConvertToHaikuColor(uint32_t color, rgb_color& haikuColor)
{
    // Convert 32-bit RGBA to Haiku rgb_color
    haikuColor.red = (color >> 24) & 0xFF;
    haikuColor.green = (color >> 16) & 0xFF;
    haikuColor.blue = (color >> 8) & 0xFF;
    haikuColor.alpha = color & 0xFF;
}

status_t
HaikuNativeGUIBackend::ClearScreen(uint32_t color)
{
    if (!fDrawingView) {
        return B_NO_INIT;
    }
    
    rgb_color haikuColor;
    ConvertToHaikuColor(color, haikuColor);
    
    fDrawingView->SetLowColor(haikuColor);
    fDrawingView->FillRect(fDrawingView->Bounds());
    
    printf("[HAIKU_GUI] Screen cleared with color 0x%x\n", color);
    return B_OK;
}

status_t
HaikuNativeGUIBackend::DrawPixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (!fDrawingView) {
        return B_NO_INIT;
    }
    
    if (x >= fWidth || y >= fHeight) {
        return B_BAD_VALUE;
    }
    
    rgb_color haikuColor;
    ConvertToHaikuColor(color, haikuColor);
    
    fDrawingView->SetHighColor(haikuColor);
    fDrawingView->StrokeLine(BPoint(x, y), BPoint(x + 1, y + 1));
    
    return B_OK;
}

status_t
HaikuNativeGUIBackend::DrawLine(uint32_t x1, uint32_t y1, 
                              uint32_t x2, uint32_t y2, uint32_t color)
{
    if (!fDrawingView) {
        return B_NO_INIT;
    }
    
    rgb_color haikuColor;
    ConvertToHaikuColor(color, haikuColor);
    
    fDrawingView->SetHighColor(haikuColor);
    fDrawingView->StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
    
    return B_OK;
}

status_t
HaikuNativeGUIBackend::DrawRect(uint32_t x, uint32_t y, 
                              uint32_t width, uint32_t height, uint32_t color)
{
    if (!fDrawingView) {
        return B_NO_INIT;
    }
    
    rgb_color haikuColor;
    ConvertToHaikuColor(color, haikuColor);
    
    fDrawingView->SetHighColor(haikuColor);
    BRect rect(x, y, x + width, y + height);
    fDrawingView->StrokeRect(rect);
    
    return B_OK;
}

status_t
HaikuNativeGUIBackend::FillRect(uint32_t x, uint32_t y, 
                               uint32_t width, uint32_t height, uint32_t color)
{
    if (!fDrawingView) {
        return B_NO_INIT;
    }
    
    rgb_color haikuColor;
    ConvertToHaikuColor(color, haikuColor);
    
    fDrawingView->SetLowColor(haikuColor);
    BRect rect(x, y, x + width, y + height);
    fDrawingView->FillRect(rect);
    
    return B_OK;
}

status_t
HaikuNativeGUIBackend::DrawString(uint32_t x, uint32_t y, const char* text, 
                                   uint32_t color, const char* font)
{
    if (!fDrawingView || !text) {
        return B_BAD_VALUE;
    }
    
    rgb_color haikuColor;
    ConvertToHaikuColor(color, haikuColor);
    
    fDrawingView->SetHighColor(haikuColor);
    fDrawingView->DrawString(text, BPoint(x, y));
    
    printf("[HAIKU_GUI] Text drawn: '%s' at (%u,%u) color=0x%x\n", text, x, y, color);
    return B_OK;
}

status_t
HaikuNativeGUIBackend::HandleEvents()
{
    if (!fMessageLooper) {
        return B_NO_INIT;
    }
    
    BMessage* message;
    while (fMessageLooper->GetNextMessage(&message, B_INFINITE_TIMEOUT) == B_OK) {
        ProcessBMessage(message);
        delete message;
    }
    
    return B_OK;
}

void
HaikuNativeGUIBackend::ProcessBMessage(BMessage* message)
{
    if (!message) return;
    
    int32 what;
    if (message->FindInt32("_what", &what) == B_OK) {
        printf("[HAIKU_GUI] Processing message: _what = 0x%x\n", what);
        
        switch (what) {
            case B_QUIT_REQUESTED:
                printf("[HAIKU_GUI] Quit requested\n");
                break;
            case B_WINDOW_RESIZED:
                printf("[HAIKU_GUI] Window resized\n");
                break;
            case B_WINDOW_MOVED:
                printf("[HAIKU_GUI] Window moved\n");
                break;
            default:
                printf("[HAIKU_GUI] Unknown message type: 0x%x\n", what);
                break;
        }
    }
    
    // Handle key events, mouse events, etc.
    ProcessInputEvents();
}

void
HaikuNativeGUIBackend::ProcessInputEvents()
{
    // Process keyboard and mouse events from Haiku
    // Implementation depends on specific requirements
    
    // Example: Handle basic window close
    if (fWindow && fWindow->QuitRequested()) {
        printf("[HAIKU_GUI] Window close requested\n");
    }
}

status_t
HaikuNativeGUIBackend::HandleInputEvent(uint32_t eventType, uint32_t data)
{
    printf("[HAIKU_GUI] Input event: type=0x%x, data=0x%x\n", eventType, data);
    
    // Handle based on event type
    switch (eventType) {
        case 1: // Key press
            printf("[HAIKU_GUI] Key pressed: 0x%x\n", data);
            break;
        case 2: // Key release
            printf("[HAIKU_GUI] Key released: 0x%x\n", data);
            break;
        case 3: // Mouse move
            printf("[HAIKU_GUI] Mouse move: 0x%x\n", data);
            break;
        case 4: // Mouse click
            printf("[HAIKU_GUI] Mouse click: 0x%x\n", data);
            break;
        default:
            printf("[HAIKU_GUI] Unknown input event\n");
            break;
    }
    
    return B_OK;
}

status_t
HaikuNativeGUIBackend::Flush()
{
    if (!fDrawingView) {
        return B_NO_INIT;
    }
    
    fDrawingView->Sync();
    
    // For Haiku, we need to trigger a redraw
    if (fView) {
        fView->Invalidate();
    }
    
    return B_OK;
}

status_t
HaikuNativeGUIBackend::WaitVSync()
{
    // For Haiku, wait for display refresh
    // This is typically handled automatically by the app_server
    
    // Simple delay to ensure display update
    snooze(16667); // ~60 FPS
    
    return B_OK;
}

status_t
HaikuNativeGUIBackend::UpdateDisplay()
{
    if (!fView) {
        return B_NO_INIT;
    }
    
    // Trigger window update
    fView->Invalidate();
    fView->Flush();
    
    return B_OK;
}