#pragma once

#include <stdint.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <string.h>
#include <pthread.h>

// Cross-platform Haiku types
#ifndef __HAIKU__
typedef int32_t status_t;
typedef int64_t bigtime_t;
typedef struct { float left, top, right, bottom; } BRect;
typedef struct { float x, y; } BPoint;
#endif

// Haiku OS Kits and IPC System
// Complete implementation: BWindow -> libroot.so -> app_server -> syscalls -> ports -> semaphores

// Haiku port and semaphore definitions (cross-platform)
#ifndef __HAIKU__
typedef int32_t port_id;
typedef int32_t sem_id;
typedef int32_t area_id;
typedef int32_t team_id;
typedef int32_t thread_id;
#define B_OK 0
#define B_ERROR (-1)
#define B_WOULD_BLOCK (-2147483645)
#define B_TIMED_OUT (-2147483646)
#define B_NAME_TOO_LONG (-2147459073)
#define B_BAD_VALUE (-2147483647)
#define B_NO_MEMORY (-2147483648)
#define B_BAD_PORT (-2147479808)
#define B_BAD_SEM_ID (-2147479807)
#define B_DUPLICATE (-2147454947)
#define B_FILE_ERROR (-2147454948)
#define B_PERMISSION_DENIED (-2147483633)

// Port flags
#define B_PORT_READ_ONLY 1
#define B_PORT_WRITE_ONLY 2

// Semaphore types
#define B_SEMAPHORE_ACQUIRE 0
#define B_SEMAPHORE_RELEASE 1
#define B_SEMAPHORE_DELETE 2
#define B_DO_NOT_RESCHEDULE 0x400

// Area flags
#define B_READ_AREA 0x01
#define B_WRITE_AREA 0x02
#define B_EXECUTE_AREA 0x04
#define B_STACK_AREA 0x08
#define B_LOCKED_AREA 0x10

// Port capacity
#define B_PORT_MAX_CAPACITY 255
#define B_PORT_DEFAULT_CAPACITY 64

#else
typedef int32_t port_id;
typedef int32_t sem_id;
typedef int32_t area_id;
typedef int32_t team_id;
typedef int32_t thread_id;
#define B_OK 0
#define B_ERROR (-1)
#define B_WOULD_BLOCK (-2147483645)
#define B_TIMED_OUT (-2147483646)
#define B_NAME_TOO_LONG (-2147459073)
#define B_BAD_VALUE (-2147483647)
#define B_NO_MEMORY (-2147483648)
#define B_BAD_PORT (-2147479808)
#define B_BAD_SEM_ID (-2147479807)
#define B_DUPLICATE (-2147454947)
#define B_FILE_ERROR (-2147454948)
#define B_PERMISSION_DENIED (-2147483633)

// Port flags
#define B_PORT_READ_ONLY 1
#define B_PORT_WRITE_ONLY 2

// Semaphore types
#define B_SEMAPHORE_ACQUIRE 0
#define B_SEMAPHORE_RELEASE 1
#define B_SEMAPHORE_DELETE 2
#define B_DO_NOT_RESCHEDULE 0x400

// Area flags
#define B_READ_AREA 0x01
#define B_WRITE_AREA 0x02
#define B_EXECUTE_AREA 0x04
#define B_STACK_AREA 0x08
#define B_LOCKED_AREA 0x10

// Port capacity
#define B_PORT_MAX_CAPACITY 255
#define B_PORT_DEFAULT_CAPACITY 64

#endif
#define B_OK 0
#define B_ERROR (-1)
#define B_WOULD_BLOCK (-2147483645)
#define B_TIMED_OUT (-2147483646)
#define B_NAME_TOO_LONG (-2147459073)
#define B_BAD_VALUE (-2147483647)
#define B_NO_MEMORY (-2147483648)
#define B_BAD_PORT (-2147479808)
#define B_BAD_SEM_ID (-2147479807)
#define B_DUPLICATE (-2147454947)
#define B_FILE_ERROR (-2147454948)
#define B_PERMISSION_DENIED (-2147483633)

// Port flags
#define B_PORT_READ_ONLY 1
#define B_PORT_WRITE_ONLY 2

// Semaphore types
#define B_SEMAPHORE_ACQUIRE 0
#define B_SEMAPHORE_RELEASE 1
#define B_SEMAPHORE_DELETE 2
#define B_DO_NOT_RESCHEDULE 0x400

// Area flags
#define B_READ_AREA 0x01
#define B_WRITE_AREA 0x02
#define B_EXECUTE_AREA 0x04
#define B_STACK_AREA 0x08
#define B_LOCKED_AREA 0x10

// Port capacity
#define B_PORT_MAX_CAPACITY 255
#define B_PORT_DEFAULT_CAPACITY 64

#endif

// Haiku Message structures (cross-platform)
struct haiku_message_header {
    int32_t what;          // Message type/opcode
    int32_t target_port;  // Reply port
    uint32_t data_size;    // Size of data following
    int32_t sender;        // Sender team/thread
    uint64_t timestamp;    // Message timestamp
    char padding[16];      // Alignment padding
};

struct haiku_bwindow_message {
    haiku_message_header header;
    // BWindow specific fields
    int32_t window_id;
    int32_t opcode;
    BRect frame;           // Window frame rectangle
    BRect update_rect;     // Update rectangle for drawing
    int32_t modifiers;      // Keyboard modifiers
    BPoint mouse_point;    // Mouse position
    int32_t buttons;        // Mouse button state
    char title[256];        // Window title
    uint32_t look;          // Window look (B_TITLED_WINDOW, etc.)
    uint32_t feel;          // Window feel (B_NORMAL_FEEL, etc.)
    uint32_t flags;         // Window flags
    uint8_t data[1024];    // Additional message data
};

// App server communication structures
struct app_server_connection {
    port_id client_port;     // Client's port for replies
    port_id server_port;     // Server's main port
    port_id window_port;     // Window communication port
    port_id message_port;    // General message port
    sem_id draw_sem;         // Drawing synchronization
    sem_id update_sem;       // Screen update synchronization
    area_id shared_area;     // Shared memory area
    team_id server_team;     // App server team ID
    thread_id render_thread; // Rendering thread ID
    bool connected;           // Connection status
    uint8_t *shared_memory;  // Mapped shared memory
    size_t shared_size;       // Size of shared memory
};

// Framebuffer to host connection
struct host_framebuffer {
    void *host_surface;       // Host display surface
    uint32_t *pixel_data;     // Pixel data buffer
    uint32_t width;           // Framebuffer width
    uint32_t height;          // Framebuffer height
    uint32_t stride;          // Bytes per row
    uint32_t format;          // Pixel format (RGB24, RGBA32, etc.)
    bool mapped;              // Whether mapped to host
    pthread_mutex_t lock;     // Thread safety
};

// Complete Haiku OS IPC System
class HaikuOSIPCSystem {
public:
    HaikuOSIPCSystem();
    ~HaikuOSIPCSystem();
    
    // Core initialization
    bool Initialize();
    void Shutdown();
    
    // Port-based IPC
    int32_t CreatePort(int32_t capacity, const char *name);
    int32_t WritePort(int32_t port, int32_t msg_code, const void *buffer, size_t size);
    int32_t ReadPort(int32_t port, int32_t *msg_code, void *buffer, size_t size, int64_t timeout);
    int32_t ClosePort(int32_t port);
    
    // Semaphore operations
    int32_t CreateSemaphore(int32 count, const char *name);
    int32_t AcquireSemaphore(int32_t sem);
    int32_t ReleaseSemaphore(int32_t sem);
    int32_t DeleteSemaphore(int32_t sem);
    
    // Area management
    int32_t CreateArea(const char *name, void **address, size_t size, uint32_t flags, uint32_t protection);
    int32_t DeleteArea(int32_t area);
    
    // Dynamic library loading
    void *LoadLibrary(const char *path);
    void *GetSymbol(void *library, const char *symbol);
    void UnloadLibrary(void *library);
    
    // Audio Kit integration
    int32_t InitializeAudio();
    int32_t CreateAudioBuffer(int32_t sample_rate, int32_t channels, int32_t buffer_size);
    int32_t WriteAudioSamples(const int16_t *samples, size_t count);
    int32_t SetAudioVolume(float volume);
    
    // Complete app_server pipeline
    int32_t ConnectToAppServer(app_server_connection *conn);
    int32_t CreateWindowInAppServer(app_server_connection *conn, const char *title, 
                                      BRect frame, uint32_t look, uint32_t feel, uint32_t flags);
    int32_t UpdateWindowInAppServer(app_server_connection *conn, uint32_t window_id, 
                                     BRect update_rect, void *bitmap_data);
    int32_t SendMouseEventToAppServer(app_server_connection *conn, uint32_t window_id, 
                                        BPoint point, int32_t buttons, int32_t modifiers);
    int32_t SendKeyboardEventToAppServer(app_server_connection *conn, uint32_t window_id, 
                                          int32_t key_code, int32_t modifiers, bool key_down);
    
    // Host framebuffer connection
    int32_t ConnectToHostFramebuffer(host_framebuffer *fb, uint32_t width, uint32_t height);
    int32_t UpdateHostFramebuffer(host_framebuffer *fb, const void *pixel_data, 
                                    uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    
    // Main app_server simulation
    int32_t StartAppServer();
    int32_t StopAppServer();
    
private:
    // Port management
    std::map<int32_t, std::string> port_names;
    std::mutex port_mutex;
    
    // Semaphore management  
    std::map<int32_t, std::string> semaphore_names;
    std::mutex semaphore_mutex;
    
    // Area management
    std::map<int32_t, void*> mapped_areas;
    std::mutex area_mutex;
    int32_t g_next_area_id = 3000;
    
    // Dynamic libraries
    std::map<void*, std::string> loaded_libraries;
    std::mutex library_mutex;
    
    // App server state
    bool app_server_running;
    std::unique_ptr<std::thread> app_server_thread;
    int32_t app_server_main_port;
    std::map<int32_t, app_server_connection> connections;
    std::mutex connection_mutex;
    
    // Audio system
    std::atomic<bool> audio_initialized;
    void *audio_device;
    std::atomic<float> audio_volume;
    std::unique_ptr<std::thread> audio_thread;
    
    // Host framebuffer
    host_framebuffer host_fb;
    bool host_fb_connected;
    std::unique_ptr<std::thread> fb_thread;
    
    // Internal methods
    void AppServerMainLoop();
    void ProcessAppServerMessage(port_id port, haiku_bwindow_message *msg);
    int32_t HandleCreateWindowMessage(int32_t reply_port, haiku_bwindow_message *msg);
    int32_t HandleUpdateWindowMessage(int32_t reply_port, haiku_bwindow_message *msg);
    int32_t HandleMouseEventMessage(int32_t reply_port, haiku_bwindow_message *msg);
    int32_t HandleKeyboardEventMessage(int32_t reply_port, haiku_bwindow_message *msg);
    
    void AudioProcessingLoop();
    void FramebufferUpdateLoop();
    
    // Cross-platform compatibility
    bool FindHaikuAppServer();
    int32_t LocateAndLoadLibroot();
    void* haiku_libroot_handle;
    
    // Function pointers to libroot.so functions
    typedef int32_t (*be_app_server_connect_func)(int32_t*, const char*);
    typedef int32_t (*be_window_create_func)(int32_t, const char*, BRect, uint32_t, uint32_t, uint32_t, int32_t*);
    typedef int32_t (*be_window_update_func)(int32_t, int32_t, BRect, void*);
    typedef int32_t (*be_window_mouse_func)(int32_t, int32_t, BPoint, int32_t, int32_t);
    typedef int32_t (*be_window_keyboard_func)(int32_t, int32_t, int32_t, int32_t, bool);
    
    be_app_server_connect_func be_app_server_connect;
    be_window_create_func be_window_create;
    be_window_update_func be_window_update;
    be_window_mouse_func be_window_mouse;
    be_window_keyboard_func be_window_keyboard;
    
    // Message constants (from Haiku's app_server)
    enum {
        AS_CREATE_WINDOW = 0x43524557, // 'CREW'
        AS_DELETE_WINDOW = 0x44454c57, // 'DELW'
        AS_UPDATE_WINDOW = 0x55504457, // 'UPDW'
        AS_MOUSE_MOVED = 0x4d6f7573, // 'Mous'
        AS_MOUSE_DOWN = 0x4d444f57,  // 'MDOW'
        AS_MOUSE_UP = 0x4d555057,   // 'MUPW'
        AS_KEY_DOWN = 0x4b44574f,   // 'KDWO'
        AS_KEY_UP = 0x4b55574f,    // 'KUWO'
        AS_WINDOW_ACTIVATED = 0x57414354, // 'WACT'
        AS_WINDOW_DEACTIVATED = 0x57444154, // 'WDAV'
        AS_WINDOW_RESIZED = 0x57524953, // 'WRIS'
        AS_QUIT_REQUESTED = 0x51554954, // 'QUIT'
        AS_WINDOW_MOVED = 0x574d4f56, // 'WMOV'
        AS_SCREEN_CHANGED = 0x53434847  // 'SCHG'
    };
    
    // App server to libroot.so communication
    int32_t CallLibrootFunction(const char* function_name, void* args, size_t arg_size, void* result, size_t result_size);
};