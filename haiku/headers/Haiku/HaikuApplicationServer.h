/*
 * HaikuApplicationServer.h - Haiku Application Server Virtualizer
 * 
 * Implements Haiku's app_server protocol for GUI operations
 * Provides window management, drawing, font, clipboard, and drag&drop
 */

#pragma once

#include "HaikuAPIVirtualizer.h"
#include <cstdint>
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

// Application Server Protocol Constants
#define APP_SERVER_PORT          56000
#define APP_SERVER_MAGIC         0x4841494B55414C50  // "HAIKUALP"
#define APP_SERVER_VERSION       1

// Window types (matching Haiku's app_server protocol)
#define WINDOW_TYPE_DOCUMENT     1
#define WINDOW_TYPE_MODAL        2
#define WINDOW_TYPE_BORDERED     3
#define WINDOW_TYPE_TITLED       4
#define WINDOW_TYPE_FLOATING      5
#define WINDOW_TYPE_DESKTOP      6

// Window look/feel
#define WINDOW_LOOK_TITLED       0
#define WINDOW_LOOK_DOCUMENT      1
#define WINDOW_LOOK_MODAL        2
#define WINDOW_LOOK_BORDERED     3
#define WINDOW_LOOK_FLOATING      4
#define WINDOW_LOOK_DESKTOP      5

#define WINDOW_FEEL_NORMAL       0
#define WINDOW_FEEL_MODAL_SUBSET 1
#define WINDOW_FEEL_APP_SUBSET  2
#define WINDOW_FEEL_ALL_FRONT   3
#define WINDOW_FEEL_STAYS_ON_TOP 4

// Window flags
#define WINDOW_NOT_MOVABLE       (1 << 0)
#define WINDOW_NOT_RESIZABLE     (1 << 1)
#define WINDOW_NOT_CLOSABLE      (1 << 2)
#define WINDOW_NOT_ZOOMABLE      (1 << 3)
#define WINDOW_NOT_MINIMIZABLE   (1 << 4)
#define WINDOW_AVOID_FRONT       (1 << 5)
#define WINDOW_AVOID_FOCUS       (1 << 6)
#define WINDOW_WILL_ACCEPT_FIRST_CLICK (1 << 7)
#define WINDOW_OUTLINE_RESIZE    (1 << 8)
#define WINDOW_NO_WORKSPACE_ACTIVATION (1 << 9)

// Message types
#define MSG_WINDOW_CREATED       1001
#define MSG_WINDOW_SHOWN         1002
#define MSG_WINDOW_HIDDEN        1003
#define MSG_WINDOW_DESTROYED     1004
#define MSG_WINDOW_RESIZED       1005
#define MSG_WINDOW_MOVED        1006
#define MSG_WINDOW_ACTIVATED     1007
#define MSG_WINDOW_DEACTIVATED   1008
#define MSG_MOUSE_DOWN          1101
#define MSG_MOUSE_UP            1102
#define MSG_MOUSE_MOVED         1103
#define MSG_KEY_DOWN            1201
#define MSG_KEY_UP              1202
#define MSG_QUIT_REQUESTED      2001

// Drawing operations
#define DRAW_OP_LINE             1
#define DRAW_OP_RECT             2
#define DRAW_OP_FILL_RECT        3
#define DRAW_OP_ELLIPSE          4
#define DRAW_OP_FILL_ELLIPSE     5
#define DRAW_OP_STRING           6
#define DRAW_OP_BITMAP           7

// Forward declarations
struct WindowInfo;
struct DrawingContext;
struct FontInfo;

// ============================================================================
// HAIKU APPLICATION SERVER
// ============================================================================

/**
 * Virtualized Haiku Application Server
 * 
 * This class implements Haiku's app_server protocol that handles all GUI operations.
 * It manages windows, drawing, fonts, clipboard, drag&drop, and input events.
 * 
 * The app_server is the central component that all Haiku GUI applications connect to
 * for window management and drawing operations.
 */
class HaikuApplicationServer : public HaikuKit {
private:
    // Server state
    bool server_running;
    bool initialized;
    uint32_t next_window_id;
    uint32_t next_font_id;
    uint32_t next_bitmap_id;
    
    // Threading
    std::unique_ptr<std::thread> server_thread;
    std::mutex server_mutex;
    std::condition_variable server_cv;
    
    // Window management
    std::map<uint32_t, std::unique_ptr<WindowInfo>> windows;
    std::mutex windows_mutex;
    
    // Font management
    std::map<uint32_t, std::unique_ptr<FontInfo>> fonts;
    std::mutex fonts_mutex;
    
    // Bitmap management
    std::map<uint32_t, std::unique_ptr<DrawingContext>> bitmaps;
    std::mutex bitmaps_mutex;
    
    // Event queue
    std::vector<uint32_t> pending_events;
    std::mutex events_mutex;
    
    // Clipboard data
    std::string clipboard_data;
    std::mutex clipboard_mutex;
    
    // Server socket (for real networking)
    int server_socket;
    
public:
    /**
     * Constructor
     */
    HaikuApplicationServer();
    
    /**
     * Destructor
     */
    virtual ~HaikuApplicationServer();
    
    // HaikuKit interface
    virtual status_t Initialize() override;
    virtual void Shutdown() override;
    
    /**
     * Start the application server
     */
    status_t Start();
    
    /**
     * Stop the application server
     */
    status_t Stop();
    
    /**
     * Check if server is running
     */
    bool IsRunning() const { return server_running; }
    
    /**
     * Process pending events (called from main loop)
     */
    void ProcessEvents();
    
    // ========================================================================
    // WINDOW MANAGEMENT
    // ========================================================================
    
    /**
     * Create a new window
     * 
     * @param title Window title
     * @param x X position
     * @param y Y position  
     * @param width Window width
     * @param height Window height
     * @param window_type Window type (document, modal, etc.)
     * @param window_look Window look
     * @param window_feel Window feel
     * @param flags Window flags
     * @return Window ID or 0 on error
     */
    uint32_t CreateWindow(const char* title, int32_t x, int32_t y,
                         uint32_t width, uint32_t height,
                         uint32_t window_type, uint32_t window_look,
                         uint32_t window_feel, uint32_t flags);
    
    /**
     * Show a window
     */
    status_t ShowWindow(uint32_t window_id);
    
    /**
     * Hide a window
     */
    status_t HideWindow(uint32_t window_id);
    
    /**
     * Destroy a window
     */
    status_t DestroyWindow(uint32_t window_id);
    
    /**
     * Move a window
     */
    status_t MoveWindow(uint32_t window_id, int32_t x, int32_t y);
    
    /**
     * Resize a window
     */
    status_t ResizeWindow(uint32_t window_id, uint32_t width, uint32_t height);
    
    /**
     * Set window title
     */
    status_t SetWindowTitle(uint32_t window_id, const char* title);
    
    /**
     * Activate a window (bring to front)
     */
    status_t ActivateWindow(uint32_t window_id);
    
    // ========================================================================
    // DRAWING OPERATIONS
    // ========================================================================
    
    /**
     * Get drawing context for a window
     */
    DrawingContext* GetDrawingContext(uint32_t window_id);
    
    /**
     * Draw a line
     */
    status_t DrawLine(uint32_t window_id, int32_t x1, int32_t y1,
                     int32_t x2, int32_t y2, uint32_t color);
    
    /**
     * Draw a rectangle outline
     */
    status_t DrawRect(uint32_t window_id, int32_t x, int32_t y,
                     uint32_t width, uint32_t height, uint32_t color);
    
    /**
     * Fill a rectangle
     */
    status_t FillRect(uint32_t window_id, int32_t x, int32_t y,
                     uint32_t width, uint32_t height, uint32_t color);
    
    /**
     * Draw an ellipse outline
     */
    status_t DrawEllipse(uint32_t window_id, int32_t x, int32_t y,
                       uint32_t width, uint32_t height, uint32_t color);
    
    /**
     * Fill an ellipse
     */
    status_t FillEllipse(uint32_t window_id, int32_t x, int32_t y,
                        uint32_t width, uint32_t height, uint32_t color);
    
    /**
     * Draw a string
     */
    status_t DrawString(uint32_t window_id, const char* string,
                       int32_t x, int32_t y, uint32_t color, uint32_t font_id);
    
    /**
     * Draw a bitmap
     */
    status_t DrawBitmap(uint32_t window_id, uint32_t bitmap_id,
                       int32_t x, int32_t y);
    
    /**
     * Flush drawing operations to screen
     */
    status_t FlushWindow(uint32_t window_id);
    
    /**
     * Clear a window
     */
    status_t ClearWindow(uint32_t window_id, uint32_t color);
    
    // ========================================================================
    // FONT MANAGEMENT
    // ========================================================================
    
    /**
     * Create a font
     */
    uint32_t CreateFont(const char* family, const char* style,
                       float size, uint16_t face);
    
    /**
     * Set font for drawing
     */
    status_t SetFont(uint32_t window_id, uint32_t font_id);
    
    /**
     * Get font metrics
     */
    status_t GetFontMetrics(uint32_t font_id, float* ascent, float* descent,
                           float* leading);
    
    /**
     * Get string width in pixels
     */
    float GetStringWidth(const char* string, uint32_t font_id);
    
    // ========================================================================
    // CLIPBOARD OPERATIONS
    // ========================================================================
    
    /**
     * Set clipboard data
     */
    status_t SetClipboard(const char* data);
    
    /**
     * Get clipboard data
     */
    status_t GetClipboard(char* buffer, size_t buffer_size);
    
    /**
     * Clear clipboard
     */
    status_t ClearClipboard();
    
    // ========================================================================
    // DRAG & DROP OPERATIONS
    // ========================================================================
    
    /**
     * Start drag operation
     */
    status_t StartDrag(uint32_t window_id, const void* data, size_t data_size);
    
    /**
     * Accept drop
     */
    status_t AcceptDrop(uint32_t window_id);
    
    /**
     * Cancel drag
     */
    status_t CancelDrag();
    
    // ========================================================================
    // EVENT HANDLING
    // ========================================================================
    
    /**
     * Queue an event for processing
     */
    status_t QueueEvent(uint32_t event_type, uint32_t window_id,
                       const void* event_data, size_t data_size);
    
    /**
     * Get next event from queue
     */
    status_t GetNextEvent(uint32_t* event_type, uint32_t* window_id,
                         void* event_data, size_t max_size);
    
    /**
     * Simulate mouse click
     */
    status_t SimulateMouseClick(uint32_t window_id, int32_t x, int32_t y);
    
    /**
     * Simulate key press
     */
    status_t SimulateKeyPress(uint32_t window_id, uint32_t key_code);
    
    // ========================================================================
    // SERVER UTILITIES
    // ========================================================================
    
    /**
     * Get server statistics
     */
    void GetServerStatistics(uint32_t* window_count, uint32_t* font_count,
                           uint32_t* event_count);
    
    /**
     * Dump server state for debugging
     */
    void DumpServerState();
    
private:
    // ========================================================================
    // PRIVATE METHODS
    // ========================================================================
    
    /**
     * Server thread main function
     */
    void ServerThreadMain();
    
    /**
     * Accept client connections
     */
    void AcceptClientConnections();
    
    /**
     * Handle client request
     */
    void HandleClientRequest(int client_socket);
    
    /**
     * Process window message
     */
    status_t ProcessWindowMessage(uint32_t message_type, uint32_t window_id,
                                const void* data, size_t data_size);
    
    /**
     * Process drawing message
     */
    status_t ProcessDrawingMessage(uint32_t drawing_op, uint32_t window_id,
                                 const void* data, size_t data_size);
    
    /**
     * Update host window (platform-specific)
     */
    status_t UpdateHostWindow(uint32_t window_id);
    
    /**
     * Create host window (platform-specific)
     */
    status_t CreateHostWindow(WindowInfo* window_info);
    
    /**
     * Destroy host window (platform-specific)
     */
    void DestroyHostWindow(uint32_t window_id);
    
    /**
     * Convert Haiku coordinates to host coordinates
     */
    void ConvertHaikuToHost(const void* haiku_coords, void* host_coords);
    
    /**
     * Convert host coordinates to Haiku coordinates
     */
    void ConvertHostToHaiku(const void* host_coords, void* haiku_coords);
};

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Window information structure
 */
struct WindowInfo {
    uint32_t id;
    std::string title;
    int32_t x, y;
    uint32_t width, height;
    uint32_t window_type;
    uint32_t window_look;
    uint32_t window_feel;
    uint32_t flags;
    bool visible;
    bool active;
    void* host_window;  // Platform-specific window handle
    std::unique_ptr<DrawingContext> drawing_context;
    uint32_t current_font_id;
    time_t creation_time;
    time_t last_activity;
    
    WindowInfo() : id(0), x(0), y(0), width(0), height(0),
                  window_type(WINDOW_TYPE_TITLED), window_look(WINDOW_LOOK_TITLED),
                  window_feel(WINDOW_FEEL_NORMAL), flags(0), visible(false),
                  active(false), host_window(nullptr), current_font_id(0) {
        creation_time = time(nullptr);
        last_activity = creation_time;
    }
};

/**
 * Drawing context structure
 */
struct DrawingContext {
    uint32_t window_id;
    uint32_t* frame_buffer;
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t stride;
    uint32_t current_color;
    uint32_t current_font_id;
    bool dirty;
    
    DrawingContext() : frame_buffer(nullptr), fb_width(0), fb_height(0),
                       stride(0), current_color(0xFF000000), current_font_id(0),
                       dirty(false) {}
};

/**
 * Font information structure
 */
struct FontInfo {
    uint32_t id;
    std::string family;
    std::string style;
    float size;
    uint16_t face;
    float ascent;
    float descent;
    float leading;
    void* host_font;  // Platform-specific font handle
    
    FontInfo() : id(0), size(12.0f), face(0), ascent(10.0f),
                  descent(3.0f), leading(2.0f), host_font(nullptr) {}
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

namespace HaikuAppServerUtils {
    /**
     * Convert Haiku color to host color
     */
    uint32_t HaikuColorToHost(uint32_t haiku_color);
    
    /**
     * Convert host color to Haiku color
     */
    uint32_t HostColorToHaiku(uint32_t host_color);
    
    /**
     * Get window type string for debugging
     */
    const char* GetWindowTypeString(uint32_t window_type);
    
    /**
     * Get window look string for debugging
     */
    const char* GetWindowLookString(uint32_t window_look);
    
    /**
     * Get window feel string for debugging
     */
    const char* GetWindowFeelString(uint32_t window_feel);
    
    /**
     * Parse window flags for debugging
     */
    std::string ParseWindowFlags(uint32_t flags);
}