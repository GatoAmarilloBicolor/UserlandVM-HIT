/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * RecycledSyscalls.h - Recycled Haiku syscalls for interpreter integration
 */

#ifndef RECYCLED_SYSCALLS_H
#define RECYCLED_SYSCALLS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>

// Forward declarations
class X86_32GuestContext;
class AddressSpace;

class RecycledSyscalls {
public:
    // Haiku syscall numbers (compatibility)
    enum HaikuSyscallNumber {
        SYS_EXIT = 1,
        SYS_FORK = 2,
        SYS_READ = 3,
        SYS_WRITE = 4,
        SYS_OPEN = 5,
        SYS_CLOSE = 6,
        SYS_WAIT4 = 7,
        SYS_IOCTL = 54,
        SYS_EXECVE = 59,
        SYS_MUNMAP = 91,
        SYS_MMAP = 90,
        SYS_MPROTECT = 125,
        SYS_SIGACTION = 67,
        SYS_SIGPROCMASK = 126,
        SYS_GETUID = 24,
        SYS_GETGID = 47,
        SYS_GETPID = 20,
        SYS_SEEK = 8,
        SYS_UNLINK = 10,
        SYS_MKDIR = 39,
        SYS_RMDIR = 40,
        SYS_PIPE = 42,
        SYS_DUP2 = 63,
        SYS_GETCWD = 183,
        SYS_CHDIR = 12,
        SYS_RENAME = 38,
        SYS_FSTAT = 28,
        SYS_STAT = 18,
        
        // Haiku-specific syscalls (high numbers)
        SYS_CREATE_AREA = 100,
        SYS_DELETE_AREA = 101,
        SYS_FIND_AREA = 102,
        SYS_SET_AREA_PROTECTION = 103,
        SYS_RESIZE_AREA = 104,
        SYS_CLONE_AREA = 105,
        SYS_GET_NEXT_AREA_INFO = 106,
        SYS_KILL = 37,
        SYS_SIGRETURN = 416,
        
        // File descriptor management
        SYS_DUP = 41,
        SYS_FCNTL = 55,
        SYS_FSYNC = 95,
        SYS_FDATASYNC = 148,
        
        // Thread management
        SYS_THREAD_CREATE = 118,
        SYS_THREAD_EXIT = 119,
        SYS_THREAD_KILL = 120,
        SYS_SEM_CREATE = 129,
        SYS_SEM_DELETE = 130,
        SYS_SEM_ACQUIRE = 131,
        SYS_SEM_RELEASE = 132,
        
        // Port and message passing
        SYS_CREATE_PORT = 122,
        SYS_WRITE_PORT = 123,
        SYS_READ_PORT = 124,
        SYS_DELETE_PORT = 125,
        
        // Time functions
        SYS_TIME = 201,
        SYS_SETTIMEOFDAY = 79,
        SYS_GETTIMEOFDAY = 78,
        
        // Network
        SYS_SOCKET = 141,
        SYS_BIND = 149,
        SYS_LISTEN = 150,
        SYS_ACCEPT = 142,
        SYS_CONNECT = 147,
        SYS_SENDTO = 146,
        SYS_RECVFROM = 145,
        
        // Standard streams
        STDOUT_FILENO = 1,
        STDERR_FILENO = 2,
        STDIN_FILENO = 0
    };

    // File descriptor tracking
    struct FileInfo {
        int host_fd;           // Host file descriptor
        std::string path;     // Original path
        int flags;            // Open flags
        mode_t mode;          // File mode
        off_t offset;         // Current offset
        bool is_open;          // Open status
        
        FileInfo() : host_fd(-1), flags(0), mode(0), offset(0), is_open(false) {}
    };

private:
    // File descriptor table (host -> guest mapping)
    static constexpr int MAX_FDS = 1024;
    FileInfo fFdTable[MAX_FDS];
    
    // Syscall handler map for fast dispatch
    std::unordered_map<uint32_t, std::function<int(X86_32GuestContext&, const uint32_t*)>> fHandlers;
    
    // Process information
    pid_t fProcessId;
    uint32_t fNextFd;
    bool fInitialized;

public:
    RecycledSyscalls();
    ~RecycledSyscalls();
    
    // Initialization
    bool Initialize();
    void Reset();
    
    // Main syscall dispatch
    int HandleSyscall(X86_32GuestContext& ctx);
    int HandleSyscallWithArgs(uint32_t syscall_num, const uint32_t* args);
    
    // Individual syscall implementations (recycled from Haiku32SyscallDispatcher)
    int SysExit(X86_32GuestContext& ctx);
    int SysWrite(X86_32GuestContext& ctx);
    int SysRead(X86_32GuestContext& ctx);
    int SysOpen(X86_32GuestContext& ctx);
    int SysClose(X86_32GuestContext& ctx);
    int SysSeek(X86_32GuestContext& ctx);
    int SysFork(X86_32GuestContext& ctx);
    int SysExecve(X86_32GuestContext& ctx);
    int SysWait4(X86_32GuestContext& ctx);
    int SysGetpid(X86_32GuestContext& ctx);
    int SysGetuid(X86_32GuestContext& ctx);
    int SysGetgid(X86_32GuestContext& ctx);
    int SysKill(X86_32GuestContext& ctx);
    int SysSigaction(X86_32GuestContext& ctx);
    int SysSigreturn(X86_32GuestContext& ctx);
    
    // File descriptor management
    int AllocateFd();
    void FreeFd(int fd);
    FileInfo* GetFileInfo(int fd);
    bool IsValidFd(int fd) const;
    
    // Standard stream management
    void InitializeStandardStreams();
    int GetHostFd(int guest_fd) const;
    int GetGuestFd(int host_fd) const;
    
    // Path resolution
    std::string ResolvePath(const char* guest_path) const;
    bool IsPathAbsolute(const char* path) const;
    std::string GetWorkingDirectory() const;
    void SetWorkingDirectory(const std::string& path);
    
    // Utility methods for argument extraction
    static uint32_t GetStackArg(X86_32GuestContext& ctx, int arg_index);
    static const char* GetStackString(X86_32GuestContext& ctx, int arg_index, AddressSpace* space);
    static void* GetStackPointer(X86_32GuestContext& ctx, int arg_index);
    
    // Error handling
    static int HaikuErrorToLinux(int haiku_error);
    static int LinuxErrorToHaiku(int linux_error);
    
    // Performance tracking
    struct SyscallMetrics {
        uint64_t total_syscalls;
        uint64_t successful_syscalls;
        uint64_t failed_syscalls;
        uint64_t fast_path_syscalls;
        std::unordered_map<uint32_t, uint64_t> syscall_counts;
        
        SyscallMetrics() : total_syscalls(0), successful_syscalls(0),
                         failed_syscalls(0), fast_path_syscalls(0) {}
    };
    
    SyscallMetrics GetMetrics() const { return fMetrics; }
    void ResetMetrics();
    void PrintMetrics() const;

private:
    // Internal helper methods
    bool SetupHandlers();
    bool SetupFileDescriptors();
    bool SetupProcessInfo();
    
    // File operation helpers
    int OpenFileInternal(const char* path, int flags, mode_t mode);
    int ReadFileInternal(int fd, void* buffer, size_t count);
    int WriteFileInternal(int fd, const void* buffer, size_t count);
    int SeekFileInternal(int fd, off_t offset, int whence);
    int CloseFileInternal(int fd);
    
    // Process management helpers
    int ForkProcessInternal();
    int ExecuteProcessInternal(const char* path, char* const* argv, char* const* envp);
    int WaitProcessInternal(pid_t pid, int* status, int options);
    
    // Signal handling helpers
    int SetSignalAction(int signum, const void* action, const void* oldaction);
    int SignalReturn();
    
    // Memory management helpers
    int CreateAreaInternal(const char* name, void** addr, uint32_t address_spec,
                         size_t size, uint32_t lock, uint32_t protection);
    int DeleteAreaInternal(int area_id);
    int SetAreaProtectionInternal(int area_id, uint32_t new_protection);
    
    // Thread management helpers
    int CreateThreadInternal(void* (*entry_func)(void*), void* arg, uint32_t* thread_id);
    int ExitThreadInternal(int status);
    int KillThreadInternal(uint32_t thread_id, int signal);
    
    // Port management helpers
    int CreatePortInternal(int32_t capacity, const char* name);
    int WritePortInternal(int32_t port, int32_t code, void* buffer, size_t buffer_size);
    int ReadPortInternal(int32_t port, void* buffer, size_t buffer_size, int32_t* code);
    
    // Debug and logging
    void LogSyscall(const char* name, uint32_t syscall_num, int result) const;
    void LogSyscallArgs(uint32_t syscall_num, const uint32_t* args) const;
    
    // Working directory management
    std::string fWorkingDirectory;
    
    // Performance metrics
    SyscallMetrics fMetrics;
    
    // Constants
    static constexpr int BASE_FD = 3; // Start from 3 (0,1,2 are stdin/out/err)
    static constexpr uint32_t SYSCALL_TABLE_SIZE = 500;
    
    // Path resolution constants
    static constexpr const char* SYSROOT_PREFIX = "sysroot/haiku32";
    static constexpr const char* LIB_PREFIX = "lib";
    static constexpr const char* LIB_SUFFIX = ".so";
};

#endif // RECYCLED_SYSCALLS_H