#pragma once

#include "PlatformTypes.h"
#include <cstdint>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <string>
#include <vector>
#include <condition_variable>

// Haiku OS Kits Unified Integration
// Optimized emulation without redundancies

#ifdef __HAIKU__
// Real Haiku OS includes
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <InterfaceKit.h>
#include <MediaKit.h>
#include <NetAddress.h>
#include <NetEndpoint.h>
#include <NetDebug.h>
#include <StorageKit.h>
#include <SupportKit.h>
#else
// Haiku OS stubs for non-Haiku systems
#define B_OK 0
#define B_ERROR (-1)
#define B_TIMED_OUT (-2147483646)
#define B_BAD_VALUE (-2147483647)
#define B_NO_MEMORY (-2147483648)

// Forward declarations for stub classes
class BApplication;
class BWindow;
class BView;
class BSoundPlayer;
class BNetAddress;
class BNetEndpoint;
#endif

// Unified Haiku OS Kits System
class HaikuOSKitsSystem {
public:
    // Singleton pattern for unified access
    static HaikuOSKitsSystem& Instance() {
        static HaikuOSKitsSystem instance;
        return instance;
    }
    
    // Initialization
    status_t Initialize();
    void Shutdown();
    
    // InterfaceKit (GUI) integration
    class InterfaceKit {
    public:
        struct Window {
            int32_t window_id;
            char title[256];
            uint32_t width, height;
            uint32_t x, y;
            bool visible, focused, minimized;
            uint32_t bg_color, fg_color;
            void *native_window; // Real BWindow pointer
        };
        
        struct Bitmap {
            int32_t bitmap_id;
            uint32_t width, height;
            uint32_t bytes_per_row;
            uint8_t *bits;
            void *native_bitmap; // Real BBitmap pointer
        };
        
        bool CreateWindow(const char *title, uint32_t width, uint32_t height, 
                          uint32_t x, uint32_t y, int32_t *window_id);
        bool DestroyWindow(int32_t window_id);
        bool DrawLine(int32_t window_id, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
        bool DrawRect(int32_t window_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
        bool FillRect(int32_t window_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
        bool DrawString(int32_t window_id, uint32_t x, uint32_t y, const char *text, uint32_t length);
        bool SetColor(int32_t window_id, uint32_t color);
        bool Flush(int32_t window_id);
        
    private:
        std::map<int32_t, Window> windows;
        std::map<int32_t, Bitmap> bitmaps;
        std::mutex gui_mutex;
        int32_t next_window_id;
        int32_t next_bitmap_id;
        
        #ifdef __HAIKU__
        BApplication *app;
        #endif
    };
    
    // MediaKit (Audio) integration
    class MediaKit {
    public:
        struct AudioFormat {
            float frame_rate;
            uint32_t channel_count;
            uint32_t format; // B_AUDIO_SHORT, etc.
            uint32_t byte_order;
            size_t buffer_size;
        };
        
        bool InitializeAudio();
        bool CreateAudioBuffer(uint32_t sample_rate, uint32_t channels, uint32_t buffer_size);
        bool WriteAudioSamples(const int16_t *samples, size_t count);
        bool SetAudioVolume(float volume);
        void CleanupAudio();
        
    private:
        std::atomic<bool> audio_initialized;
        std::atomic<float> audio_volume;
        AudioFormat audio_format;
        std::mutex audio_mutex;
        
        #ifdef __HAIKU__
        BSoundPlayer *sound_player;
        #endif
    };
    
    // NetworkKit (Internet) integration
    class NetworkKit {
    public:
        struct NetworkConnection {
            int32_t conn_id;
            char host[256];
            uint16_t port;
            bool connected;
            uint32_t timeout_ms;
            void *native_endpoint; // Real BNetEndpoint pointer
        };
        
        struct NetworkAddress {
            char hostname[256];
            uint32_t ip_address; // Network byte order
            uint16_t port;
            bool resolved;
        };
        
        bool InitializeNetwork();
        bool CreateConnection(const char *host, uint16_t port, int32_t *conn_id);
        bool Connect(int32_t conn_id);
        bool Disconnect(int32_t conn_id);
        bool SendData(int32_t conn_id, const void *data, size_t size);
        bool ReceiveData(int32_t conn_id, void *buffer, size_t size, size_t *bytes_received);
        bool ResolveAddress(const char *hostname, NetworkAddress *address);
        void CleanupNetwork();
        
        // Syscall handling
        bool HandleNetworkSyscall(uint32_t syscall_num, uint32_t *args, uint32_t *result);
        
    private:
        std::map<int32_t, NetworkConnection> connections;
        std::mutex network_mutex;
        int32_t next_conn_id;
        std::atomic<bool> network_initialized;
    };
    
    // StorageKit integration
    class StorageKit {
    public:
        bool CreateFile(const char *path, const void *data, size_t size);
        bool ReadFile(const char *path, void *buffer, size_t size, size_t *bytes_read);
        bool DeleteFile(const char *path);
        bool DirectoryExists(const char *path);
        bool CreateDirectory(const char *path);
        
    private:
        std::mutex storage_mutex;
    };
    
    // SupportKit integration
    class SupportKit {
    public:
        void *AllocateMemory(size_t size);
        void FreeMemory(void *ptr);
        bool CopyMemory(void *dest, const void *src, size_t size);
        bool SetMemory(void *ptr, uint8_t value, size_t size);
        
    private:
        std::mutex memory_mutex;
    };
    
    // Access to individual kits
    InterfaceKit& GetInterfaceKit() { return interface_kit; }
    MediaKit& GetMediaKit() { return media_kit; }
    NetworkKit& GetNetworkKit() { return network_kit; }
    StorageKit& GetStorageKit() { return storage_kit; }
    SupportKit& GetSupportKit() { return support_kit; }
    
    // Unified syscall handling
    bool HandleHaikuSyscall(uint32_t kit_id, uint32_t syscall_num, uint32_t *args, uint32_t *result);
    
    // Kit IDs for syscall routing
    static constexpr uint32_t KIT_INTERFACE = 1;
    static constexpr uint32_t KIT_MEDIA = 2;
    static constexpr uint32_t KIT_NETWORK = 3;
    static constexpr uint32_t KIT_STORAGE = 4;
    static constexpr uint32_t KIT_SUPPORT = 5;

private:
    HaikuOSKitsSystem() : initialized(false) {}
    ~HaikuOSKitsSystem() { Shutdown(); }
    
    // Individual kit instances
    InterfaceKit interface_kit;
    MediaKit media_kit;
    NetworkKit network_kit;
    StorageKit storage_kit;
    SupportKit support_kit;
    
    std::atomic<bool> initialized;
    std::mutex system_mutex;
    
    // Prevent copying
    HaikuOSKitsSystem(const HaikuOSKitsSystem&) = delete;
    HaikuOSKitsSystem& operator=(const HaikuOSKitsSystem&) = delete;
};

// Global access macros for convenience
#define HAIKU_INTERFACE HaikuOSKitsSystem::Instance().GetInterfaceKit()
#define HAIKU_MEDIA HaikuOSKitsSystem::Instance().GetMediaKit()
#define HAIKU_NETWORK HaikuOSKitsSystem::Instance().GetNetworkKit()
#define HAIKU_STORAGE HaikuOSKitsSystem::Instance().GetStorageKit()
#define HAIKU_SUPPORT HaikuOSKitsSystem::Instance().GetSupportKit()