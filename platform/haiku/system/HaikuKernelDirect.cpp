/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * HaikuKernelDirect.cpp - Direct Haiku kernel integration implementation
 */

#include "HaikuKernelDirect.h"
#include <cstdio>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>

HaikuKernelDirect::HaikuKernelDirect() 
    : fMode(DIRECT_MODE_OFF), fIsNativeHaiku(false), fKernelDirectAvailable(false) {
    printf("[KERNEL_DIRECT] Haiku Kernel Direct Interface initialized\n");
}

HaikuKernelDirect::~HaikuKernelDirect() {
    CleanupResources();
    printf("[KERNEL_DIRECT] Haiku Kernel Direct Interface destroyed\n");
}

bool HaikuKernelDirect::Initialize(DirectMode mode) {
    printf("[KERNEL_DIRECT] Initializing direct mode: %d\n", mode);
    
    fMode = mode;
    
    if (!DetectNativeHaiku()) {
        printf("[KERNEL_DIRECT] Not running on native Haiku, direct mode disabled\n");
        fKernelDirectAvailable = false;
        return false;
    }
    
    if (!InitializeKernelInterface()) {
        printf("[KERNEL_DIRECT] Failed to initialize kernel interface\n");
        fKernelDirectAvailable = false;
        return false;
    }
    
    if (!SetupDirectHandles()) {
        printf("[KERNEL_DIRECT] Failed to setup direct handles\n");
        fKernelDirectAvailable = false;
        return false;
    }
    
    fKernelDirectAvailable = true;
    printf("[KERNEL_DIRECT] Direct kernel interface ready\n");
    return true;
}

void HaikuKernelDirect::SetMode(DirectMode mode) {
    fMode = mode;
    printf("[KERNEL_DIRECT] Mode changed to: %d\n", mode);
}

// Direct file operations - bypassing all emulation layers
status_t HaikuKernelDirect::DirectOpenFile(const char* path, int flags, mode_t mode, int* fd) {
    if (!fKernelDirectAvailable) {
        printf("[KERNEL_DIRECT] Direct mode not available\n");
        return B_ERROR;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Check cache first
    int cached_fd = LookupFileCache(path);
    if (cached_fd >= 0) {
        *fd = cached_fd;
        RecordDirectCall("open_cached", 0);
        return B_OK;
    }
    
    // Direct kernel call
    printf("[KERNEL_DIRECT] Direct open: %s\n", path);
    status_t result = CallKernelOpen(path, flags, mode);
    
    if (result == B_OK) {
        int handle = AllocateFileHandle();
        if (handle >= 0) {
            fFileHandles[handle].kernel_fd = *fd; // Kernel will set this
            fFileHandles[handle].open_flags = flags;
            fFileHandles[handle].open_mode = mode;
            fFileHandles[handle].path_buffer = AllocatePath(path);
            fFileHandles[handle].path_hash = HashPath(path);
            fFileHandles[handle].current_pos = 0;
            fFileHandles[handle].is_seekable = true;
            fFileHandles[handle].is_writable = (flags & O_WRONLY) || (flags & O_RDWR);
            
            UpdateFileCache(path, handle);
            *fd = handle;
        } else {
            result = B_NO_MEMORY;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    RecordDirectCall("open", duration.count());
    
    return result;
}

status_t HaikuKernelDirect::DirectReadFile(int fd, off_t pos, void* buffer, size_t* bytesRead) {
    if (!IsValidFileHandle(fd)) {
        printf("[KERNEL_DIRECT] Invalid file handle: %d\n", fd);
        return B_BAD_VALUE;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    DirectFileHandle& handle = fFileHandles[fd];
    if (handle.kernel_fd < 0) {
        return B_FILE_ERROR;
    }
    
    printf("[KERNEL_DIRECT] Direct read: fd=%d, pos=%lld, size=%zu\n", fd, pos, *bytesRead);
    
    // Direct kernel call
    status_t result = CallKernelRead(handle.kernel_fd, buffer, *bytesRead, (ssize_t*)bytesRead);
    
    if (result == B_OK) {
        handle.current_pos = pos + *bytesRead;
        fMetrics.file_operations_direct++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    RecordDirectCall("read", duration.count());
    
    return result;
}

status_t HaikuKernelDirect::DirectWriteFile(int fd, off_t pos, const void* buffer, size_t bytesToWrite, size_t* bytesWritten) {
    if (!IsValidFileHandle(fd)) {
        printf("[KERNEL_DIRECT] Invalid file handle: %d\n", fd);
        return B_BAD_VALUE;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    DirectFileHandle& handle = fFileHandles[fd];
    if (!handle.is_writable) {
        printf("[KERNEL_DIRECT] File not writable: %d\n", fd);
        return B_NOT_ALLOWED;
    }
    
    printf("[KERNEL_DIRECT] Direct write: fd=%d, pos=%lld, size=%zu\n", fd, pos, bytesToWrite);
    
    // Direct kernel call
    status_t result = CallKernelWrite(handle.kernel_fd, buffer, bytesToWrite, (ssize_t*)bytesWritten);
    
    if (result == B_OK) {
        handle.current_pos = pos + *bytesWritten;
        fMetrics.file_operations_direct++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    RecordDirectCall("write", duration.count());
    
    return result;
}

status_t HaikuKernelDirect::DirectCloseFile(int fd) {
    if (!IsValidFileHandle(fd)) {
        return B_BAD_VALUE;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    DirectFileHandle& handle = fFileHandles[fd];
    if (handle.path_buffer) {
        InvalidateFileCache(handle.path_buffer);
    }
    
    printf("[KERNEL_DIRECT] Direct close: fd=%d\n", fd);
    
    status_t result = CallKernelClose(handle.kernel_fd);
    
    if (result == B_OK) {
        FreePath(handle.path_buffer);
        FreeFileHandle(fd);
        fMetrics.file_operations_direct++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    RecordDirectCall("close", duration.count());
    
    return result;
}

status_t HaikuKernelDirect::DirectCreateArea(const char* name, void** address, uint32_t addressSpec,
                                         size_t size, uint32_t lock, uint32_t protection, 
                                         area_id* area) {
    if (!fKernelDirectAvailable) {
        return B_ERROR;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    printf("[KERNEL_DIRECT] Direct create area: %s, size=%zu\n", name, size);
    
    status_t result = CallKernelCreateArea(name, address, addressSpec, size, lock, protection);
    
    if (result == B_OK && *area >= 0) {
        int handle = AllocateMemoryArea();
        if (handle >= 0) {
            fMemoryAreas[handle].area_id = *area;
            fMemoryAreas[handle].base_address = *address;
            fMemoryAreas[handle].size = size;
            fMemoryAreas[handle].protection = protection;
            fMemoryAreas[handle].name = strdup(name);
            fMemoryAreas[handle].is_shared = (protection & B_SHARED_AREA) != 0;
            fMemoryAreas[handle].ref_count = 1;
            fMetrics.memory_operations_direct++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    RecordDirectCall("create_area", duration.count());
    
    return result;
}

status_t HaikuKernelDirect::DirectCreateThread(thread_func function, void* argument, const char* name,
                                           int32_t priority, size_t stackSize, thread_id* threadId) {
    if (!fKernelDirectAvailable) {
        return B_ERROR;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    printf("[KERNEL_DIRECT] Direct create thread: %s\n", name);
    
    status_t result = CallKernelSpawnThread(function, argument, name, priority, stackSize, threadId);
    
    if (result == B_OK && *threadId >= 0) {
        int handle = AllocateThread();
        if (handle >= 0) {
            fThreads[handle].thread_id = *threadId;
            fThreads[handle].entry_point = (void*)function;
            fThreads[handle].argument = argument;
            fThreads[handle].thread_priority = priority;
            fThreads[handle].stack_size = stackSize;
            fThreads[handle].is_running = true;
            fThreads[handle].is_suspended = false;
            fMetrics.thread_operations_direct++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    RecordDirectCall("create_thread", duration.count());
    
    return result;
}

status_t HaikuKernelDirect::DirectCreatePort(int32_t capacity, const char* name, port_id* port) {
    if (!fKernelDirectAvailable) {
        return B_ERROR;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    printf("[KERNEL_DIRECT] Direct create port: %s, capacity=%d\n", name, capacity);
    
    status_t result = CallKernelCreatePort(capacity, name);
    
    if (result == B_OK && *port >= 0) {
        int handle = AllocatePort();
        if (handle >= 0) {
            fPorts[handle].port_id = *port;
            fPorts[handle].capacity = capacity;
            fPorts[handle].name = strdup(name);
            fPorts[handle].is_read_only = false;
            fPorts[handle].total_messages = 0;
            fMetrics.ipc_operations_direct++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    RecordDirectCall("create_port", duration.count());
    
    return result;
}

status_t HaikuKernelDirect::DirectWritePort(port_id port, int32_t code, const void* buffer, size_t bufferSize) {
    if (!fKernelDirectAvailable) {
        return B_ERROR;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    printf("[KERNEL_DIRECT] Direct write port: port=%d, code=%d, size=%zu\n", port, code, bufferSize);
    
    status_t result = CallKernelWritePort(port, code, buffer, bufferSize);
    
    if (result == B_OK) {
        fMetrics.ipc_operations_direct++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    RecordDirectCall("write_port", duration.count());
    
    return result;
}

status_t HaikuKernelDirect::DirectReadPort(port_id port, void* buffer, size_t bufferSize, int32_t* code) {
    if (!fKernelDirectAvailable) {
        return B_ERROR;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    printf("[KERNEL_DIRECT] Direct read port: port=%d, size=%zu\n", port, bufferSize);
    
    status_t result = CallKernelReadPort(port, buffer, bufferSize, code);
    
    if (result == B_OK) {
        fMetrics.ipc_operations_direct++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    RecordDirectCall("read_port", duration.count());
    
    return result;
}

// Private implementation methods
bool HaikuKernelDirect::DetectNativeHaiku() {
    // Check if running on native Haiku
    fIsNativeHaiku = (getenv("BEOS") != nullptr) || (getenv("HAIKU") != nullptr);
    printf("[KERNEL_DIRECT] Native Haiku detected: %s\n", fIsNativeHaiku ? "YES" : "NO");
    return fIsNativeHaiku;
}

bool HaikuKernelDirect::InitializeKernelInterface() {
    printf("[KERNEL_DIRECT] Initializing kernel interface\n");
    return true;
}

bool HaikuKernelDirect::SetupDirectHandles() {
    printf("[KERNEL_DIRECT] Setting up direct handles\n");
    
    // Initialize handle tables
    memset(fFileHandles, 0, sizeof(fFileHandles));
    memset(fMemoryAreas, 0, sizeof(fMemoryAreas));
    memset(fThreads, 0, sizeof(fThreads));
    memset(fPorts, 0, sizeof(fPorts));
    
    // Setup standard file descriptors
    fFileHandles[0].kernel_fd = 0; // stdin
    fFileHandles[0].is_writable = false;
    fFileHandles[1].kernel_fd = 1; // stdout
    fFileHandles[1].is_writable = true;
    fFileHandles[2].kernel_fd = 2; // stderr
    fFileHandles[2].is_writable = true;
    
    return true;
}

int HaikuKernelDirect::AllocateFileHandle() {
    for (int i = 3; i < MAX_DIRECT_FILES; i++) { // Start from 3 (skip stdin/stdout/stderr)
        if (fFileHandles[i].kernel_fd < 0) {
            return i;
        }
    }
    return -1; // No available handles
}

void HaikuKernelDirect::FreeFileHandle(int fd) {
    if (fd >= 0 && fd < MAX_DIRECT_FILES) {
        memset(&fFileHandles[fd], 0, sizeof(DirectFileHandle));
    }
}

int HaikuKernelDirect::AllocateMemoryArea() {
    for (int i = 0; i < MAX_DIRECT_AREAS; i++) {
        if (fMemoryAreas[i].area_id < 0) {
            return i;
        }
    }
    return -1;
}

int HaikuKernelDirect::AllocateThread() {
    for (int i = 0; i < MAX_DIRECT_THREADS; i++) {
        if (fThreads[i].thread_id < 0) {
            return i;
        }
    }
    return -1;
}

int HaikuKernelDirect::AllocatePort() {
    for (int i = 0; i < MAX_DIRECT_PORTS; i++) {
        if (fPorts[i].port_id < 0) {
            return i;
        }
    }
    return -1;
}

// Direct kernel call implementations (simplified)
status_t HaikuKernelDirect::CallKernelOpen(const char* path, int flags, mode_t mode) {
    // On Haiku, this would be a direct kernel syscall
    // For now, use standard POSIX as fallback
    int fd = open(path, flags, mode);
    return (fd >= 0) ? B_OK : B_ERROR;
}

status_t HaikuKernelDirect::CallKernelRead(int fd, void* buffer, size_t count, ssize_t* bytesRead) {
    ssize_t result = read(fd, buffer, count);
    *bytesRead = (result >= 0) ? result : 0;
    return (result >= 0) ? B_OK : B_ERROR;
}

status_t HaikuKernelDirect::CallKernelWrite(int fd, const void* buffer, size_t count, ssize_t* bytesWritten) {
    ssize_t result = write(fd, buffer, count);
    *bytesWritten = (result >= 0) ? result : 0;
    return (result >= 0) ? B_OK : B_ERROR;
}

status_t HaikuKernelDirect::CallKernelClose(int fd) {
    int result = close(fd);
    return (result == 0) ? B_OK : B_ERROR;
}

status_t HaikuKernelDirect::CallKernelCreateArea(const char* name, void** addr, uint32_t addressSpec,
                                               size_t size, uint32_t lock, uint32_t protection) {
    // On Haiku, this would be create_area()
    void* allocated_addr = malloc(size);
    *addr = allocated_addr;
    return (allocated_addr != nullptr) ? B_OK : B_NO_MEMORY;
}

status_t HaikuKernelDirect::CallKernelSpawnThread(thread_func function, void* arg, const char* name,
                                             int32_t priority, size_t stackSize, thread_id* thread) {
    // Simplified thread creation - would be spawn_thread() on Haiku
    return B_ERROR; // Not implemented in this simplified version
}

status_t HaikuKernelDirect::CallKernelCreatePort(int32_t capacity, const char* name) {
    return B_ERROR; // Not implemented in this simplified version
}

status_t HaikuKernelDirect::CallKernelDeletePort(port_id port) {
    return B_ERROR; // Not implemented
}

status_t HaikuKernelDirect::CallKernelWritePort(port_id port, int32_t code, const void* buffer, size_t bufferSize) {
    return B_ERROR; // Not implemented
}

status_t HaikuKernelDirect::CallKernelReadPort(port_id port, void* buffer, size_t bufferSize, int32_t* code) {
    return B_ERROR; // Not implemented
}

// Performance tracking
void HaikuKernelDirect::RecordDirectCall(const char* operation, uint64_t time_us) {
    fMetrics.direct_calls_made++;
    fMetrics.avg_direct_call_time_us = (fMetrics.avg_direct_call_time_us * (fMetrics.direct_calls_made - 1) + time_us) / fMetrics.direct_calls_made;
}

void HaikuKernelDirect::RecordEmulationSaved(const char* operation) {
    fMetrics.emulation_calls_saved++;
}

void HaikuKernelDirect::PrintMetrics() const {
    printf("[KERNEL_DIRECT] Performance Metrics:\n");
    printf("[KERNEL_DIRECT] Direct calls made: %llu\n", (unsigned long long)fMetrics.direct_calls_made);
    printf("[KERNEL_DIRECT] Emulation calls saved: %llu\n", (unsigned long long)fMetrics.emulation_calls_saved);
    printf("[KERNEL_DIRECT] Performance improvement factor: %.2fx\n", fMetrics.performance_improvement_factor);
    printf("[KERNEL_DIRECT] Average direct call time: %.2f us\n", fMetrics.avg_direct_call_time_us);
}

void HaikuKernelDirect::ResetMetrics() {
    fMetrics = DirectMetrics();
}

void HaikuKernelDirect::CleanupResources() {
    printf("[KERNEL_DIRECT] Cleaning up direct resources\n");
    CleanupFileHandles();
    CleanupMemoryAreas();
    CleanupThreads();
    CleanupPorts();
}

void HaikuKernelDirect::CleanupFileHandles() {
    for (int i = 0; i < MAX_DIRECT_FILES; i++) {
        if (fFileHandles[i].kernel_fd >= 0) {
            CallKernelClose(fFileHandles[i].kernel_fd);
        }
        if (fFileHandles[i].path_buffer) {
            FreePath(fFileHandles[i].path_buffer);
        }
    }
    memset(fFileHandles, 0, sizeof(fFileHandles));
}

void HaikuKernelDirect::CleanupMemoryAreas() {
    for (int i = 0; i < MAX_DIRECT_AREAS; i++) {
        if (fMemoryAreas[i].base_address) {
            free(fMemoryAreas[i].base_address);
        }
        if (fMemoryAreas[i].name) {
            free(fMemoryAreas[i].name);
        }
    }
    memset(fMemoryAreas, 0, sizeof(fMemoryAreas));
}

void HaikuKernelDirect::CleanupThreads() {
    memset(fThreads, 0, sizeof(fThreads));
}

void HaikuKernelDirect::CleanupPorts() {
    memset(fPorts, 0, sizeof(fPorts));
}

bool HaikuKernelDirect::IsValidFileHandle(int fd) const {
    return fd >= 0 && fd < MAX_DIRECT_FILES && fFileHandles[fd].kernel_fd >= 0;
}

char* HaikuKernelDirect::AllocatePath(const char* path) {
    return path ? strdup(path) : nullptr;
}

void HaikuKernelDirect::FreePath(char* path) {
    if (path) {
        free(path);
    }
}

uint32_t HaikuKernelDirect::HashPath(const char* path) const {
    if (!path) return 0;
    
    uint32_t hash = 5381;
    unsigned char c;
    while ((c = *path++)) {
        hash = (hash * 33) + c;
    }
    return hash;
}

int HaikuKernelDirect::LookupFileCache(const char* path) {
    uint32_t path_hash = HashPath(path);
    
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (fFileCache[i].is_valid && 
            fFileCache[i].path_hash == path_hash && 
            strcmp(fFileCache[i].cached_path, path) == 0) {
            return fFileCache[i].cached_fd;
        }
    }
    return -1;
}

void HaikuKernelDirect::UpdateFileCache(const char* path, int fd) {
    // Find cache slot
    int oldest_slot = 0;
    uint64_t oldest_time = fFileCache[0].access_time;
    
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (fFileCache[i].access_time < oldest_time) {
            oldest_time = fFileCache[i].access_time;
            oldest_slot = i;
        }
    }
    
    // Update oldest slot
    if (fFileCache[oldest_slot].cached_path) {
        free(fFileCache[oldest_slot].cached_path);
    }
    
    fFileCache[oldest_slot].cached_path = AllocatePath(path);
    fFileCache[oldest_slot].cached_fd = fd;
    fFileCache[oldest_slot].path_hash = HashPath(path);
    fFileCache[oldest_slot].access_time = fMetrics.direct_calls_made;
    fFileCache[oldest_slot].is_valid = true;
}

void HaikuKernelDirect::InvalidateFileCache(const char* path) {
    uint32_t path_hash = HashPath(path);
    
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (fFileCache[i].is_valid && 
            fFileCache[i].path_hash == path_hash && 
            strcmp(fFileCache[i].cached_path, path) == 0) {
            fFileCache[i].is_valid = false;
            if (fFileCache[i].cached_path) {
                free(fFileCache[i].cached_path);
                fFileCache[i].cached_path = nullptr;
            }
        }
    }
}

void HaikuKernelDirect::CleanupFileCache() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (fFileCache[i].cached_path) {
            free(fFileCache[i].cached_path);
        }
    }
    memset(fFileCache, 0, sizeof(fFileCache));
}