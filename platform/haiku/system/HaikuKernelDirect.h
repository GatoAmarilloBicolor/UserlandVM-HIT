/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * HaikuKernelDirect.h - Direct Haiku kernel integration bypassing emulation
 */

#ifndef HAIKU_KERNEL_DIRECT_H
#define HAIKU_KERNEL_DIRECT_H

#include <SupportDefs.h>
#include <OS.h>
#include <syscalls.h>
#include <kernel/OS.h>
#include <kernel/fs_info.h>
#include <kernel/image.h>
#include <kernel/thread.h>
#include <kernel/sem.h>
#include <kernel/port.h>
#include <kernel/area.h>
#include <file_io.h>
#include <fs_interface.h>

class HaikuKernelDirect {
public:
    // Direct kernel syscall interface
    enum DirectMode {
        DIRECT_MODE_OFF = 0,      // Use current emulation
        DIRECT_MODE_AUTO = 1,      // Auto-detect when possible
        DIRECT_MODE_FORCE = 2,     // Force direct mode
        DIRECT_MODE_HYBRID = 3    // Hybrid mode with optimization
    };

private:
    DirectMode fMode;
    bool fIsNativeHaiku;
    bool fKernelDirectAvailable;
    
    // File system direct interface
    struct DirectFileHandle {
        int kernel_fd;         // Actual Haiku kernel FD
        void* haiku_file;      // Haiku file structure pointer
        char* path_buffer;      // Cached path
        uint32_t path_hash;     // Path hash for fast lookup
        uint32_t open_flags;    // Original open flags
        uint32_t open_mode;     // Original open mode
        off_t current_pos;      // Current file position
        bool is_seekable;        // Can this file be seeked
        bool is_writable;        // Can this file be written
        
        DirectFileHandle() : kernel_fd(-1), haiku_file(nullptr), path_buffer(nullptr),
                           path_hash(0), open_flags(0), open_mode(0), 
                           current_pos(0), is_seekable(false), is_writable(false) {}
    };

    // Memory management direct interface
    struct DirectMemoryArea {
        area_id area_id;         // Haiku area ID
        void* base_address;      // Base address of area
        size_t size;             // Size of area
        uint32_t protection;      // Memory protection flags
        char* name;             // Area name
        bool is_shared;          // Is this a shared area
        int ref_count;           // Reference count
        
        DirectMemoryArea() : area_id(-1), base_address(nullptr), size(0),
                           protection(0), name(nullptr), is_shared(false), ref_count(0) {}
    };

    // Threading direct interface
    struct DirectThread {
        thread_id thread_id;     // Haiku thread ID
        void* thread_object;     // Thread object pointer
        int32_t thread_priority;  // Thread priority
        uint32_t stack_size;     // Thread stack size
        void* entry_point;       // Thread entry point
        void* argument;          // Thread argument
        bool is_running;         // Thread running state
        bool is_suspended;       // Thread suspended state
        
        DirectThread() : thread_id(-1), thread_object(nullptr), thread_priority(0),
                        stack_size(0), entry_point(nullptr), argument(nullptr),
                        is_running(false), is_suspended(false) {}
    };

    // IPC direct interface
    struct DirectPort {
        port_id port_id;        // Haiku port ID
        void* port_object;       // Port object pointer
        int32_t capacity;        // Port capacity
        uint32_t message_count;  // Current message count
        char* name;              // Port name
        bool is_read_only;        // Read-only port
        int32_t total_messages;   // Total messages processed
        
        DirectPort() : port_id(-1), port_object(nullptr), capacity(0), 
                       message_count(0), name(nullptr), is_read_only(false), 
                       total_messages(0) {}
    };

    // Direct handles tables
    static constexpr int MAX_DIRECT_FILES = 1024;
    static constexpr int MAX_DIRECT_AREAS = 512;
    static constexpr int MAX_DIRECT_THREADS = 256;
    static constexpr int MAX_DIRECT_PORTS = 128;
    
    DirectFileHandle fFileHandles[MAX_DIRECT_FILES];
    DirectMemoryArea fMemoryAreas[MAX_DIRECT_AREAS];
    DirectThread fThreads[MAX_DIRECT_THREADS];
    DirectPort fPorts[MAX_DIRECT_PORTS];
    
    // Performance tracking
    struct DirectMetrics {
        uint64_t direct_calls_made;
        uint64_t emulation_calls_saved;
        uint64_t kernel_calls_direct;
        uint64_t file_operations_direct;
        uint64_t memory_operations_direct;
        uint64_t thread_operations_direct;
        uint64_t ipc_operations_direct;
        double avg_direct_call_time_us;
        double performance_improvement_factor;
        
        DirectMetrics() : direct_calls_made(0), emulation_calls_saved(0),
                           kernel_calls_direct(0), file_operations_direct(0),
                           memory_operations_direct(0), thread_operations_direct(0),
                           ipc_operations_direct(0), avg_direct_call_time_us(0.0),
                           performance_improvement_factor(1.0) {}
    };

    DirectMetrics fMetrics;

public:
    HaikuKernelDirect();
    ~HaikuKernelDirect();
    
    // Mode control
    bool Initialize(DirectMode mode = DIRECT_MODE_AUTO);
    void SetMode(DirectMode mode);
    DirectMode GetMode() const { return fMode; }
    bool IsDirectModeAvailable() const { return fKernelDirectAvailable; }
    
    // Direct file operations (bypassing emulation)
    status_t DirectOpenFile(const char* path, int flags, mode_t mode, int* fd);
    status_t DirectReadFile(int fd, off_t pos, void* buffer, size_t* bytesRead);
    status_t DirectWriteFile(int fd, off_t pos, const void* buffer, size_t bytesToWrite, size_t* bytesWritten);
    status_t DirectCloseFile(int fd);
    status_t DirectSeekFile(int fd, off_t pos, int whence, off_t* newPos);
    status_t DirectStatFile(const char* path, struct stat* st);
    status_t DirectDeleteFile(const char* path);
    
    // Direct memory operations (using Haiku areas)
    status_t DirectCreateArea(const char* name, void** address, uint32_t addressSpec,
                            size_t size, uint32_t lock, uint32_t protection, 
                            area_id* area);
    status_t DirectDeleteArea(area_id area);
    status_t DirectResizeArea(area_id area, size_t newSize);
    status_t DirectSetAreaProtection(area_id area, uint32_t protection);
    status_t DirectCloneArea(area_id sourceArea, const char* name, void** address,
                            uint32_t addressSpec, area_id* newArea);
    
    // Direct threading operations
    status_t DirectCreateThread(thread_func function, void* argument, const char* name,
                             int32_t priority, size_t stackSize, thread_id* threadId);
    status_t DirectKillThread(thread_id thread, int signal);
    status_t DirectSuspendThread(thread_id thread);
    status_t DirectResumeThread(thread_id thread);
    status_t DirectExitThread(status_t status);
    status_t DirectWaitForThread(thread_id thread, status_t* exitCode, uint32_t flags);
    
    // Direct IPC operations (using Haiku ports)
    status_t DirectCreatePort(int32_t capacity, const char* name, port_id* port);
    status_t DirectDeletePort(port_id port);
    status_t DirectWritePort(port_id port, int32_t code, const void* buffer, size_t bufferSize);
    status_t DirectReadPort(port_id port, void* buffer, size_t bufferSize, int32_t* code);
    status_t DirectSendPort(port_id port, int32_t code, const void* buffer, size_t bufferSize);
    status_t DirectReceivePort(port_id port, void* buffer, size_t bufferSize, int32_t* code);
    
    // GUI operations (direct to AppServer)
    status_t DirectCreateWindow(const char* title, int32_t left, int32_t top, 
                             int32_t width, int32_t height, uint32_t flags, void** window);
    status_t DirectShowWindow(void* window);
    status_t DirectHideWindow(void* window);
    status_t DirectDrawRect(void* window, int32_t x, int32_t y, int32_t width, int32_t height, rgb_color color);
    status_t DirectInvalidateWindow(void* window, const BRect* rect);
    
    // Process operations (direct kernel)
    status_t DirectLoadImage(const char* path, uint32_t flags, image_id* image);
    status_t DirectUnloadImage(image_id image);
    status_t DirectGetImageInfo(image_id image, image_info* info);
    status_t DirectCreateTeam(const char* name, const char* path, char* const* argv,
                           char* const* envp, team_id* team);
    
    // Direct system information
    status_t DirectSystemInfo(system_info* info);
    status_t DirectKernelInfo(kernel_info* info);
    status_t DirectCPUInfo(cpu_info* info);
    
    // Advanced direct operations
    status_t DirectSuspendThreadByThreadID(thread_id thread, const void* message);
    status_t DirectResumeThreadByThreadID(thread_id thread);
    status_t DirectSendSignalToTeam(team_id team, uint32_t signal);
    status_t DirectSetThreadPriority(thread_id thread, int32_t priority);
    
    // Performance and monitoring
    DirectMetrics GetMetrics() const { return fMetrics; }
    void ResetMetrics();
    void PrintMetrics() const;
    
    // Resource management
    void CleanupResources();
    void CleanupFileHandles();
    void CleanupMemoryAreas();
    void CleanupThreads();
    void CleanupPorts();

private:
    // Initialization helpers
    bool DetectNativeHaiku();
    bool InitializeKernelInterface();
    bool SetupDirectHandles();
    
    // Handle management
    int AllocateFileHandle();
    int AllocateMemoryArea();
    int AllocateThread();
    int AllocatePort();
    
    void FreeFileHandle(int fd);
    void FreeMemoryArea(area_id area);
    void FreeThread(thread_id thread);
    void FreePort(port_id port);
    
    // Validation helpers
    bool IsValidFileHandle(int fd) const;
    bool IsValidMemoryArea(area_id area) const;
    bool IsValidThread(thread_id thread) const;
    bool IsValidPort(port_id port) const;
    
    // Path and name handling
    char* AllocatePath(const char* path);
    void FreePath(char* path);
    uint32_t HashPath(const char* path) const;
    
    // Direct kernel calls (bypassing all emulation)
    status_t CallKernelOpen(const char* path, int flags, mode_t mode);
    status_t CallKernelRead(int fd, void* buffer, size_t count, ssize_t* bytesRead);
    status_t CallKernelWrite(int fd, const void* buffer, size_t count, ssize_t* bytesWritten);
    status_t CallKernelClose(int fd);
    status_t CallKernelSeek(int fd, off_t offset, int whence, off_t* result);
    
    status_t CallKernelCreateArea(const char* name, void** addr, uint32_t addressSpec,
                                size_t size, uint32_t lock, uint32_t protection);
    status_t CallKernelDeleteArea(area_id area);
    
    status_t CallKernelSpawnThread(thread_func function, void* arg, const char* name,
                                 int32_t priority, size_t stackSize, thread_id* thread);
    status_t CallKernelKillThread(thread_id thread, int signal);
    
    status_t CallKernelCreatePort(int32_t capacity, const char* name);
    status_t CallKernelDeletePort(port_id port);
    status_t CallKernelWritePort(port_id port, int32_t code, const void* buffer, size_t bufferSize);
    status_t CallKernelReadPort(port_id port, void* buffer, size_t bufferSize, int32_t* code);
    
    // Performance tracking
    void RecordDirectCall(const char* operation, uint64_t time_us);
    void RecordEmulationSaved(const char* operation);
    
    // Cache and optimization
    struct DirectCache {
        char* cached_path;
        int cached_fd;
        uint32_t path_hash;
        uint64_t access_time;
        bool is_valid;
        
        DirectCache() : cached_path(nullptr), cached_fd(-1), path_hash(0), 
                       access_time(0), is_valid(false) {}
    };
    
    static constexpr int CACHE_SIZE = 64;
    DirectCache fFileCache[CACHE_SIZE];
    
    // Cache operations
    int LookupFileCache(const char* path);
    void UpdateFileCache(const char* path, int fd);
    void InvalidateFileCache(const char* path);
    void CleanupFileCache();
};

#endif // HAIKU_KERNEL_DIRECT_H