/*
 * HaikuInterfaceKitSimple.h - Simple Haiku Interface Kit
 * 
 * Concrete implementation without abstract base class complications
 */

#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <mutex>

// Constants
#define HAIKU_COLOR_BLACK           0
#define HAIKU_COLOR_WHITE           1
#define HAIKU_COLOR_RED             2
#define HAIKU_COLOR_GREEN           3
#define HAIKU_COLOR_BLUE            4
#define COLOR_COUNT                 5

// Simple window info structure
struct HaikuWindowInfo {
    uint32_t id;
    std::string title;
    int32_t x, y;
    uint32_t width, height;
    bool visible;
    bool active;
    void* host_handle;
    
    HaikuWindowInfo() : id(0), x(0), y(0), width(0), height(0),
                       visible(false), active(false), host_handle(nullptr) {}
};

// Simple view info structure
struct HaikuViewInfo {
    uint32_t id;
    uint32_t window_id;
    uint32_t parent_id;
    
    HaikuViewInfo() : id(0), window_id(0), parent_id(0) {}
};

// Simple concrete implementation
class HaikuInterfaceKitSimple {
private:
    static HaikuInterfaceKitSimple* instance;
    static std::mutex instance_mutex;
    
    std::map<uint32_t, HaikuWindowInfo> windows;
    std::map<uint32_t, HaikuViewInfo> views;
    uint32_t next_window_id;
    uint32_t next_view_id;
    uint32_t colors[COLOR_COUNT];
    
    mutable std::mutex kit_mutex;
    bool initialized;
    
public:
    HaikuInterfaceKitSimple();
    ~HaikuInterfaceKitSimple();
    
    // Singleton access
    static HaikuInterfaceKitSimple& GetInstance();
    
    // Core functionality
    status_t Initialize();
    void Shutdown();
    bool IsInitialized() const { return initialized; }
    
    // Window management
    uint32_t CreateWindow(const char* title, uint32_t x, uint32_t y,
                         uint32_t width, uint32_t height);
    status_t ShowWindow(uint32_t window_id);
    status_t HideWindow(uint32_t window_id);
    void DestroyWindow(uint32_t window_id);
    
    // Drawing operations
    status_t DrawLine(uint32_t window_id, int32_t x1, int32_t y1,
                     int32_t x2, int32_t y2, uint32_t color);
    status_t FillRect(uint32_t window_id, int32_t x, int32_t y,
                     uint32_t width, uint32_t height, uint32_t color);
    status_t DrawString(uint32_t window_id, const char* string,
                       int32_t x, int32_t y, uint32_t color, uint32_t font_id);
    status_t Flush(uint32_t window_id);
    
    // View operations
    status_t AddChild(uint32_t window_id, uint32_t parent_view_id, uint32_t child_view_id);
    status_t RemoveChild(uint32_t window_id, uint32_t view_id);
    
    // Utility methods
    uint32_t GetColor(uint32_t color_index) const;
    void GetStatistics(uint32_t* window_count, uint32_t* view_count) const;
    void DumpState() const;
    
    // For backward compatibility
    typedef HaikuInterfaceKitSimple HaikuInterfaceKit;
};

// For compatibility with existing code
extern "C" {
    HaikuInterfaceKit* GetHaikuInterfaceKit();
}