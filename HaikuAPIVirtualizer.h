/*
 * HaikuAPIVirtualizer.h - Haiku/BeOS API Virtualization Layer
 * 
 * Complete Haiku API implementation for cross-platform execution
 * Provides all Haiku Kits: Interface, Storage, Application, Support, etc.
 */

#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <mutex>

// Forward declarations for Haiku API classes
class HaikuApplicationServer;
class HaikuSyscallDispatcher;
class BApplication;
class BWindow;
class BView;

// Haiku API Virtualizer Status Codes
typedef int32_t status_t;
#ifndef B_OK
#define B_OK                    0
#endif
#ifndef B_ERROR
#define B_ERROR                 (-1)
#endif
#ifndef B_NO_MEMORY
#define B_NO_MEMORY             (-2147483646)
#endif
#ifndef B_BAD_VALUE
#define B_BAD_VALUE             (-2147483647)
#endif
#ifndef B_NO_INIT
#define B_NO_INIT               (-2147483645)
#endif

// ============================================================================
// HAIKU API VIRTUALIZER CORE
// ============================================================================

/**
 * Main Haiku API Virtualizer class
 * 
 * This class provides the complete Haiku API implementation that allows
 * any Haiku/BeOS application to run on any host platform.
 * 
 * Architecture:
 * Guest Haiku App → libbe.so → HaikuAPIVirtualizer → Host OS
 */
class HaikuAPIVirtualizer {
public:
    /**
     * Initialize the Haiku API Virtualizer
     * Sets up all Haiku Kits and application server
     */
    virtual status_t Initialize() = 0;
    
    /**
     * Shutdown the virtualizer
     * Clean up all resources and stop application server
     */
    virtual void Shutdown() = 0;
    
    /**
     * Check if virtualizer is initialized
     */
    virtual bool IsInitialized() const = 0;
    
    /**
     * Get application server instance
     */
    virtual HaikuApplicationServer* GetApplicationServer() = 0;
    
    /**
     * Get syscall dispatcher instance
     */
    virtual HaikuSyscallDispatcher* GetSyscallDispatcher() = 0;
    
    virtual ~HaikuAPIVirtualizer() = default;
};

/**
 * Factory for creating Haiku API Virtualizer instances
 */
class HaikuAPIVirtualizerFactory {
public:
    /**
     * Create a new Haiku API Virtualizer instance
     * 
     * @param host_platform Target host platform (linux, windows, macos, haiku)
     * @return Configured virtualizer instance
     */
    static std::unique_ptr<HaikuAPIVirtualizer> CreateVirtualizer(
        const std::string& host_platform = "auto");
    
    /**
     * Detect host platform automatically
     */
    static std::string DetectHostPlatform();
    
    /**
     * Get supported host platforms
     */
    static std::vector<std::string> GetSupportedPlatforms();
};

// ============================================================================
// HAIKU KITS IMPLEMENTATION
// ============================================================================

/**
 * Base class for all Haiku Kit implementations
 */
class HaikuKit {
protected:
    std::string kit_name;
    bool initialized;
    std::mutex kit_mutex;
    
public:
    explicit HaikuKit(const std::string& name) 
        : kit_name(name), initialized(false) {}
    
    virtual ~HaikuKit() = default;
    
    virtual status_t Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsInitialized() const { return initialized; }
    
    const std::string& GetKitName() const { return kit_name; }
};

/**
 * Interface Kit Implementation
 * 
 * Provides: BWindow, BView, BControl, BButton, BTextView, etc.
 */
class HaikuInterfaceKit : public HaikuKit {
public:
    HaikuInterfaceKit() : HaikuKit("Interface Kit") {}
    
    // Window management
    virtual uint32_t CreateWindow(const char* title, uint32_t x, uint32_t y, 
                                uint32_t width, uint32_t height) = 0;
    virtual status_t ShowWindow(uint32_t window_id) = 0;
    virtual status_t HideWindow(uint32_t window_id) = 0;
    virtual void DestroyWindow(uint32_t window_id) = 0;
    
    // Drawing operations
    virtual status_t DrawLine(uint32_t window_id, int32_t x1, int32_t y1, 
                             int32_t x2, int32_t y2, uint32_t color) = 0;
    virtual status_t FillRect(uint32_t window_id, int32_t x, int32_t y, 
                             uint32_t width, uint32_t height, uint32_t color) = 0;
    virtual status_t DrawString(uint32_t window_id, int32_t x, int32_t y, 
                               const char* string, uint32_t color) = 0;
    virtual status_t Flush(uint32_t window_id) = 0;
    
    // View operations
    virtual status_t AddChild(uint32_t window_id, uint32_t parent_view_id, 
                             uint32_t child_view_id) = 0;
    virtual status_t RemoveChild(uint32_t window_id, uint32_t view_id) = 0;
    
    static HaikuInterfaceKit& GetInstance();
};

/**
 * Storage Kit Implementation
 * 
 * Provides: BFile, BDirectory, BEntry, BPath, BVolume, BQuery
 */
class HaikuStorageKit : public HaikuKit {
public:
    HaikuStorageKit() : HaikuKit("Storage Kit") {}
    
    // File operations
    virtual uint32_t OpenFile(const char* path, uint32_t mode) = 0;
    virtual status_t CloseFile(uint32_t file_id) = 0;
    virtual ssize_t ReadFile(uint32_t file_id, void* buffer, size_t size) = 0;
    virtual ssize_t WriteFile(uint32_t file_id, const void* buffer, size_t size) = 0;
    virtual status_t SeekFile(uint32_t file_id, off_t position, uint32_t seek_mode) = 0;
    virtual status_t SetFileSize(uint32_t file_id, off_t size) = 0;
    
    // Directory operations
    virtual uint32_t OpenDirectory(const char* path) = 0;
    virtual status_t CloseDirectory(uint32_t dir_id) = 0;
    virtual status_t ReadDirectory(uint32_t dir_id, char* name, size_t size) = 0;
    virtual status_t RewindDirectory(uint32_t dir_id) = 0;
    
    // Entry operations
    virtual status_t GetEntryInfo(const char* path, void* info) = 0;
    virtual status_t CreateEntry(const char* path, uint32_t type) = 0;
    virtual status_t DeleteEntry(const char* path) = 0;
    virtual status_t MoveEntry(const char* old_path, const char* new_path) = 0;
    
    // Path operations
    virtual status_t GetAbsolutePath(const char* path, char* abs_path, size_t size) = 0;
    virtual status_t GetParentPath(const char* path, char* parent_path, size_t size) = 0;
    
    static HaikuStorageKit& GetInstance();
};

/**
 * Application Kit Implementation
 * 
 * Provides: BApplication, BLooper, BMessenger, BHandler, BMessage
 */
class HaikuApplicationKit : public HaikuKit {
public:
    HaikuApplicationKit() : HaikuKit("Application Kit") {}
    
    // Application management
    virtual status_t CreateApplication(const char* signature) = 0;
    virtual status_t RunApplication() = 0;
    virtual status_t QuitApplication() = 0;
    virtual bool IsApplicationRunning() = 0;
    
    // Message handling
    virtual uint32_t CreateMessage() = 0;
    virtual status_t SendMessage(uint32_t message_id, uint32_t target) = 0;
    virtual status_t PostMessage(uint32_t message_id, uint32_t target) = 0;
    
    // Looper management
    virtual uint32_t CreateLooper(const char* name) = 0;
    virtual status_t RunLooper(uint32_t looper_id) = 0;
    virtual status_t QuitLooper(uint32_t looper_id) = 0;
    
    // Handler management
    virtual uint32_t CreateHandler() = 0;
    virtual status_t AddHandler(uint32_t looper_id, uint32_t handler_id) = 0;
    virtual status_t RemoveHandler(uint32_t looper_id, uint32_t handler_id) = 0;
    
    static HaikuApplicationKit& GetInstance();
};

/**
 * Support Kit Implementation
 * 
 * Provides: BString, BList, BObjectList, BLocker, BPoint, BRect
 */
class HaikuSupportKit : public HaikuKit {
public:
    HaikuSupportKit() : HaikuKit("Support Kit") {}
    
    // String operations
    virtual uint32_t CreateString(const char* string) = 0;
    virtual status_t SetString(uint32_t string_id, const char* string) = 0;
    virtual status_t GetString(uint32_t string_id, char* buffer, size_t size) = 0;
    virtual void DeleteString(uint32_t string_id) = 0;
    
    // List operations
    virtual uint32_t CreateList() = 0;
    virtual status_t AddItem(uint32_t list_id, void* item) = 0;
    virtual status_t RemoveItem(uint32_t list_id, int32_t index) = 0;
    virtual void* GetItem(uint32_t list_id, int32_t index) = 0;
    virtual int32_t CountItems(uint32_t list_id) = 0;
    virtual void DeleteList(uint32_t list_id) = 0;
    
    // Geometry operations
    virtual status_t CreatePoint(int32_t x, int32_t y, void* point) = 0;
    virtual status_t CreateRect(int32_t left, int32_t top, 
                              int32_t right, int32_t bottom, void* rect) = 0;
    virtual status_t IntersectRect(const void* rect1, const void* rect2, void* result) = 0;
    virtual status_t UnionRect(const void* rect1, const void* rect2, void* result) = 0;
    
    static HaikuSupportKit& GetInstance();
};

/**
 * Network Kit Implementation
 * 
 * Provides: BNetAddress, BNetBuffer, BNetEndpoint, BUrl, BHttpRequest
 */
class HaikuNetworkKit : public HaikuKit {
public:
    HaikuNetworkKit() : HaikuKit("Network Kit") {}
    
    // Socket operations
    virtual uint32_t CreateSocket(uint32_t domain, uint32_t type, uint32_t protocol) = 0;
    virtual status_t ConnectSocket(uint32_t socket_id, const char* address, uint16_t port) = 0;
    virtual status_t BindSocket(uint32_t socket_id, const char* address, uint16_t port) = 0;
    virtual status_t ListenSocket(uint32_t socket_id, int32_t backlog) = 0;
    virtual uint32_t AcceptSocket(uint32_t socket_id, char* client_address, uint16_t* port) = 0;
    virtual status_t CloseSocket(uint32_t socket_id) = 0;
    
    // Data transfer
    virtual ssize_t SendSocket(uint32_t socket_id, const void* buffer, size_t size, 
                             uint32_t flags) = 0;
    virtual ssize_t ReceiveSocket(uint32_t socket_id, void* buffer, size_t size, 
                                uint32_t flags) = 0;
    
    // HTTP operations
    virtual uint32_t CreateHttpRequest(const char* url) = 0;
    virtual status_t ExecuteHttpRequest(uint32_t request_id) = 0;
    virtual status_t GetHttpResponse(uint32_t request_id, char* response, size_t size) = 0;
    virtual void DeleteHttpRequest(uint32_t request_id) = 0;
    
    // DNS operations
    virtual status_t ResolveHost(const char* hostname, char* address, size_t size) = 0;
    virtual status_t ReverseResolve(const char* address, char* hostname, size_t size) = 0;
    
    static HaikuNetworkKit& GetInstance();
};

/**
 * Media Kit Implementation
 * 
 * Provides: BSoundPlayer, BSoundRecorder, BMediaNode, BMediaRoster
 */
class HaikuMediaKit : public HaikuKit {
public:
    HaikuMediaKit() : HaikuKit("Media Kit") {}
    
    // Audio playback
    virtual uint32_t CreateSoundPlayer(uint32_t format, uint32_t sample_rate, 
                                     uint32_t channels) = 0;
    virtual status_t StartSoundPlayer(uint32_t player_id) = 0;
    virtual status_t StopSoundPlayer(uint32_t player_id) = 0;
    virtual status_t SetSoundPlayerVolume(uint32_t player_id, float volume) = 0;
    virtual void DeleteSoundPlayer(uint32_t player_id) = 0;
    
    // Audio recording
    virtual uint32_t CreateSoundRecorder(uint32_t format, uint32_t sample_rate, 
                                       uint32_t channels) = 0;
    virtual status_t StartSoundRecorder(uint32_t recorder_id) = 0;
    virtual status_t StopSoundRecorder(uint32_t recorder_id) = 0;
    virtual void DeleteSoundRecorder(uint32_t recorder_id) = 0;
    
    // Media node operations
    virtual uint32_t CreateMediaNode(const char* node_type) = 0;
    virtual status_t ConnectMediaNodes(uint32_t source_node, uint32_t dest_node) = 0;
    virtual status_t DisconnectMediaNodes(uint32_t source_node, uint32_t dest_node) = 0;
    virtual void DeleteMediaNode(uint32_t node_id) = 0;
    
    static HaikuMediaKit& GetInstance();
};

// ============================================================================
// CONFIGURATION AND UTILITIES
// ============================================================================

/**
 * Configuration for Haiku API Virtualizer
 */
struct HaikuVirtualizerConfig {
    std::string host_platform;
    bool enable_gui;
    bool enable_sound;
    bool enable_network;
    bool enable_media;
    uint32_t memory_size;
    std::string working_directory;
    
    HaikuVirtualizerConfig() 
        : host_platform("auto"), enable_gui(true), enable_sound(true),
          enable_network(true), enable_media(true), memory_size(128 * 1024 * 1024) {}
};

/**
 * Utility functions for Haiku API Virtualizer
 */
namespace HaikuAPIUtils {
    /**
     * Convert Haiku path to host path
     */
    std::string ConvertHaikuPathToHost(const std::string& haiku_path);
    
    /**
     * Convert host path to Haiku path
     */
    std::string ConvertHostPathToHaiku(const std::string& host_path);
    
    /**
     * Get Haiku system directory
     */
    std::string GetHaikuSystemDirectory();
    
    /**
     * Get Haiku user directory
     */
    std::string GetHaikuUserDirectory();
    
    /**
     * Convert Haiku error code to string
     */
    std::string ErrorToString(status_t error);
    
    /**
     * Log Haiku API calls (for debugging)
     */
    void LogAPICall(const std::string& kit, const std::string& function, 
                    const std::string& parameters);
}