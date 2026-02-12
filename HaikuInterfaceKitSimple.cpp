/*
 * HaikuInterfaceKitSimple.cpp - Simple Haiku Interface Kit Implementation
 */

#include "HaikuInterfaceKitSimple.h"
#include "UnifiedStatusCodes.h"
#include <cstdio>

// Static instance initialization
HaikuInterfaceKitSimple* HaikuInterfaceKitSimple::instance = nullptr;
std::mutex HaikuInterfaceKitSimple::instance_mutex;

HaikuInterfaceKitSimple::HaikuInterfaceKitSimple() 
    : next_window_id(1), next_view_id(1), initialized(false) {
    
    // Initialize colors
    colors[HAIKU_COLOR_BLACK] = 0xFF000000;
    colors[HAIKU_COLOR_WHITE] = 0xFFFFFFFF;
    colors[HAIKU_COLOR_RED] = 0xFFFF0000;
    colors[HAIKU_COLOR_GREEN] = 0xFF00FF00;
    colors[HAIKU_COLOR_BLUE] = 0xFF0000FF;
}

HaikuInterfaceKitSimple::~HaikuInterfaceKitSimple() {
    if (initialized) {
        Shutdown();
    }
}

HaikuInterfaceKitSimple& HaikuInterfaceKitSimple::GetInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (!instance) {
        instance = new HaikuInterfaceKitSimple();
    }
    return *instance;
}

status_t HaikuInterfaceKitSimple::Initialize() {
    if (initialized) {
        return B_OK;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    printf("[HAIKU_INTERFACE] Initializing Simple Interface Kit...\n");
    
    initialized = true;
    
    printf("[HAIKU_INTERFACE] ✅ Simple Interface Kit initialized\n");
    
    return B_OK;
}

void HaikuInterfaceKitSimple::Shutdown() {
    if (!initialized) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    printf("[HAIKU_INTERFACE] Shutting down Simple Interface Kit...\n");
    
    windows.clear();
    views.clear();
    
    initialized = false;
    
    printf("[HAIKU_INTERFACE] ✅ Simple Interface Kit shutdown complete\n");
}

uint32_t HaikuInterfaceKitSimple::CreateWindow(const char* title, uint32_t x, uint32_t y,
                                               uint32_t width, uint32_t height) {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    uint32_t window_id = next_window_id++;
    
    HaikuWindowInfo window;
    window.id = window_id;
    window.x = x;
    window.y = y;
    window.width = width;
    window.height = height;
    window.visible = false;
    window.active = false;
    window.title = title ? title : "Untitled";
    window.host_handle = reinterpret_cast<void*>(0x60000000 + window_id);
    
    windows[window_id] = window;
    
    printf("[HAIKU_INTERFACE] 🖼️  Created window %u: %s (%ux%u at %u,%u)\n",
           window_id, window.title.c_str(), width, height, x, y);
    
    return window_id;
}

status_t HaikuInterfaceKitSimple::ShowWindow(uint32_t window_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = windows.find(window_id);
    if (it == windows.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuWindowInfo& window = it->second;
    if (window.visible) {
        return B_OK;
    }
    
    window.visible = true;
    window.active = true;
    
    printf("[HAIKU_INTERFACE] 👁️  Showed window %u: %s\n", window_id, window.title.c_str());
    
    return B_OK;
}

status_t HaikuInterfaceKitSimple::HideWindow(uint32_t window_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = windows.find(window_id);
    if (it == windows.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuWindowInfo& window = it->second;
    if (!window.visible) {
        return B_OK;
    }
    
    window.visible = false;
    window.active = false;
    
    printf("[HAIKU_INTERFACE] 👁️  Hidden window %u: %s\n", window_id, window.title.c_str());
    
    return B_OK;
}

void HaikuInterfaceKitSimple::DestroyWindow(uint32_t window_id) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = windows.find(window_id);
    if (it == windows.end()) {
        return;
    }
    
    printf("[HAIKU_INTERFACE] 🗑️  Destroying window %u: %s\n", window_id, it->second.title.c_str());
    
    windows.erase(it);
}

status_t HaikuInterfaceKitSimple::DrawLine(uint32_t window_id, int32_t x1, int32_t y1,
                                              int32_t x2, int32_t y2, uint32_t color) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = windows.find(window_id);
    if (it == windows.end() || !it->second.visible) {
        return B_BAD_VALUE;
    }
    
    printf("[HAIKU_INTERFACE] 📏 Drew line on window %u: (%d,%d)->(%d,%d) color=0x%08X\n",
           window_id, x1, y1, x2, y2, color);
    
    return B_OK;
}

status_t HaikuInterfaceKitSimple::FillRect(uint32_t window_id, int32_t x, int32_t y,
                                                uint32_t width, uint32_t height, uint32_t color) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = windows.find(window_id);
    if (it == windows.end() || !it->second.visible) {
        return B_BAD_VALUE;
    }
    
    printf("[HAIKU_INTERFACE] ⬜ Filled rectangle on window %u: %ux%u at (%d,%d) color=0x%08X\n",
           window_id, width, height, x, y, color);
    
    return B_OK;
}

status_t HaikuInterfaceKitSimple::DrawString(uint32_t window_id, const char* string,
                                                 int32_t x, int32_t y, uint32_t color, uint32_t font_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = windows.find(window_id);
    if (it == windows.end() || !it->second.visible) {
        return B_BAD_VALUE;
    }
    
    printf("[HAIKU_INTERFACE] 📝 Drew string on window %u: \"%s\" at (%d,%d) color=0x%08X font=%u\n",
           window_id, string ? string : "(null)", x, y, color, font_id);
    
    return B_OK;
}

status_t HaikuInterfaceKitSimple::Flush(uint32_t window_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = windows.find(window_id);
    if (it == windows.end() || !it->second.visible) {
        return B_BAD_VALUE;
    }
    
    printf("[HAIKU_INTERFACE] 🔄 Flushed window %u: %s\n", window_id, it->second.title.c_str());
    
    return B_OK;
}

status_t HaikuInterfaceKitSimple::AddChild(uint32_t window_id, uint32_t parent_view_id, uint32_t child_view_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = windows.find(window_id);
    if (it == windows.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuViewInfo view;
    view.id = child_view_id;
    view.window_id = window_id;
    view.parent_id = parent_view_id;
    
    views[child_view_id] = view;
    
    printf("[HAIKU_INTERFACE] 🔗 Added view %u as child of view %u in window %u\n",
           child_view_id, parent_view_id, window_id);
    
    return B_OK;
}

status_t HaikuInterfaceKitSimple::RemoveChild(uint32_t window_id, uint32_t view_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    auto it = views.find(view_id);
    if (it == views.end() || it->second.window_id != window_id) {
        return B_BAD_VALUE;
    }
    
    printf("[HAIKU_INTERFACE] 🔗 Removed view %u from window %u\n", view_id, window_id);
    
    views.erase(it);
    
    return B_OK;
}

uint32_t HaikuInterfaceKitSimple::GetColor(uint32_t color_index) const {
    if (color_index < COLOR_COUNT) {
        return colors[color_index];
    }
    return colors[HAIKU_COLOR_BLACK];
}

void HaikuInterfaceKitSimple::GetStatistics(uint32_t* window_count, uint32_t* view_count) const {
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    if (window_count) *window_count = windows.size();
    if (view_count) *view_count = views.size();
}

void HaikuInterfaceKitSimple::DumpState() const {
    std::lock_guard<std::mutex> lock(kit_mutex);
    
    printf("[HAIKU_INTERFACE] Simple Interface Kit State Dump:\n");
    printf("  Windows (%zu):\n", windows.size());
    for (const auto& pair : windows) {
        printf("    %u: %s (%ux%u at %d,%d) %s\n",
               pair.second.id, pair.second.title.c_str(), 
               pair.second.width, pair.second.height,
               pair.second.x, pair.second.y,
               pair.second.visible ? "visible" : "hidden");
    }
    
    printf("  Views (%zu):\n", views.size());
    for (const auto& pair : views) {
        printf("    %u: in window %u, parent %u\n",
               pair.second.id, pair.second.window_id, pair.second.parent_id);
    }
}

// C compatibility wrapper
extern "C" {
    HaikuInterfaceKit* GetHaikuInterfaceKit() {
        return &HaikuInterfaceKitSimple::GetInstance();
    }
}