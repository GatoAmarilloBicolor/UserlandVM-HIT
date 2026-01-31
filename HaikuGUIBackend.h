/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Haiku GUI Backend
 * Abstract interface for GUI operations (windows, drawing, input)
 * Can be implemented via SDL2, X11, Wayland, or other backends
 */

#pragma once

#include <SupportDefs.h>
#include <stdint.h>
#include <vector>

// Forward declarations
class ScreenRenderer;

/**
 * Rectangle structure for GUI operations
 */
struct Rect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
    
    int32_t Width() const { return right - left; }
    int32_t Height() const { return bottom - top; }
    
    bool IsValid() const { return left <= right && top <= bottom; }
};

/**
 * Color structure (ARGB)
 */
struct Color {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
    
    uint32_t AsUint32() const {
        return (a << 24) | (r << 16) | (g << 8) | b;
    }
    
    static Color FromUint32(uint32_t val) {
        return {(uint8_t)(val & 0xFF),
                (uint8_t)((val >> 8) & 0xFF),
                (uint8_t)((val >> 16) & 0xFF),
                (uint8_t)((val >> 24) & 0xFF)};
    }
};

/**
 * Window handle (opaque)
 */
typedef uint32_t WindowHandle;

/**
 * Input event types
 */
enum class InputEventType {
    MOUSE_MOVE = 0,
    MOUSE_BUTTON_DOWN = 1,
    MOUSE_BUTTON_UP = 2,
    KEY_DOWN = 3,
    KEY_UP = 4,
    WINDOW_CLOSE = 5,
};

struct InputEvent {
    InputEventType type;
    union {
        struct {
            int32_t x;
            int32_t y;
            int32_t button;  // 0=left, 1=middle, 2=right
        } mouse;
        struct {
            uint16_t key_code;
            uint8_t modifiers;  // shift, ctrl, alt, etc
        } keyboard;
    } data;
};

/**
 * GUI Backend Interface
 * Architecture and rendering-independent abstraction
 */
class HaikuGUIBackend {
public:
    virtual ~HaikuGUIBackend() = default;
    
    /**
     * Initialize the GUI backend
     * @param width Screen width
     * @param height Screen height
     * @param title Window title
     */
    virtual status_t Initialize(uint32_t width, uint32_t height, const char* title) = 0;
    
    /**
     * Shutdown the GUI backend
     */
    virtual status_t Shutdown() = 0;
    
    // Window Management
    
    /**
     * Create a new window
     */
    virtual status_t CreateWindow(uint32_t width, uint32_t height,
                                 const char* title, WindowHandle& handle) = 0;
    
    /**
     * Destroy a window
     */
    virtual status_t DestroyWindow(WindowHandle handle) = 0;
    
    /**
     * Set window title
     */
    virtual status_t SetWindowTitle(WindowHandle handle, const char* title) = 0;
    
    /**
     * Show/hide window
     */
    virtual status_t ShowWindow(WindowHandle handle) = 0;
    virtual status_t HideWindow(WindowHandle handle) = 0;
    
    /**
     * Move window to position
     */
    virtual status_t MoveWindow(WindowHandle handle, int32_t x, int32_t y) = 0;
    
    /**
     * Resize window
     */
    virtual status_t ResizeWindow(WindowHandle handle, uint32_t width, uint32_t height) = 0;
    
    /**
     * Get window frame
     */
    virtual status_t GetWindowFrame(WindowHandle handle, Rect& frame) = 0;
    
    // Graphics Operations
    
    /**
     * Fill rectangle with color
     * Coordinates are in window client area space
     */
    virtual status_t FillRect(WindowHandle window, const Rect& rect, Color color) = 0;
    
    /**
     * Draw a string of text
     */
    virtual status_t DrawString(WindowHandle window, int32_t x, int32_t y,
                               const char* text, Color color) = 0;
    
    /**
     * Set the current drawing color for future operations
     */
    virtual status_t SetColor(Color color) = 0;
    
    /**
     * Copy pixel data to window
     */
    virtual status_t CopyPixels(WindowHandle window, const Rect& rect,
                               const uint32_t* pixels) = 0;
    
    /**
     * Flush graphics - ensure all drawing operations are visible
     */
    virtual status_t FlushGraphics(WindowHandle window) = 0;
    
    /**
     * Get the current framebuffer pointer for direct pixel access
     * Returns pointer to ARGB framebuffer, or nullptr if not supported
     */
    virtual uint32_t* GetFramebuffer(WindowHandle window, uint32_t& pitch) = 0;
    
    // Input Operations
    
    /**
     * Poll for input events
     * Returns true if an event was retrieved
     */
    virtual bool PollEvent(InputEvent& event) = 0;
    
    /**
     * Get mouse position
     */
    virtual status_t GetMousePosition(int32_t& x, int32_t& y) = 0;
    
    /**
     * Wait for event with timeout (ms)
     * 0 = non-blocking, -1 = infinite
     */
    virtual bool WaitEvent(InputEvent& event, int timeout_ms) = 0;
    
    // Utility
    
    /**
     * Get screen dimensions
     */
    virtual void GetScreenSize(uint32_t& width, uint32_t& height) = 0;
    
    /**
     * Take a screenshot and save to memory
     * Returns allocated buffer (caller must free)
     */
    virtual uint32_t* Screenshot(uint32_t& width, uint32_t& height) = 0;
};

/**
 * Create a GUI backend appropriate for the host platform
 * Tries SDL2 first, falls back to platform-specific implementation
 */
HaikuGUIBackend* CreateGUIBackend();
