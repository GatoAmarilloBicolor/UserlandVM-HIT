/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * Haiku OS Native GUI Backend Implementation
 * Uses native Haiku app_server for optimal performance
 */

#ifndef _HAIKU_GUI_BACKEND_H
#define _HAIKU_GUI_BACKEND_H

// Haiku OS native includes
#include <InterfaceKit.h>
#include <AppKit.h>
#include <StorageKit.h>
#include <SupportKit.h>
#include "HaikuGUIBackend.h"

class HaikuNativeGUIBackend : public HaikuGUIBackend {
public:
    HaikuNativeGUIBackend();
    virtual ~HaikuNativeGUIBackend();

    // Initialize native Haiku GUI system
    virtual status_t Initialize(uint32_t width, uint32_t height, const char* title) override;

    // Native Haiku window operations
    virtual status_t CreateWindow(const char* title, uint32_t x, uint32_t y, 
                                 uint32_t width, uint32_t height) override;
    virtual status_t SetWindowTitle(const char* title) override;
    virtual status_t ResizeWindow(uint32_t newWidth, uint32_t newHeight) override;
    virtual status_t ShowWindow() override;
    virtual status_t HideWindow() override;
    virtual status_t CloseWindow() override;

    // Native Haiku drawing operations
    virtual status_t ClearScreen(uint32_t color) override;
    virtual status_t DrawPixel(uint32_t x, uint32_t y, uint32_t color) override;
    virtual status_t DrawLine(uint32_t x1, uint32_t y1, 
                           uint32_t x2, uint32_t y2, uint32_t color) override;
    virtual status_t DrawRect(uint32_t x, uint32_t y, 
                          uint32_t width, uint32_t height, uint32_t color) override;
    virtual status_t FillRect(uint32_t x, uint32_t y, 
                          uint32_t width, uint32_t height, uint32_t color) override;
    virtual status_t DrawString(uint32_t x, uint32_t y, const char* text, 
                           uint32_t color, const char* font = "be_font") override;

    // Native Haiku event handling
    virtual status_t HandleEvents() override;
    virtual status_t HandleInputEvent(uint32_t eventType, uint32_t data) override;

    // Synchronize with display
    virtual status_t Flush() override;
    virtual status_t WaitVSync() override;

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