#pragma once

#include "HaikuEmulationFramework.h"
#include <map>
#include <vector>
#include <string>

namespace HaikuEmulation {

/////////////////////////////////////////////////////////////////////////////
// Modular InterfaceKit - Universal GUI Kit
/////////////////////////////////////////////////////////////////////////////

class ModularInterfaceKit : public UniversalKit<0x01, "InterfaceKit", "1.0.0"> {
public:
    // Window management
    struct Window {
        int32_t window_id;
        char title[256];
        uint32_t width, height;
        uint32_t x, y;
        bool visible, focused, minimized;
        uint32_t bg_color, fg_color;
        void* native_window;
    };
    
    // Bitmap management
    struct Bitmap {
        int32_t bitmap_id;
        uint32_t width, height;
        uint32_t bytes_per_row;
        uint8_t* bits;
        void* native_bitmap;
    };
    
    // Drawing operations
    struct Point {
        uint32_t x, y;
    };
    
    struct Rect {
        uint32_t x, y, width, height;
    };
    
    // Color definitions
    static constexpr uint32_t COLOR_BLACK = 0xFF000000;
    static constexpr uint32_t COLOR_WHITE = 0xFFFFFFFF;
    static constexpr uint32_t COLOR_RED = 0xFFFF0000;
    static constexpr uint32_t COLOR_GREEN = 0xFF00FF00;
    static constexpr uint32_t COLOR_BLUE = 0xFF0000FF;
    
    // Syscall numbers
    static constexpr uint32_t SYSCALL_CREATE_WINDOW = 0x010001;
    static constexpr uint32_t SYSCALL_DESTROY_WINDOW = 0x010002;
    static constexpr uint32_t SYSCALL_SHOW_WINDOW = 0x010003;
    static constexpr uint32_t SYSCALL_HIDE_WINDOW = 0x010004;
    static constexpr uint32_t SYSCALL_MOVE_WINDOW = 0x010005;
    static constexpr uint32_t SYSCALL_RESIZE_WINDOW = 0x010006;
    static constexpr uint32_t SYSCALL_SET_WINDOW_TITLE = 0x010007;
    
    static constexpr uint32_t SYSCALL_DRAW_LINE = 0x010100;
    static constexpr uint32_t SYSCALL_DRAW_RECT = 0x010101;
    static constexpr uint32_t SYSCALL_FILL_RECT = 0x010102;
    static constexpr uint32_t SYSCALL_DRAW_ELLIPSE = 0x010103;
    static constexpr uint32_t SYSCALL_FILL_ELLIPSE = 0x010104;
    static constexpr uint32_t SYSCALL_DRAW_STRING = 0x010105;
    static constexpr uint32_t SYSCALL_SET_COLOR = 0x010106;
    static constexpr uint32_t SYSCALL_SET_FONT = 0x010107;
    
    static constexpr uint32_t SYSCALL_CREATE_BITMAP = 0x010200;
    static constexpr uint32_t SYSCALL_DESTROY_BITMAP = 0x010201;
    static constexpr uint32_t SYSCALL_LOCK_BITMAP = 0x010202;
    static constexpr uint32_t SYSCALL_UNLOCK_BITMAP = 0x010203;
    static constexpr uint32_t SYSCALL_GET_BITMAP_BITS = 0x010204;
    
    static constexpr uint32_t SYSCALL_FLUSH = 0x010300;
    static constexpr uint32_t SYSCALL_SYNC = 0x010301;
    static constexpr uint32_t SYSCALL_INVALIDATE = 0x010302;
    
    // IEmulationKit implementation
    bool Initialize() override;
    void Shutdown() override;
    
    bool HandleSyscall(uint32_t syscall_num, uint32_t* args, uint32_t* result) override;
    std::vector<uint32_t> GetSupportedSyscalls() const override;
    
    // Window operations
    bool CreateWindow(const char* title, uint32_t width, uint32_t height, 
                     uint32_t x, uint32_t y, int32_t* window_id);
    bool DestroyWindow(int32_t window_id);
    bool ShowWindow(int32_t window_id);
    bool HideWindow(int32_t window_id);
    bool MoveWindow(int32_t window_id, uint32_t x, uint32_t y);
    bool ResizeWindow(int32_t window_id, uint32_t width, uint32_t height);
    bool SetWindowTitle(int32_t window_id, const char* title);
    
    // Drawing operations
    bool DrawLine(int32_t window_id, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
    bool DrawRect(int32_t window_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    bool FillRect(int32_t window_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
    bool DrawEllipse(int32_t window_id, uint32_t cx, uint32_t cy, uint32_t rx, uint32_t ry);
    bool FillEllipse(int32_t window_id, uint32_t cx, uint32_t cy, uint32_t rx, uint32_t ry, uint32_t color);
    bool DrawString(int32_t window_id, uint32_t x, uint32_t y, const char* text, uint32_t length);
    bool SetColor(int32_t window_id, uint32_t color);
    bool SetFont(int32_t window_id, const char* font_family, const char* font_style, float size);
    
    // Bitmap operations
    bool CreateBitmap(uint32_t width, uint32_t height, int32_t* bitmap_id);
    bool DestroyBitmap(int32_t bitmap_id);
    bool LockBitmap(int32_t bitmap_id);
    bool UnlockBitmap(int32_t bitmap_id);
    bool GetBitmapBits(int32_t bitmap_id, uint8_t** bits);
    
    // Display operations
    bool Flush(int32_t window_id);
    bool Sync(int32_t window_id);
    bool Invalidate(int32_t window_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    
    // State management
    bool SaveState(void** data, size_t* size) override;
    bool LoadState(const void* data, size_t size) override;
    
    // Event handling
    bool HandleMouseEvent(int32_t window_id, uint32_t event_type, uint32_t x, uint32_t y, uint32_t buttons);
    bool HandleKeyboardEvent(int32_t window_id, uint32_t event_type, uint32_t key_code, uint32_t modifiers);
    
    // Hardware acceleration
    bool EnableHardwareAcceleration(bool enable);
    bool IsHardwareAccelerationEnabled() const;

private:
    // Window management
    std::map<int32_t, Window> windows;
    std::map<int32_t, Bitmap> bitmaps;
    int32_t next_window_id;
    int32_t next_bitmap_id;
    
    // Display properties
    uint32_t display_width;
    uint32_t display_height;
    uint32_t current_color;
    std::string current_font_family;
    std::string current_font_style;
    float current_font_size;
    
    // Hardware acceleration
    bool hardware_acceleration_enabled;
    void* gl_context;
    uint32_t shader_program;
    
    // Native Haiku integration
    void* native_application;
    std::map<int32_t, void*> native_windows;
    
    // Internal methods
    void InitializeNativeHaiku();
    void InitializeHardwareAcceleration();
    void CleanupNativeHaiku();
    void CleanupHardwareAcceleration();
    
    // Drawing helpers
    void SoftwareDrawLine(int32_t window_id, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
    void HardwareDrawLine(int32_t window_id, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
    void SoftwareDrawRect(int32_t window_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void HardwareDrawRect(int32_t window_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    
    // Configuration hooks
    bool OnConfigure(const std::map<std::string, std::string>& config) override;
};

/////////////////////////////////////////////////////////////////////////////
// Auto-registration
/////////////////////////////////////////////////////////////////////////////

HAIKU_REGISTER_KIT(ModularInterfaceKit);

} // namespace HaikuEmulation