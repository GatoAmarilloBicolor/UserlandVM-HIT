/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Haiku GUI Backend Implementation
 * SDL2-based rendering with fallback to stub mode
 */

#include "HaikuGUIBackend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>

// Try to include SDL2, but make it optional
#ifdef HAVE_SDL2
    #include <SDL2/SDL.h>
    #define SDL2_ENABLED 1
#else
    #define SDL2_ENABLED 0
#endif

// ============================================================================
// Window Info Structure
// ============================================================================

struct WindowInfo {
    uint32_t width;
    uint32_t height;
    char title[256];
    uint32_t* framebuffer;
    bool visible;
    
#if SDL2_ENABLED
    SDL_Window* sdl_window;
    SDL_Renderer* sdl_renderer;
    SDL_Texture* sdl_texture;
#endif
};

// ============================================================================
// SDL2-Based GUI Backend Implementation
// ============================================================================

#if SDL2_ENABLED

class SDL2GUIBackend : public HaikuGUIBackend {
public:
    SDL2GUIBackend();
    ~SDL2GUIBackend() override;
    
    status_t Initialize(uint32_t width, uint32_t height, const char* title) override;
    status_t Shutdown() override;
    
    status_t CreateWindow(uint32_t width, uint32_t height,
                         const char* title, WindowHandle& handle) override;
    status_t DestroyWindow(WindowHandle handle) override;
    status_t SetWindowTitle(WindowHandle handle, const char* title) override;
    status_t ShowWindow(WindowHandle handle) override;
    status_t HideWindow(WindowHandle handle) override;
    status_t MoveWindow(WindowHandle handle, int32_t x, int32_t y) override;
    status_t ResizeWindow(WindowHandle handle, uint32_t width, uint32_t height) override;
    status_t GetWindowFrame(WindowHandle handle, Rect& frame) override;
    
    status_t FillRect(WindowHandle window, const Rect& rect, Color color) override;
    status_t DrawString(WindowHandle window, int32_t x, int32_t y,
                       const char* text, Color color) override;
    status_t SetColor(Color color) override;
    status_t CopyPixels(WindowHandle window, const Rect& rect,
                       const uint32_t* pixels) override;
    status_t FlushGraphics(WindowHandle window) override;
    
    uint32_t* GetFramebuffer(WindowHandle window, uint32_t& pitch) override;
    
    bool PollEvent(InputEvent& event) override;
    status_t GetMousePosition(int32_t& x, int32_t& y) override;
    bool WaitEvent(InputEvent& event, int timeout_ms) override;
    
    void GetScreenSize(uint32_t& width, uint32_t& height) override;
    uint32_t* Screenshot(uint32_t& width, uint32_t& height) override;
    
private:
    std::map<WindowHandle, WindowInfo> fWindows;
    WindowHandle fNextWindowHandle;
    Color fCurrentColor;
    bool fInitialized;
    
    WindowInfo* GetWindow(WindowHandle handle);
};

SDL2GUIBackend::SDL2GUIBackend()
    : fNextWindowHandle(1), fCurrentColor{0, 0, 0, 0xFF}, fInitialized(false)
{
}

SDL2GUIBackend::~SDL2GUIBackend()
{
    if (fInitialized) {
        Shutdown();
    }
}

status_t SDL2GUIBackend::Initialize(uint32_t width, uint32_t height, const char* title)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("[GUIBackend] SDL_Init failed: %s\n", SDL_GetError());
        return B_ERROR;
    }
    
    fInitialized = true;
    printf("[GUIBackend] SDL2 initialized\n");
    return B_OK;
}

status_t SDL2GUIBackend::Shutdown()
{
    // Destroy all windows
    std::vector<WindowHandle> handles;
    for (auto& w : fWindows) {
        handles.push_back(w.first);
    }
    for (auto h : handles) {
        DestroyWindow(h);
    }
    
    if (fInitialized) {
        SDL_Quit();
        fInitialized = false;
    }
    
    return B_OK;
}

status_t SDL2GUIBackend::CreateWindow(uint32_t width, uint32_t height,
                                     const char* title, WindowHandle& handle)
{
    WindowInfo info;
    info.width = width;
    info.height = height;
    strncpy(info.title, title, sizeof(info.title) - 1);
    info.title[sizeof(info.title) - 1] = 0;
    info.visible = false;
    
    // Allocate framebuffer
    info.framebuffer = (uint32_t*)malloc(width * height * 4);
    if (!info.framebuffer) {
        return B_NO_MEMORY;
    }
    
    // Create SDL window
    info.sdl_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN
    );
    
    if (!info.sdl_window) {
        printf("[GUIBackend] SDL_CreateWindow failed: %s\n", SDL_GetError());
        free(info.framebuffer);
        return B_ERROR;
    }
    
    // Create renderer
    info.sdl_renderer = SDL_CreateRenderer(
        info.sdl_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!info.sdl_renderer) {
        printf("[GUIBackend] SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(info.sdl_window);
        free(info.framebuffer);
        return B_ERROR;
    }
    
    // Create texture for framebuffer
    info.sdl_texture = SDL_CreateTexture(
        info.sdl_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height
    );
    
    if (!info.sdl_texture) {
        printf("[GUIBackend] SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(info.sdl_renderer);
        SDL_DestroyWindow(info.sdl_window);
        free(info.framebuffer);
        return B_ERROR;
    }
    
    // Assign handle
    handle = fNextWindowHandle++;
    fWindows[handle] = info;
    
    printf("[GUIBackend] Window created: handle=%u, %ux%u\n", handle, width, height);
    return B_OK;
}

status_t SDL2GUIBackend::DestroyWindow(WindowHandle handle)
{
    auto it = fWindows.find(handle);
    if (it == fWindows.end()) {
        return B_BAD_VALUE;
    }
    
    WindowInfo& info = it->second;
    
    if (info.sdl_texture) SDL_DestroyTexture(info.sdl_texture);
    if (info.sdl_renderer) SDL_DestroyRenderer(info.sdl_renderer);
    if (info.sdl_window) SDL_DestroyWindow(info.sdl_window);
    if (info.framebuffer) free(info.framebuffer);
    
    fWindows.erase(it);
    return B_OK;
}

status_t SDL2GUIBackend::SetWindowTitle(WindowHandle handle, const char* title)
{
    WindowInfo* info = GetWindow(handle);
    if (!info) return B_BAD_VALUE;
    
    strncpy(info->title, title, sizeof(info->title) - 1);
    SDL_SetWindowTitle(info->sdl_window, title);
    return B_OK;
}

status_t SDL2GUIBackend::ShowWindow(WindowHandle handle)
{
    WindowInfo* info = GetWindow(handle);
    if (!info) return B_BAD_VALUE;
    
    SDL_ShowWindow(info->sdl_window);
    info->visible = true;
    return B_OK;
}

status_t SDL2GUIBackend::HideWindow(WindowHandle handle)
{
    WindowInfo* info = GetWindow(handle);
    if (!info) return B_BAD_VALUE;
    
    SDL_HideWindow(info->sdl_window);
    info->visible = false;
    return B_OK;
}

status_t SDL2GUIBackend::MoveWindow(WindowHandle handle, int32_t x, int32_t y)
{
    WindowInfo* info = GetWindow(handle);
    if (!info) return B_BAD_VALUE;
    
    SDL_SetWindowPosition(info->sdl_window, x, y);
    return B_OK;
}

status_t SDL2GUIBackend::ResizeWindow(WindowHandle handle, uint32_t width, uint32_t height)
{
    WindowInfo* info = GetWindow(handle);
    if (!info) return B_BAD_VALUE;
    
    // Reallocate framebuffer
    uint32_t* new_fb = (uint32_t*)malloc(width * height * 4);
    if (!new_fb) return B_NO_MEMORY;
    
    free(info->framebuffer);
    info->framebuffer = new_fb;
    info->width = width;
    info->height = height;
    
    SDL_SetWindowSize(info->sdl_window, width, height);
    
    // Recreate texture
    SDL_DestroyTexture(info->sdl_texture);
    info->sdl_texture = SDL_CreateTexture(
        info->sdl_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height
    );
    
    return B_OK;
}

status_t SDL2GUIBackend::GetWindowFrame(WindowHandle handle, Rect& frame)
{
    WindowInfo* info = GetWindow(handle);
    if (!info) return B_BAD_VALUE;
    
    frame.left = 0;
    frame.top = 0;
    frame.right = info->width;
    frame.bottom = info->height;
    
    return B_OK;
}

status_t SDL2GUIBackend::FillRect(WindowHandle window, const Rect& rect, Color color)
{
    WindowInfo* info = GetWindow(window);
    if (!info) return B_BAD_VALUE;
    
    uint32_t col = color.AsUint32();
    uint32_t* fb = info->framebuffer;
    uint32_t pitch = info->width;
    
    for (int32_t y = rect.top; y < rect.bottom && y < (int32_t)info->height; ++y) {
        for (int32_t x = rect.left; x < rect.right && x < (int32_t)info->width; ++x) {
            fb[y * pitch + x] = col;
        }
    }
    
    return B_OK;
}

status_t SDL2GUIBackend::DrawString(WindowHandle window, int32_t x, int32_t y,
                                   const char* text, Color color)
{
    // TODO: Implement text rendering with SDL2 TTF or bitmap fonts
    return B_OK;
}

status_t SDL2GUIBackend::SetColor(Color color)
{
    fCurrentColor = color;
    return B_OK;
}

status_t SDL2GUIBackend::CopyPixels(WindowHandle window, const Rect& rect,
                                   const uint32_t* pixels)
{
    WindowInfo* info = GetWindow(window);
    if (!info || !pixels) return B_BAD_VALUE;
    
    uint32_t* fb = info->framebuffer;
    uint32_t width = info->width;
    
    for (int32_t y = rect.top; y < rect.bottom && y < (int32_t)info->height; ++y) {
        int32_t src_y = y - rect.top;
        for (int32_t x = rect.left; x < rect.right && x < (int32_t)info->width; ++x) {
            int32_t src_x = x - rect.left;
            fb[y * width + x] = pixels[src_y * rect.Width() + src_x];
        }
    }
    
    return B_OK;
}

status_t SDL2GUIBackend::FlushGraphics(WindowHandle window)
{
    WindowInfo* info = GetWindow(window);
    if (!info) return B_BAD_VALUE;
    
    // Update texture from framebuffer
    SDL_UpdateTexture(info->sdl_texture, nullptr, info->framebuffer,
                     info->width * 4);
    
    // Render
    SDL_RenderClear(info->sdl_renderer);
    SDL_RenderCopy(info->sdl_renderer, info->sdl_texture, nullptr, nullptr);
    SDL_RenderPresent(info->sdl_renderer);
    
    return B_OK;
}

uint32_t* SDL2GUIBackend::GetFramebuffer(WindowHandle window, uint32_t& pitch)
{
    WindowInfo* info = GetWindow(window);
    if (!info) return nullptr;
    
    pitch = info->width;
    return info->framebuffer;
}

bool SDL2GUIBackend::PollEvent(InputEvent& event)
{
    SDL_Event sdl_event;
    if (!SDL_PollEvent(&sdl_event)) {
        return false;
    }
    
    switch (sdl_event.type) {
        case SDL_MOUSEMOTION:
            event.type = InputEventType::MOUSE_MOVE;
            event.data.mouse.x = sdl_event.motion.x;
            event.data.mouse.y = sdl_event.motion.y;
            return true;
            
        case SDL_MOUSEBUTTONDOWN:
            event.type = InputEventType::MOUSE_BUTTON_DOWN;
            event.data.mouse.button = sdl_event.button.button - 1;
            event.data.mouse.x = sdl_event.button.x;
            event.data.mouse.y = sdl_event.button.y;
            return true;
            
        case SDL_MOUSEBUTTONUP:
            event.type = InputEventType::MOUSE_BUTTON_UP;
            event.data.mouse.button = sdl_event.button.button - 1;
            event.data.mouse.x = sdl_event.button.x;
            event.data.mouse.y = sdl_event.button.y;
            return true;
            
        case SDL_KEYDOWN:
            event.type = InputEventType::KEY_DOWN;
            event.data.keyboard.key_code = sdl_event.key.keysym.scancode;
            event.data.keyboard.modifiers = sdl_event.key.keysym.mod >> 8;
            return true;
            
        case SDL_KEYUP:
            event.type = InputEventType::KEY_UP;
            event.data.keyboard.key_code = sdl_event.key.keysym.scancode;
            event.data.keyboard.modifiers = sdl_event.key.keysym.mod >> 8;
            return true;
            
        case SDL_QUIT:
            event.type = InputEventType::WINDOW_CLOSE;
            return true;
    }
    
    return false;
}

status_t SDL2GUIBackend::GetMousePosition(int32_t& x, int32_t& y)
{
    SDL_GetMouseState(&x, &y);
    return B_OK;
}

bool SDL2GUIBackend::WaitEvent(InputEvent& event, int timeout_ms)
{
    SDL_Event sdl_event;
    int result = SDL_WaitEventTimeout(&sdl_event, timeout_ms);
    
    if (result <= 0) {
        return false;
    }
    
    // Reuse PollEvent logic by putting event back
    SDL_PushEvent(&sdl_event);
    return PollEvent(event);
}

void SDL2GUIBackend::GetScreenSize(uint32_t& width, uint32_t& height)
{
    SDL_DisplayMode mode;
    SDL_GetDesktopDisplayMode(0, &mode);
    width = mode.w;
    height = mode.h;
}

uint32_t* SDL2GUIBackend::Screenshot(uint32_t& width, uint32_t& height)
{
    // TODO: Implement screenshot
    return nullptr;
}

WindowInfo* SDL2GUIBackend::GetWindow(WindowHandle handle)
{
    auto it = fWindows.find(handle);
    if (it == fWindows.end()) {
        return nullptr;
    }
    return &it->second;
}

#endif  // SDL2_ENABLED

// ============================================================================
// Stub GUI Backend (no SDL2)
// ============================================================================

class StubGUIBackend : public HaikuGUIBackend {
public:
    StubGUIBackend();
    ~StubGUIBackend() override;
    
    status_t Initialize(uint32_t width, uint32_t height, const char* title) override;
    status_t Shutdown() override;
    
    status_t CreateWindow(uint32_t width, uint32_t height,
                         const char* title, WindowHandle& handle) override;
    status_t DestroyWindow(WindowHandle handle) override;
    status_t SetWindowTitle(WindowHandle handle, const char* title) override;
    status_t ShowWindow(WindowHandle handle) override;
    status_t HideWindow(WindowHandle handle) override;
    status_t MoveWindow(WindowHandle handle, int32_t x, int32_t y) override;
    status_t ResizeWindow(WindowHandle handle, uint32_t width, uint32_t height) override;
    status_t GetWindowFrame(WindowHandle handle, Rect& frame) override;
    
    status_t FillRect(WindowHandle window, const Rect& rect, Color color) override;
    status_t DrawString(WindowHandle window, int32_t x, int32_t y,
                       const char* text, Color color) override;
    status_t SetColor(Color color) override;
    status_t CopyPixels(WindowHandle window, const Rect& rect,
                       const uint32_t* pixels) override;
    status_t FlushGraphics(WindowHandle window) override;
    
    uint32_t* GetFramebuffer(WindowHandle window, uint32_t& pitch) override;
    
    bool PollEvent(InputEvent& event) override;
    status_t GetMousePosition(int32_t& x, int32_t& y) override;
    bool WaitEvent(InputEvent& event, int timeout_ms) override;
    
    void GetScreenSize(uint32_t& width, uint32_t& height) override;
    uint32_t* Screenshot(uint32_t& width, uint32_t& height) override;
    
private:
    std::map<WindowHandle, WindowInfo> fWindows;
    WindowHandle fNextWindowHandle;
};

StubGUIBackend::StubGUIBackend()
    : fNextWindowHandle(1)
{
    printf("[GUIBackend] Using stub backend (SDL2 not available)\n");
}

StubGUIBackend::~StubGUIBackend()
{
}

status_t StubGUIBackend::Initialize(uint32_t width, uint32_t height, const char* title)
{
    printf("[GUIBackend] Stub Initialize: %ux%u '%s'\n", width, height, title);
    return B_OK;
}

status_t StubGUIBackend::Shutdown()
{
    return B_OK;
}

status_t StubGUIBackend::CreateWindow(uint32_t width, uint32_t height,
                                     const char* title, WindowHandle& handle)
{
    WindowInfo info;
    info.width = width;
    info.height = height;
    strncpy(info.title, title, sizeof(info.title) - 1);
    info.visible = true;
    
    // Allocate framebuffer
    info.framebuffer = (uint32_t*)malloc(width * height * 4);
    if (!info.framebuffer) return B_NO_MEMORY;
    
    memset(info.framebuffer, 0, width * height * 4);
    
    handle = fNextWindowHandle++;
    fWindows[handle] = info;
    
    printf("[GUIBackend] Window created (stub): handle=%u, %ux%u\n", handle, width, height);
    return B_OK;
}

status_t StubGUIBackend::DestroyWindow(WindowHandle handle)
{
    auto it = fWindows.find(handle);
    if (it == fWindows.end()) return B_BAD_VALUE;
    
    if (it->second.framebuffer) {
        free(it->second.framebuffer);
    }
    fWindows.erase(it);
    return B_OK;
}

status_t StubGUIBackend::SetWindowTitle(WindowHandle handle, const char* title)
{
    auto it = fWindows.find(handle);
    if (it == fWindows.end()) return B_BAD_VALUE;
    
    strncpy(it->second.title, title, sizeof(it->second.title) - 1);
    return B_OK;
}

status_t StubGUIBackend::ShowWindow(WindowHandle handle)
{
    auto it = fWindows.find(handle);
    if (it == fWindows.end()) return B_BAD_VALUE;
    
    it->second.visible = true;
    return B_OK;
}

status_t StubGUIBackend::HideWindow(WindowHandle handle)
{
    auto it = fWindows.find(handle);
    if (it == fWindows.end()) return B_BAD_VALUE;
    
    it->second.visible = false;
    return B_OK;
}

status_t StubGUIBackend::MoveWindow(WindowHandle handle, int32_t x, int32_t y)
{
    return B_OK;
}

status_t StubGUIBackend::ResizeWindow(WindowHandle handle, uint32_t width, uint32_t height)
{
    auto it = fWindows.find(handle);
    if (it == fWindows.end()) return B_BAD_VALUE;
    
    uint32_t* new_fb = (uint32_t*)malloc(width * height * 4);
    if (!new_fb) return B_NO_MEMORY;
    
    free(it->second.framebuffer);
    it->second.framebuffer = new_fb;
    it->second.width = width;
    it->second.height = height;
    
    return B_OK;
}

status_t StubGUIBackend::GetWindowFrame(WindowHandle handle, Rect& frame)
{
    auto it = fWindows.find(handle);
    if (it == fWindows.end()) return B_BAD_VALUE;
    
    frame.left = 0;
    frame.top = 0;
    frame.right = it->second.width;
    frame.bottom = it->second.height;
    
    return B_OK;
}

status_t StubGUIBackend::FillRect(WindowHandle window, const Rect& rect, Color color)
{
    auto it = fWindows.find(window);
    if (it == fWindows.end()) return B_BAD_VALUE;
    
    WindowInfo& info = it->second;
    uint32_t col = color.AsUint32();
    
    for (int32_t y = rect.top; y < rect.bottom && y < (int32_t)info.height; ++y) {
        for (int32_t x = rect.left; x < rect.right && x < (int32_t)info.width; ++x) {
            info.framebuffer[y * info.width + x] = col;
        }
    }
    
    return B_OK;
}

status_t StubGUIBackend::DrawString(WindowHandle window, int32_t x, int32_t y,
                                   const char* text, Color color)
{
    return B_OK;
}

status_t StubGUIBackend::SetColor(Color color)
{
    return B_OK;
}

status_t StubGUIBackend::CopyPixels(WindowHandle window, const Rect& rect,
                                   const uint32_t* pixels)
{
    return B_OK;
}

status_t StubGUIBackend::FlushGraphics(WindowHandle window)
{
    return B_OK;
}

uint32_t* StubGUIBackend::GetFramebuffer(WindowHandle window, uint32_t& pitch)
{
    auto it = fWindows.find(window);
    if (it == fWindows.end()) return nullptr;
    
    pitch = it->second.width;
    return it->second.framebuffer;
}

bool StubGUIBackend::PollEvent(InputEvent& event)
{
    return false;
}

status_t StubGUIBackend::GetMousePosition(int32_t& x, int32_t& y)
{
    x = 0;
    y = 0;
    return B_OK;
}

bool StubGUIBackend::WaitEvent(InputEvent& event, int timeout_ms)
{
    return false;
}

void StubGUIBackend::GetScreenSize(uint32_t& width, uint32_t& height)
{
    width = 1024;
    height = 768;
}

uint32_t* StubGUIBackend::Screenshot(uint32_t& width, uint32_t& height)
{
    return nullptr;
}

// ============================================================================
// Factory Function
// ============================================================================

HaikuGUIBackend* CreateGUIBackend()
{
#if SDL2_ENABLED
    return new SDL2GUIBackend();
#else
    return new StubGUIBackend();
#endif
}
