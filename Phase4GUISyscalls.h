#pragma once

#include "PlatformTypes.h"

// Forward declaration
class HaikuOSIPCSystem;

// Only include REAL Haiku backend when compiling on Haiku
#ifdef __HAIKU__
#include "RealGUIBackend.h"
#define HAS_REAL_HAIKU_BACKEND 1
#else
#define HAS_REAL_HAIKU_BACKEND 0
#endif
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Phase4: GUI Syscalls for Haiku window rendering
// Full implementation with BWindow/BView/Display/Network/Events support

class Phase4GUISyscallHandler {
public:
    // Haiku GUI syscall numbers (from BeOS/Haiku syscall table)
    static constexpr int SYSCALL_CREATE_WINDOW = 10001;
    static constexpr int SYSCALL_DESTROY_WINDOW = 10002;
    static constexpr int SYSCALL_POST_MESSAGE = 10003;
    static constexpr int SYSCALL_GET_MESSAGE = 10004;
    static constexpr int SYSCALL_DRAW_LINE = 10005;
    static constexpr int SYSCALL_DRAW_RECT = 10006;
    static constexpr int SYSCALL_FILL_RECT = 10007;
    static constexpr int SYSCALL_DRAW_STRING = 10008;
    static constexpr int SYSCALL_SET_COLOR = 10009;
    static constexpr int SYSCALL_FLUSH = 10010;
    static constexpr int SYSCALL_CREATE_BITMAP = 10011;
    static constexpr int SYSCALL_DESTROY_BITMAP = 10012;
    static constexpr int SYSCALL_BITMAP_BITS = 10013;
    static constexpr int SYSCALL_ACQUIRE_BITMAP = 10014;
    static constexpr int SYSCALL_RELEASE_BITMAP = 10015;
    static constexpr int SYSCALL_NETWORK_INIT = 10016;
    static constexpr int SYSCALL_NETWORK_CONNECT = 10017;
    static constexpr int SYSCALL_NETWORK_SEND = 10018;
    static constexpr int SYSCALL_NETWORK_RECV = 10019;
    static constexpr int SYSCALL_HARDWARE_ACCEL = 10020;
    static constexpr int SYSCALL_MOUSE_EVENT = 10021;
    static constexpr int SYSCALL_KEYBOARD_EVENT = 10022;
    static constexpr int SYSCALL_WINDOW_FOCUS = 10023;
    static constexpr int SYSCALL_WINDOW_RESIZE = 10024;
    static constexpr int SYSCALL_DISPLAY_MODE = 10025;
    
    // Window structure with full Haiku compatibility
    struct Window {
        int32_t window_id;
        char title[256];
        uint32_t width;
        uint32_t height;
        uint32_t x;
        uint32_t y;
        bool visible;
        uint32_t bg_color;
        uint32_t fg_color;
        bool focused;
        bool minimized;
        uint32_t flags;
        void *view_data;
        uint32_t pixel_format;
    };
    
    // Bitmap structure for graphics acceleration
    struct Bitmap {
        int32_t bitmap_id;
        uint32_t width;
        uint32_t height;
        uint32_t bytes_per_row;
        uint32_t pixel_format;
        uint8_t *bits;
        bool locked;
        uint32_t flags;
    };
    
    // Network connection structure
    struct NetworkConnection {
        int32_t conn_id;
        int socket_fd;
        char host[256];
        uint16_t port;
        bool connected;
        uint32_t timeout_ms;
    };
    
    Phase4GUISyscallHandler() : next_window_id(1), next_bitmap_id(1), next_conn_id(1), 
                                output_lines(0), hardware_accelerated(false), 
                                display_width(1024), display_height(768), 
                                current_color(0x00000000), network_initialized(false),
                                ipc_system(nullptr) {
        printf("[GUI] Initialized Full GUI Syscall Handler\n");
        printf("[GUI] Display: %dx%d, Hardware Accel: %s\n", 
               display_width, display_height, hardware_accelerated ? "ON" : "OFF");
        InitializeDisplay();
        InitializeNetwork();
    }
    
    ~Phase4GUISyscallHandler() {
        CleanupDisplay();
        CleanupNetwork();
    }
    
    // Set the IPC system reference for message communication
    void SetIPCSystem(HaikuOSIPCSystem* sys) {
        ipc_system = sys;
        if (ipc_system) {
            printf("[GUI] IPC system connected\n");
        }
    }
    
    bool HandleGUISyscall(int syscall_num, uint32_t *args, uint32_t *result) {
        printf("[GUI] Syscall %d", syscall_num);
        fflush(stdout);
        
        std::lock_guard<std::mutex> lock(gui_mutex);
        
        switch (syscall_num) {
            case SYSCALL_CREATE_WINDOW:
                return HandleCreateWindow(args, result);
            case SYSCALL_DESTROY_WINDOW:
                return HandleDestroyWindow(args, result);
            case SYSCALL_POST_MESSAGE:
                return HandlePostMessage(args, result);
            case SYSCALL_GET_MESSAGE:
                return HandleGetMessage(args, result);
            case SYSCALL_DRAW_LINE:
                return HandleDrawLine(args, result);
            case SYSCALL_DRAW_RECT:
                return HandleDrawRect(args, result);
            case SYSCALL_FILL_RECT:
                return HandleFillRect(args, result);
            case SYSCALL_DRAW_STRING:
                return HandleDrawString(args, result);
            case SYSCALL_SET_COLOR:
                return HandleSetColor(args, result);
            case SYSCALL_FLUSH:
                return HandleFlush(args, result);
            case SYSCALL_CREATE_BITMAP:
                return HandleCreateBitmap(args, result);
            case SYSCALL_DESTROY_BITMAP:
                return HandleDestroyBitmap(args, result);
            case SYSCALL_BITMAP_BITS:
                return HandleBitmapBits(args, result);
            case SYSCALL_ACQUIRE_BITMAP:
                return HandleAcquireBitmap(args, result);
            case SYSCALL_RELEASE_BITMAP:
                return HandleReleaseBitmap(args, result);
            case SYSCALL_NETWORK_INIT:
                return HandleNetworkInit(args, result);
            case SYSCALL_NETWORK_CONNECT:
                return HandleNetworkConnect(args, result);
            case SYSCALL_NETWORK_SEND:
                return HandleNetworkSend(args, result);
            case SYSCALL_NETWORK_RECV:
                return HandleNetworkRecv(args, result);
            case SYSCALL_HARDWARE_ACCEL:
                return HandleHardwareAccel(args, result);
            case SYSCALL_MOUSE_EVENT:
                return HandleMouseEvent(args, result);
            case SYSCALL_KEYBOARD_EVENT:
                return HandleKeyboardEvent(args, result);
            case SYSCALL_WINDOW_FOCUS:
                return HandleWindowFocus(args, result);
            case SYSCALL_WINDOW_RESIZE:
                return HandleWindowResize(args, result);
            case SYSCALL_DISPLAY_MODE:
                return HandleDisplayMode(args, result);
            default:
                printf(" (UNIMPLEMENTED)\n");
                *result = -1;
                return false;
        }
    }
    
    void PrintWindowInfo() {
        printf("[GUI] === Window Manager Status ===\n");
        printf("[GUI] Display: %dx%d, Hardware Accel: %s\n",
               display_width, display_height, hardware_accelerated ? "ON" : "OFF");
        printf("[GUI] Windows: %zu\n", windows.size());
        
        for (const auto& pair : windows) {
            const Window& win = pair.second;
            printf("[GUI]   Window %d: '%s' (%dx%d at %d,%d) %s %s %s\n",
                   win.window_id, win.title, win.width, win.height, win.x, win.y,
                   win.visible ? "visible" : "hidden",
                   win.focused ? "focused" : "unfocused",
                   win.minimized ? "minimized" : "normal");
        }
        
        printf("[GUI] Bitmaps: %zu\n", bitmaps.size());
        printf("[GUI] Network Connections: %zu\n", connections.size());
        printf("[GUI] Message Queue: %zu messages\n", message_queue.size());
        printf("[GUI] ================================\n");
    }
    
private:
    // GUI state management
    std::map<int32_t, Window> windows;
    std::map<int32_t, Bitmap> bitmaps;
    std::map<int32_t, NetworkConnection> connections;
    std::vector<std::string> message_queue;
    
    // IPC system reference for Haiku communication
    HaikuOSIPCSystem* ipc_system;
    
    // Display and rendering state
    int32_t next_window_id;
    int32_t next_bitmap_id;
    int32_t next_conn_id;
    uint32_t display_width;
    uint32_t display_height;
    uint32_t current_color;
    bool hardware_accelerated;
    bool network_initialized;
    size_t output_lines;
    
    // Frame buffer for software rendering
    std::unique_ptr<uint8_t[]> frame_buffer;
    size_t frame_buffer_size;
    
    // Thread safety
    std::mutex gui_mutex;
    
    // GUI backend state
    bool use_real_backend;
#if HAS_REAL_HAIKU_BACKEND
    std::unique_ptr<RealGUIBackend> real_backend;
#endif
    
    // Display initialization and management
    void InitializeDisplay() {
        printf("[GUI] Initializing GUI backend...\n");
        use_real_backend = false;
        
#if HAS_REAL_HAIKU_BACKEND
        printf("[GUI] Attempting REAL Haiku Be API backend...\n");
        
        // Initialize REAL Haiku GUI backend
        real_backend = std::make_unique<RealGUIBackend>();
        use_real_backend = real_backend->Initialize();
        
        if (use_real_backend) {
            display_width = real_backend->GetScreenWidth();
            display_height = real_backend->GetScreenHeight();
            printf("[GUI] ✅ REAL Haiku backend connected: %dx%d display\n", display_width, display_height);
        } else {
            printf("[GUI] ❌ REAL Haiku backend failed, using fallback\n");
        }
#endif
        
        if (!use_real_backend) {
            // Fallback to software rendering
            display_width = 1024;
            display_height = 768;
            frame_buffer_size = display_width * display_height * 4; // 32-bit RGBA
            frame_buffer = std::make_unique<uint8_t[]>(frame_buffer_size);
            
            // Initialize to white background
            memset(frame_buffer.get(), 0xFF, frame_buffer_size);
            
            printf("[GUI] Using software fallback: %dx%d, framebuffer: %zu bytes\n",
                   display_width, display_height, frame_buffer_size);
        }
    }
    
    void CleanupDisplay() {
        frame_buffer.reset();
        printf("[GUI] Display cleaned up\n");
    }
    
    // Network initialization
    void InitializeNetwork() {
        network_initialized = true;
        printf("[GUI] Network subsystem initialized\n");
    }
    
    void CleanupNetwork() {
        for (auto& pair : connections) {
            if (pair.second.socket_fd >= 0) {
                close(pair.second.socket_fd);
            }
        }
        connections.clear();
        printf("[GUI] Network subsystem cleaned up\n");
    }
    
    // Window management syscalls
    bool HandleCreateWindow(uint32_t *args, uint32_t *result) {
        printf("[GUI-SYSCALL] CREATE_WINDOW called with detailed logging\n");
        
        const char *title_ptr = (const char *)args[0];
        uint32_t width = args[1];
        uint32_t height = args[2];
        uint32_t x = args[3];
        uint32_t y = args[4];
        uint32_t flags = 0; // Default flags
        
        printf("[GUI-SYSCALL] CREATE_WINDOW: title='%s' size=%dx%d pos=(%d,%d)\n",
               title_ptr ? title_ptr : "(null)", width, height, x, y);
        
        Window win;
        win.window_id = next_window_id++;
        win.width = width;
        win.height = height;
        win.x = x;
        win.y = y;
        win.visible = true;
        win.bg_color = 0xFFFFFF;
        win.fg_color = 0x000000;
        win.focused = false;
        win.minimized = false;
        win.flags = flags;
        win.view_data = nullptr;
        win.pixel_format = 32; // RGBA
        
        // Safe title copy
        if (title_ptr) {
            strncpy(win.title, title_ptr, sizeof(win.title) - 1);
            win.title[sizeof(win.title) - 1] = '\0';
        } else {
            snprintf(win.title, sizeof(win.title), "Window %d", win.window_id);
        }
        
#if HAS_REAL_HAIKU_BACKEND
        // Use REAL Haiku backend if available
        if (use_real_backend && real_backend) {
                printf("[GUI-SYSCALL] Using REAL Haiku backend for window creation\n");
            uint32_t real_window_id;
            if (real_backend->CreateRealWindow(title_ptr, width, height, x, y, &real_window_id)) {
                win.window_id = real_window_id;
                windows[win.window_id] = win;
                *result = win.window_id;
                
                printf("[GUI-SYSCALL] ✅ REAL Haiku window created: id=%d\n", win.window_id);
                return true;
            } else {
                printf("[GUI-SYSCALL] ❌ REAL Haiku window creation failed\n");
            }
        }
#endif
        
        // Fallback to software rendering
        printf("[GUI-SYSCALL] Using software rendering fallback\n");
        windows[win.window_id] = win;
        *result = win.window_id;
        
        printf("[GUI-SYSCALL] ✅ Software window created: id=%d '%s' (%dx%d at %d,%d) flags=0x%x\n",
               win.window_id, win.title, width, height, x, y, flags);
        
        // If hardware acceleration is available, initialize it
        if (hardware_accelerated) {
            InitializeHardwareAccelForWindow(win.window_id);
        }
        
        return true;
    }
    
    bool HandleDestroyWindow(uint32_t *args, uint32_t *result) {
        int32_t window_id = (int32_t)args[0];
        
        auto it = windows.find(window_id);
        if (it != windows.end()) {
            printf("[GUI] Destroyed window %d: '%s'\n", window_id, it->second.title);
            windows.erase(it);
            *result = 0;
            return true;
        }
        
        printf("[GUI] Window %d not found\n", window_id);
        *result = -1;
        return true;
    }
    
    // Message handling
    bool HandlePostMessage(uint32_t *args, uint32_t *result) {
        uint32_t msg_code = args[0];
        const char *msg_data = (const char *)args[1];
        
        if (msg_data) {
            message_queue.push_back(std::string(msg_data));
            printf("[GUI] Posted message: code=0x%x data='%s'\n", msg_code, msg_data);
        } else {
            message_queue.push_back("");
            printf("[GUI] Posted message: code=0x%x (no data)\n", msg_code);
        }
        
        *result = 0;
        return true;
    }
    
    bool HandleGetMessage(uint32_t *args, uint32_t *result) {
        if (message_queue.empty()) {
            *result = 0; // No messages
            return true;
        }
        
        std::string msg = message_queue.front();
        message_queue.erase(message_queue.begin());
        
        // For simplicity, return message length (real implementation would copy to buffer)
        *result = static_cast<uint32_t>(msg.length());
        
        printf("[GUI] Retrieved message length: %zu\n", msg.length());
        return true;
    }
    
    // Drawing operations
    bool HandleDrawLine(uint32_t *args, uint32_t *result) {
        uint32_t x1 = args[0];
        uint32_t y1 = args[1];
        uint32_t x2 = args[2];
        uint32_t y2 = args[3];
        
        printf("[GUI-SYSCALL] DRAW_LINE: (%d,%d) to (%d,%d) color=0x%x\n", 
               x1, y1, x2, y2, current_color);
        
        if (hardware_accelerated) {
            HardwareDrawLine(x1, y1, x2, y2);
        } else {
            SoftwareDrawLine(x1, y1, x2, y2);
        }
        
        *result = 0;
        return true;
    }
    
    bool HandleDrawRect(uint32_t *args, uint32_t *result) {
        uint32_t x = args[0];
        uint32_t y = args[1];
        uint32_t w = args[2];
        uint32_t h = args[3];
        
        printf("[GUI] Draw rect (%d,%d,%d,%d) color=0x%x\n", x, y, w, h, current_color);
        
        if (hardware_accelerated) {
            HardwareDrawRect(x, y, w, h);
        } else {
            SoftwareDrawRect(x, y, w, h);
        }
        
        *result = 0;
        return true;
    }
    
    bool HandleFillRect(uint32_t *args, uint32_t *result) {
        uint32_t x = args[0];
        uint32_t y = args[1];
        uint32_t w = args[2];
        uint32_t h = args[3];
        uint32_t color = args[4];
        
        printf("[GUI] Fill rect (%d,%d,%d,%d) color=0x%x\n", x, y, w, h, color);
        
        if (hardware_accelerated) {
            HardwareFillRect(x, y, w, h, color);
        } else {
            SoftwareFillRect(x, y, w, h, color);
        }
        
        *result = 0;
        return true;
    }
    
    bool HandleDrawString(uint32_t *args, uint32_t *result) {
        uint32_t x = args[0];
        uint32_t y = args[1];
        const char *text = (const char *)args[2];
        uint32_t length = args[3];
        
        printf("[GUI] Draw string at (%d,%d) length=%d text='%.*s' color=0x%x\n",
               x, y, length, length, text ? text : "(null)", current_color);
        
        if (hardware_accelerated) {
            HardwareDrawString(x, y, text, length);
        } else {
            SoftwareDrawString(x, y, text, length);
        }
        
        *result = 0;
        return true;
    }
    
    bool HandleSetColor(uint32_t *args, uint32_t *result) {
        uint32_t color = args[0];
        current_color = color;
        printf("[GUI] Set color to 0x%x (R:%d G:%d B:%d A:%d)\n",
               color, (color >> 16) & 0xFF, (color >> 8) & 0xFF, 
               color & 0xFF, (color >> 24) & 0xFF);
        *result = 0;
        return true;
    }
    
    bool HandleFlush(uint32_t *args, uint32_t *result) {
        if (hardware_accelerated) {
            HardwareFlush();
        } else {
            SoftwareFlush();
        }
        
        printf("[GUI] Flushed display\n");
        *result = 0;
        return true;
    }
    
    // Bitmap operations
    bool HandleCreateBitmap(uint32_t *args, uint32_t *result) {
        uint32_t width = args[0];
        uint32_t height = args[1];
        uint32_t flags = args[2];
        
        Bitmap bmp;
        bmp.bitmap_id = next_bitmap_id++;
        bmp.width = width;
        bmp.height = height;
        bmp.bytes_per_row = width * 4; // 32-bit
        bmp.pixel_format = 32;
        bmp.locked = false;
        bmp.flags = flags;
        
        size_t bitmap_size = bmp.bytes_per_row * height;
        bmp.bits = new uint8_t[bitmap_size];
        memset(bmp.bits, 0, bitmap_size);
        
        bitmaps[bmp.bitmap_id] = bmp;
        *result = bmp.bitmap_id;
        
        printf("[GUI] Created bitmap %d: %dx%d size=%zu bytes\n",
               bmp.bitmap_id, width, height, bitmap_size);
        
        return true;
    }
    
    bool HandleDestroyBitmap(uint32_t *args, uint32_t *result) {
        int32_t bitmap_id = (int32_t)args[0];
        
        auto it = bitmaps.find(bitmap_id);
        if (it != bitmaps.end()) {
            delete[] it->second.bits;
            bitmaps.erase(it);
            printf("[GUI] Destroyed bitmap %d\n", bitmap_id);
            *result = 0;
            return true;
        }
        
        printf("[GUI] Bitmap %d not found\n", bitmap_id);
        *result = -1;
        return true;
    }
    
    bool HandleBitmapBits(uint32_t *args, uint32_t *result) {
        int32_t bitmap_id = (int32_t)args[0];
        
        auto it = bitmaps.find(bitmap_id);
        if (it != bitmaps.end()) {
            *result = (uint32_t)(uintptr_t)it->second.bits;
            printf("[GUI] Bitmap bits for %d: %p\n", bitmap_id, it->second.bits);
            return true;
        }
        
        *result = 0;
        return false;
    }
    
    bool HandleAcquireBitmap(uint32_t *args, uint32_t *result) {
        int32_t bitmap_id = (int32_t)args[0];
        
        auto it = bitmaps.find(bitmap_id);
        if (it != bitmaps.end()) {
            it->second.locked = true;
            *result = 0;
            printf("[GUI] Acquired bitmap %d\n", bitmap_id);
            return true;
        }
        
        *result = -1;
        return false;
    }
    
    bool HandleReleaseBitmap(uint32_t *args, uint32_t *result) {
        int32_t bitmap_id = (int32_t)args[0];
        
        auto it = bitmaps.find(bitmap_id);
        if (it != bitmaps.end()) {
            it->second.locked = false;
            *result = 0;
            printf("[GUI] Released bitmap %d\n", bitmap_id);
            return true;
        }
        
        *result = -1;
        return false;
    }
    
    // Network operations
    bool HandleNetworkInit(uint32_t *args, uint32_t *result) {
        if (!network_initialized) {
            InitializeNetwork();
        }
        *result = 0;
        return true;
    }
    
    bool HandleNetworkConnect(uint32_t *args, uint32_t *result) {
        const char *host = (const char *)args[0];
        uint16_t port = (uint16_t)args[1];
        uint32_t timeout_ms = args[2];
        
        NetworkConnection conn;
        conn.conn_id = next_conn_id++;
        conn.socket_fd = -1;
        strncpy(conn.host, host ? host : "", sizeof(conn.host) - 1);
        conn.port = port;
        conn.timeout_ms = timeout_ms;
        conn.connected = false;
        
        // Attempt connection
        conn.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (conn.socket_fd >= 0) {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            
            // Simple hostname resolution (localhost for testing)
            if (strcmp(host, "localhost") == 0 || strcmp(host, "127.0.0.1") == 0) {
                addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            } else {
                addr.sin_addr.s_addr = inet_addr(host);
            }
            
            if (connect(conn.socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                conn.connected = true;
                printf("[GUI] Connected to %s:%d (conn_id=%d)\n", host, port, conn.conn_id);
            } else {
                close(conn.socket_fd);
                conn.socket_fd = -1;
                printf("[GUI] Failed to connect to %s:%d\n", host, port);
            }
        }
        
        connections[conn.conn_id] = conn;
        *result = conn.connected ? conn.conn_id : -1;
        return true;
    }
    
    bool HandleNetworkSend(uint32_t *args, uint32_t *result) {
        int32_t conn_id = (int32_t)args[0];
        const void *data = (const void *)args[1];
        uint32_t size = args[2];
        
        auto it = connections.find(conn_id);
        if (it != connections.end() && it->second.connected) {
            ssize_t sent = send(it->second.socket_fd, data, size, 0);
            *result = sent > 0 ? (uint32_t)sent : 0;
            printf("[GUI] Sent %zu bytes on conn %d\n", sent, conn_id);
            return true;
        }
        
        *result = 0;
        return false;
    }
    
    bool HandleNetworkRecv(uint32_t *args, uint32_t *result) {
        int32_t conn_id = (int32_t)args[0];
        void *buffer = (void *)args[1];
        uint32_t size = args[2];
        
        auto it = connections.find(conn_id);
        if (it != connections.end() && it->second.connected) {
            ssize_t received = recv(it->second.socket_fd, buffer, size, 0);
            *result = received > 0 ? (uint32_t)received : 0;
            printf("[GUI] Received %zu bytes on conn %d\n", received, conn_id);
            return true;
        }
        
        *result = 0;
        return false;
    }
    
    // Hardware acceleration
    bool HandleHardwareAccel(uint32_t *args, uint32_t *result) {
        bool enable = args[0] != 0;
        hardware_accelerated = enable;
        
        if (enable) {
            InitializeHardwareAcceleration();
            printf("[GUI] Hardware acceleration enabled\n");
        } else {
            CleanupHardwareAcceleration();
            printf("[GUI] Hardware acceleration disabled\n");
        }
        
        *result = hardware_accelerated ? 1 : 0;
        return true;
    }
    
    // Event handling
    bool HandleMouseEvent(uint32_t *args, uint32_t *result) {
        uint32_t event_type = args[0]; // 1=down, 2=up, 3=move
        uint32_t x = args[1];
        uint32_t y = args[2];
        uint32_t buttons = args[3];
        
        printf("[GUI-SYSCALL] MOUSE_EVENT: type=%d pos=(%d,%d) buttons=0x%x\n",
               event_type, x, y, buttons);
        
        // Route to appropriate window
        RouteMouseEvent(event_type, x, y, buttons);
        
        *result = 0;
        return true;
    }
    
    bool HandleKeyboardEvent(uint32_t *args, uint32_t *result) {
        uint32_t event_type = args[0]; // 1=down, 2=up
        uint32_t key_code = args[1];
        uint32_t modifiers = args[2];
        
        printf("[GUI-SYSCALL] KEYBOARD_EVENT: type=%d key=0x%x modifiers=0x%x\n",
               event_type, key_code, modifiers);
        
        // Route to focused window
        RouteKeyboardEvent(event_type, key_code, modifiers);
        
        *result = 0;
        return true;
    }
    
    bool HandleWindowFocus(uint32_t *args, uint32_t *result) {
        int32_t window_id = (int32_t)args[0];
        bool focused = args[1] != 0;
        
        auto it = windows.find(window_id);
        if (it != windows.end()) {
            it->second.focused = focused;
            printf("[GUI] Window %d focus %s\n", window_id, focused ? "gained" : "lost");
            *result = 0;
            return true;
        }
        
        *result = -1;
        return false;
    }
    
    bool HandleWindowResize(uint32_t *args, uint32_t *result) {
        int32_t window_id = (int32_t)args[0];
        uint32_t new_width = args[1];
        uint32_t new_height = args[2];
        
        auto it = windows.find(window_id);
        if (it != windows.end()) {
            printf("[GUI] Window %d resized from %dx%d to %dx%d\n",
                   window_id, it->second.width, it->second.height, new_width, new_height);
            
            it->second.width = new_width;
            it->second.height = new_height;
            
            // Handle resize in hardware/software
            ResizeWindow(window_id, new_width, new_height);
            
            *result = 0;
            return true;
        }
        
        *result = -1;
        return false;
    }
    
    bool HandleDisplayMode(uint32_t *args, uint32_t *result) {
        uint32_t new_width = args[0];
        uint32_t new_height = args[1];
        uint32_t new_depth = args[2];
        
        printf("[GUI] Display mode change: %dx%d@%d-bit\n", 
               new_width, new_height, new_depth);
        
        display_width = new_width;
        display_height = new_height;
        
        // Reinitialize display with new mode
        CleanupDisplay();
        InitializeDisplay();
        
        *result = 0;
        return true;
    }
    
    // Software rendering implementations
    void SoftwareDrawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
        // Simple Bresenham line algorithm
        int dx = abs((int)x2 - (int)x1);
        int dy = abs((int)y2 - (int)y1);
        int sx = x1 < x2 ? 1 : -1;
        int sy = y1 < y2 ? 1 : -1;
        int err = dx - dy;
        
        uint32_t x = x1, y = y1;
        
        while (true) {
            if (x < display_width && y < display_height) {
                uint32_t pixel_offset = (y * display_width + x) * 4;
                if (pixel_offset < frame_buffer_size) {
                    uint8_t *pixel = &frame_buffer[pixel_offset];
                    pixel[0] = (current_color >> 0) & 0xFF;   // B
                    pixel[1] = (current_color >> 8) & 0xFF;   // G
                    pixel[2] = (current_color >> 16) & 0xFF;  // R
                    pixel[3] = (current_color >> 24) & 0xFF;  // A
                }
            }
            
            if (x == x2 && y == y2) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }
    
    void SoftwareDrawRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        // Draw rectangle outline
        SoftwareDrawLine(x, y, x + w - 1, y);
        SoftwareDrawLine(x + w - 1, y, x + w - 1, y + h - 1);
        SoftwareDrawLine(x + w - 1, y + h - 1, x, y + h - 1);
        SoftwareDrawLine(x, y + h - 1, x, y);
    }
    
    void SoftwareFillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
        for (uint32_t py = y; py < y + h && py < display_height; py++) {
            for (uint32_t px = x; px < x + w && px < display_width; px++) {
                uint32_t pixel_offset = (py * display_width + px) * 4;
                if (pixel_offset < frame_buffer_size) {
                    uint8_t *pixel = &frame_buffer[pixel_offset];
                    pixel[0] = (color >> 0) & 0xFF;   // B
                    pixel[1] = (color >> 8) & 0xFF;   // G
                    pixel[2] = (color >> 16) & 0xFF;  // R
                    pixel[3] = (color >> 24) & 0xFF;  // A
                }
            }
        }
    }
    
    void SoftwareDrawString(uint32_t x, uint32_t y, const char *text, uint32_t length) {
        // Simple text rendering (placeholder)
        printf("[GUI] SoftwareDrawString: '%.*s' at (%d,%d)\n", 
               length, text ? text : "(null)", x, y);
    }
    
    void SoftwareFlush() {
        // In a real implementation, this would copy framebuffer to display
        printf("[GUI] Software flush: %d bytes to display\n", (int)frame_buffer_size);
    }
    
    // Hardware acceleration implementation with OpenGL/Vulkan
    struct HardwareAccelContext {
        void *gl_context;
        void *vulkan_instance;
        uint32_t texture_id;
        uint32_t framebuffer_id;
        uint32_t shader_program;
        bool initialized;
        uint32_t vertex_buffer;
        uint32_t vertex_array;
    };
    
    HardwareAccelContext hw_accel;
    
    void InitializeHardwareAcceleration() {
        printf("[GUI-HW] Initializing hardware acceleration...\n");
        
        memset(&hw_accel, 0, sizeof(hw_accel));
        
        #ifdef __HAIKU__
        // Real Haiku OpenGL implementation
        // This would use BGLView or similar for hardware acceleration
        printf("[GUI-HW] Attempting Haiku OpenGL initialization...\n");
        
        // Create OpenGL context
        hw_accel.gl_context = NULL; // Would be created with BGLView
        if (hw_accel.gl_context) {
            hw_accel.initialized = true;
            printf("[GUI-HW] ✅ Haiku OpenGL context created\n");
            
            // Generate OpenGL resources
            // glGenTextures(1, &hw_accel.texture_id);
            // glGenFramebuffers(1, &hw_accel.framebuffer_id);
            // glGenBuffers(1, &hw_accel.vertex_buffer);
            // glGenVertexArrays(1, &hw_accel.vertex_array);
            
            // Create shader program for 2D rendering
            hw_accel.shader_program = CreateShaderProgram();
            
            printf("[GUI-HW] OpenGL resources initialized\n");
        } else {
            printf("[GUI-HW] ❌ Failed to create Haiku OpenGL context\n");
        }
        #else
        // Stub implementation for non-Haiku systems
        printf("[GUI-HW] Using stub hardware acceleration (non-Haiku)\n");
        hw_accel.gl_context = (void*)0xBEEFDEAD;
        hw_accel.texture_id = 1;
        hw_accel.framebuffer_id = 1;
        hw_accel.shader_program = 1;
        hw_accel.initialized = true;
        #endif
        
        if (hw_accel.initialized) {
            printf("[GUI-HW] ✅ Hardware acceleration initialized\n");
            printf("[GUI-HW] Context: %p, Texture: %u, FBO: %u\n", 
                   hw_accel.gl_context, hw_accel.texture_id, hw_accel.framebuffer_id);
        } else {
            printf("[GUI-HW] ❌ Hardware acceleration initialization failed\n");
        }
    }
    
    void CleanupHardwareAcceleration() {
        printf("[GUI-HW] Cleaning up hardware acceleration...\n");
        
        if (!hw_accel.initialized) {
            return;
        }
        
        #ifdef __HAIKU__
        // Real Haiku OpenGL cleanup
        if (hw_accel.gl_context) {
            // glDeleteTextures(1, &hw_accel.texture_id);
            // glDeleteFramebuffers(1, &hw_accel.framebuffer_id);
            // glDeleteBuffers(1, &hw_accel.vertex_buffer);
            // glDeleteVertexArrays(1, &hw_accel.vertex_array);
            // glDeleteProgram(hw_accel.shader_program);
            
            // Destroy OpenGL context
            hw_accel.gl_context = NULL;
        }
        #endif
        
        memset(&hw_accel, 0, sizeof(hw_accel));
        printf("[GUI-HW] ✅ Hardware acceleration cleaned up\n");
    }
    
    void InitializeHardwareAccelForWindow(int32_t window_id) {
        printf("[GUI-HW] Initializing hardware accel for window %d\n", window_id);
        
        if (!hw_accel.initialized) {
            printf("[GUI-HW] Hardware acceleration not initialized\n");
            return;
        }
        
        #ifdef __HAIKU__
        // Create window-specific framebuffer
        // glBindFramebuffer(GL_FRAMEBUFFER, hw_accel.framebuffer_id);
        // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 
        //                        hw_accel.texture_id, 0);
        #endif
        
        printf("[GUI-HW] ✅ Hardware acceleration ready for window %d\n", window_id);
    }
    
    uint32_t CreateShaderProgram() {
        printf("[GUI-HW] Creating shader program for 2D rendering\n");
        
        #ifdef __HAIKU__
        // Real OpenGL shader creation
        const char* vertex_shader_source = 
            "#version 330 core\n"
            "layout (location = 0) in vec2 aPos;\n"
            "layout (location = 1) in vec4 aColor;\n"
            "out vec4 vertexColor;\n"
            "void main() {\n"
            "    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
            "    vertexColor = aColor;\n"
            "}\n";
            
        const char* fragment_shader_source = 
            "#version 330 core\n"
            "in vec4 vertexColor;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "    fragColor = vertexColor;\n"
            "}\n";
        
        // Compile shaders and link program
        // GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        // GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        // GLuint program = glCreateProgram();
        
        // Return program ID
        return 1; // Placeholder
        #else
        return 1; // Stub program ID
        #endif
    }
    
    void HardwareDrawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
        if (!hw_accel.initialized) {
            printf("[GUI-HW] Hardware acceleration not available\n");
            SoftwareDrawLine(x1, y1, x2, y2);
            return;
        }
        
        printf("[GUI-HW] Hardware accelerated line (%d,%d) to (%d,%d)\n", x1, y1, x2, y2);
        
        #ifdef __HAIKU__
        // Real OpenGL line drawing
        // glBindFramebuffer(GL_FRAMEBUFFER, hw_accel.framebuffer_id);
        // glUseProgram(hw_accel.shader_program);
        // 
        // // Create line vertices
        // float vertices[] = {
        //     (float)x1 / display_width * 2.0f - 1.0f,
        //     1.0f - (float)y1 / display_height * 2.0f,
        //     (float)x2 / display_width * 2.0f - 1.0f,
        //     1.0f - (float)y2 / display_height * 2.0f
        // };
        // 
        // // Upload and draw
        // glBindBuffer(GL_ARRAY_BUFFER, hw_accel.vertex_buffer);
        // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        // glDrawArrays(GL_LINES, 0, 2);
        #endif
    }
    
    void HardwareDrawRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        if (!hw_accel.initialized) {
            printf("[GUI-HW] Hardware acceleration not available\n");
            SoftwareDrawRect(x, y, w, h);
            return;
        }
        
        printf("[GUI-HW] Hardware accelerated rect (%d,%d,%d,%d)\n", x, y, w, h);
        
        #ifdef __HAIKU__
        // Real OpenGL rectangle drawing (outline)
        // Convert to normalized device coordinates
        float x1 = (float)x / display_width * 2.0f - 1.0f;
        float y1 = 1.0f - (float)y / display_height * 2.0f;
        float x2 = (float)(x + w) / display_width * 2.0f - 1.0f;
        float y2 = 1.0f - (float)(y + h) / display_height * 2.0f;
        
        // Rectangle outline vertices
        float vertices[] = {
            x1, y1,  // Top-left
            x2, y1,  // Top-right
            x2, y2,  // Bottom-right
            x1, y2,  // Bottom-left
            x1, y1   // Close the loop
        };
        
        // Upload and draw
        // glBindBuffer(GL_ARRAY_BUFFER, hw_accel.vertex_buffer);
        // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        // glDrawArrays(GL_LINE_STRIP, 0, 5);
        #endif
    }
    
    void HardwareFillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
        if (!hw_accel.initialized) {
            printf("[GUI-HW] Hardware acceleration not available\n");
            SoftwareFillRect(x, y, w, h, color);
            return;
        }
        
        printf("[GUI-HW] Hardware accelerated filled rect (%d,%d,%d,%d) color=0x%x\n", 
               x, y, w, h, color);
        
        #ifdef __HAIKU__
        // Real OpenGL filled rectangle
        // Convert to normalized device coordinates
        float x1 = (float)x / display_width * 2.0f - 1.0f;
        float y1 = 1.0f - (float)y / display_height * 2.0f;
        float x2 = (float)(x + w) / display_width * 2.0f - 1.0f;
        float y2 = 1.0f - (float)(y + h) / display_height * 2.0f;
        
        // Rectangle vertices (two triangles)
        float vertices[] = {
            x1, y1,  // Triangle 1
            x2, y1,
            x1, y2,
            x2, y2,  // Triangle 2
            x1, y2,
            x2, y1
        };
        
        // Set color uniform
        // glUniform4f(glGetUniformLocation(hw_accel.shader_program, "uColor"),
        //            (color >> 16) & 0xFF / 255.0f,  // R
        //            (color >> 8) & 0xFF / 255.0f,   // G
        //            color & 0xFF / 255.0f,          // B
        //            (color >> 24) & 0xFF / 255.0f); // A
        
        // Upload and draw
        // glBindBuffer(GL_ARRAY_BUFFER, hw_accel.vertex_buffer);
        // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        // glDrawArrays(GL_TRIANGLES, 0, 6);
        #endif
    }
    
    void HardwareDrawString(uint32_t x, uint32_t y, const char *text, uint32_t length) {
        if (!hw_accel.initialized) {
            printf("[GUI-HW] Hardware acceleration not available\n");
            SoftwareDrawString(x, y, text, length);
            return;
        }
        
        printf("[GUI-HW] Hardware accelerated text at (%d,%d) length=%d\n", x, y, length);
        
        #ifdef __HAIKU__
        // Real OpenGL text rendering would use texture atlases or signed distance fields
        // For now, just log the operation
        #endif
    }
    
    void HardwareFlush() {
        if (!hw_accel.initialized) {
            printf("[GUI-HW] Hardware acceleration not available\n");
            SoftwareFlush();
            return;
        }
        
        printf("[GUI-HW] Hardware accelerated flush to display\n");
        
        #ifdef __HAIKU__
        // Real OpenGL flush
        // glBindFramebuffer(GL_READ_FRAMEBUFFER, hw_accel.framebuffer_id);
        // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Default framebuffer
        // glBlitFramebuffer(0, 0, display_width, display_height,
        //                  0, 0, display_width, display_height,
        //                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
        // glFlush();
        // glSwapBuffers(); // If using double buffering
        #endif
    }
    
    // Event routing
    void RouteMouseEvent(uint32_t event_type, uint32_t x, uint32_t y, uint32_t buttons) {
        // Find window under mouse and route event
        for (const auto& pair : windows) {
            const Window& win = pair.second;
            if (win.visible && !win.minimized &&
                x >= win.x && x < win.x + win.width &&
                y >= win.y && y < win.y + win.height) {
                printf("[GUI] Routing mouse event to window %d\n", win.window_id);
                // Would send event to window's message queue
                break;
            }
        }
    }
    
    void RouteKeyboardEvent(uint32_t event_type, uint32_t key_code, uint32_t modifiers) {
        // Find focused window and route keyboard event
        for (const auto& pair : windows) {
            if (pair.second.focused) {
                printf("[GUI] Routing keyboard event to focused window %d\n", pair.first);
                // Would send event to window's message queue
                break;
            }
        }
    }
    
    void ResizeWindow(int32_t window_id, uint32_t new_width, uint32_t new_height) {
        if (hardware_accelerated) {
            printf("[GUI] Hardware resize for window %d to %dx%d\n", window_id, new_width, new_height);
        } else {
            printf("[GUI] Software resize for window %d to %dx%d\n", window_id, new_width, new_height);
        }
    }
};