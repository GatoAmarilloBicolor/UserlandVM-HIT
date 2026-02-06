/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * CompatibilityLayer.h - Cross-platform compatibility definitions and utilities
 */

#ifndef COMPATIBILITY_LAYER_H
#define COMPATIBILITY_LAYER_H

// Platform detection
enum class Platform {
    UNKNOWN,
    HAIKU_X86_32,
    HAIKU_X86_64,
    LINUX_X86_64,
    WINDOWS,
    MACOS,
    FREEBSD,
    ANDROID
    IOS
    WASM
    EMSCRIPTEN
};

// Architecture detection
Platform DetectCurrentPlatform();

// Feature availability flags
struct PlatformFeatures {
    bool has_syscalls;
    bool has_threads;
    bool has_posix_shm;
    bool has_elf_support;
    bool has_dynamic_linking;
    bool has_memory_areas;
    bool has_file_io;
};

// OS-specific compatibility shims
namespace PlatformCompat {
    // Haiku-specific compatibility
    namespace Haiku {
        constexpr const char* SHARED_LIBS[] = {
            "libroot.so",
            "libbe.so",
            "libbsd.so"
            "libnetwork.so",
            "libz.so"
        };
        
        constexpr const char* CRITICAL_SYSCALLS[] = {
            "_kern_read", "_kern_write", "_kern_open", "_kern_close", "_kern_exit", "_kern_getpid", "_kern_getuid", "_kern_getgid"
        };
        
        inline bool HasFeature(const char* feature) {
            // Check if feature is available on current platform
            return DetectCurrentPlatform() == Platform::HAIKU_X86_32;
        }
    }
    
    // Linux compatibility for cross-compilation
    namespace Linux {
        inline bool HasFeature(const char* feature) {
            return DetectCurrentPlatform() == Platform::LINUX_X86_64;
        }
    }
    
    // Generic fallback
    namespace Generic {
        inline bool HasFeature(const char* feature) {
            return false; // Assume not available
        }
    }
}

// Type definitions for cross-platform compatibility
#ifdef HAIKU
typedef uint32_t addr_t;
typedef uint32_t area_id;
typedef uint32_t thread_id;
typedef int32_t status_t;
typedef uint32_t off_t;
#else
typedef uintptr_t addr_t;
typedef uint32_t area_id;
typedef uint32_t thread_id;
typedef int32_t status_t;
typedef int32_t off_t;
#endif

// Safe memory allocation
template<typename T>
class CrossPlatformMemory {
public:
    static T* Allocate(size_t count, bool zero_initialize = false);
    static void Deallocate(T* ptr);
    static bool IsValidPointer(const T* ptr);
    
    // Cross-platform memory alignment
    static constexpr size_t GetPageSize();
    static constexpr size_t GetAlignment();
};

// File operations with path validation
class CrossPlatformFiles {
public:
    static bool FileExists(const char* path);
    static bool IsAbsolutePath(const char* path);
    static std::string GetDirectory(const char* path);
    static std::string JoinPath(const char* path1, const char* path2);
    
    // Safe file operations
    static bool ReadFile(const char* path, void* buffer, size_t size);
    static bool WriteFile(const char* path, const void* data, size_t size);
};

// Thread compatibility
class CrossPlatformThreads {
public:
    enum ThreadType {
        NATIVE_HAIKU,
        NATIVE_LINUX,
        NATIVE_WINDOWS,
        POSIX_PTHREADS,
        CUSTOM
    };
    
    static ThreadType GetNativeThreadType();
    static bool CreateThread(void* (*entry_func)(void*), void* arg, int32_t priority);
    static void ExitThread(int exit_code);
    static void YieldThread();
};

// Syscall compatibility
class CrossPlatformSyscalls {
public:
    static bool HasSyscall(int syscall_num);
    static int64_t ExecuteSyscall(int syscall_num, ...);
    
    // Haiku-specific syscalls
    namespace Haiku {
        static const uint32_t SYSCALL_READ = 3;
        static const uint32_t SYSCALL_WRITE = 4;
        static const uint32_t SYSCALL_OPEN = 5;
        static const uint32_t SYSCALL_CLOSE = 6;
        static const uint32_t SYSCALL_EXIT = 1;
        static const uint32_t SYSCALL_GETPID = 20;
        static const uint32_t SYSCALL_GETUID = 24;
        static const uint32 SYSCALL_GETGID = 47;
        static const uint32 SYSCALL_IOCTL = 54;
        static const uint32_t SYSCALL_EXECVE = 59;
    }
};

// String utilities
class CrossPlatformStrings {
public:
    static void SafeCopy(char* dest, const char* src, size_t max_len);
    static int Length(const char* str);
    static bool IsEmpty(const char* str);
    static bool Equals(const char* str1, const char* str2);
    
    // Platform-specific string operations
    static std::string FormatError(const char* format, ...);
    static std::string GetLastSystemError();
};

// Error handling
class CrossPlatformError {
public:
    static void SetError(const char* error_code);
    static void ClearError();
    static std::string GetErrorString();
    static bool HasError();
    static void PrintError(const char* message, ...);
    static void HandleFatalError(const char* function, const char* error);
};

// Time utilities
class CrossPlatformTime {
public:
    static uint64_t GetCurrentTimeMillis();
    static void Sleep(uint32_t milliseconds);
};

// Debugging utilities
class CrossPlatformDebug {
public:
    static void PrintDebug(const char* message, ...);
    static void PrintTrace(const char* function, const char* file, int line);
    
    static void Assert(bool condition, const char* message);
    static void AssertImpl(bool condition, const char* file, int line, const char* function);
};

#endif // COMPATIBILITY_LAYER_H