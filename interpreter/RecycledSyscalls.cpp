/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * RecycledSyscalls.cpp - Recycled Haiku syscalls implementation
 */

#include "RecycledSyscalls.h"
#include "../SignalHandling.h"
#include "X86_32GuestContext.h"
#include "AddressSpace.h"
#include "../GuestMemoryOperations.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

RecycledSyscalls::RecycledSyscalls() 
    : fProcessId(0), fNextFd(BASE_FD), fInitialized(false) {
    memset(fFdTable, 0, sizeof(fFdTable));
    fWorkingDirectory = "/";
}

RecycledSyscalls::~RecycledSyscalls() {
    // Close all file descriptors
    for (int i = 0; i < MAX_FDS; i++) {
        if (fFdTable[i].is_open && fFdTable[i].host_fd >= 0) {
            close(fFdTable[i].host_fd);
        }
    }
}

bool RecycledSyscalls::Initialize() {
    if (fInitialized) {
        return true;
    }

    printf("[SYSCALL] Initializing recycled syscalls\n");
    
    if (!SetupHandlers()) {
        printf("[SYSCALL] Failed to setup handlers\n");
        return false;
    }
    
    if (!SetupFileDescriptors()) {
        printf("[SYSCALL] Failed to setup file descriptors\n");
        return false;
    }
    
    if (!SetupProcessInfo()) {
        printf("[SYSCALL] Failed to setup process info\n");
        return false;
    }
    
    fInitialized = true;
    printf("[SYSCALL] Recycled syscalls initialized\n");
    return true;
}

int RecycledSyscalls::HandleSyscall(X86_32GuestContext& ctx) {
    uint32_t syscall_num = ctx.eax;
    fMetrics.total_syscalls++;
    
    // Extract arguments from stack
    uint32_t args[6];
    for (int i = 0; i < 6; i++) {
        args[i] = GetStackArg(ctx, i);
    }
    
    auto it = fHandlers.find(syscall_num);
    if (it != fHandlers.end()) {
        int result = it->second(ctx, args);
        fMetrics.successful_syscalls++;
        
        if (syscall_num >= 500) {
            LogSyscall("Haiku syscall", syscall_num, result);
        }
        
        return result;
    }
    
    printf("[SYSCALL] Unknown syscall: %u\n", syscall_num);
    fMetrics.failed_syscalls++;
    return -ENOSYS;
}

int RecycledSyscalls::SysExit(X86_32GuestContext& ctx) {
    uint32_t status = GetStackArg(ctx, 0);
    LogSyscall("exit", SYS_EXIT, status);
    
    printf("[SYSCALL] Process exiting with status %u\n", status);
    
    // Clean up resources
    for (int i = 0; i < MAX_FDS; i++) {
        if (fFdTable[i].is_open && fFdTable[i].host_fd >= 0) {
            close(fFdTable[i].host_fd);
        }
    }
    
    exit(status);
    return 0; // Never reached
}

int RecycledSyscalls::SysWrite(X86_32GuestContext& ctx) {
    uint32_t fd = GetStackArg(ctx, 0);
    uint32_t buffer_addr = GetStackArg(ctx, 1);
    uint32_t count = GetStackArg(ctx, 2);
    
    // Fast path for stdout/stderr
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        fMetrics.fast_path_syscalls++;
        
        // Read guest memory safely
        GuestMemoryOperations guest_mem(ctx->address_space);
        char* buffer = new char[count + 1];
        memset(buffer, 0, count + 1);
        
        // Use SignalHandling for enhanced memory access
        if (!SignalHandling::CheckReadAccess((uintptr_t)buffer_addr, count)) {
            printf("[X86_SYSCALLS] INT 0x80 - read: Memory access denied (PROTECTION FAULT)\n");
            delete[] buffer;
            return -EFAULT; // EACCES - Permission denied
        }
        
        if (!SignalHandling::CheckReadAccess((uintptr_t)buffer_addr, count)) {
            printf("[X86_SYSCALLS] INT 0x80 - read: Memory protection fault (SEGMENTATION)\n");
            delete[] buffer;
            return -EFAULT; // EFAULT - Segmentation fault
        }
        
        if (!guest_mem.ReadStringFromGuest(buffer_addr, buffer, count)) {
            printf("[X86_SYSCALLS] INT 0x80 - read: General I/O error\n");
            delete[] buffer;
            return -EIO; // EIO - I/O error
        }
        
        int result = write(fd, buffer, count);
        
        delete[] buffer;
        LogSyscall("write (fast)", SYS_WRITE, result);
        ctx.eax = result;
        return result;
    }
    
    return WriteFileInternal(fd, (void*)buffer_addr, count);
}

int RecycledSyscalls::SysRead(X86_32GuestContext& ctx) {
    uint32_t fd = GetStackArg(ctx, 0);
    uint32_t buffer_addr = GetStackArg(ctx, 1);
    uint32_t count = GetStackArg(ctx, 2);
    
    return ReadFileInternal(fd, (void*)buffer_addr, count);
}

int RecycledSyscalls::SysOpen(X86_32GuestContext& ctx) {
    const char* path = (const char*)GetStackArg(ctx, 0);
    uint32_t flags = GetStackArg(ctx, 1);
    uint32_t mode = GetStackArg(ctx, 2);
    
    std::string resolved_path = ResolvePath(path);
    int result = OpenFileInternal(resolved_path.c_str(), flags, mode);
    LogSyscall("open", SYS_OPEN, result);
    return result;
}

int RecycledSyscalls::SysClose(X86_32GuestContext& ctx) {
    uint32_t fd = GetStackArg(ctx, 0);
    int result = CloseFileInternal(fd);
    LogSyscall("close", SYS_CLOSE, result);
    return result;
}

int RecycledSyscalls::SysSeek(X86_32GuestContext& ctx) {
    uint32_t fd = GetStackArg(ctx, 0);
    uint32_t offset = GetStackArg(ctx, 1);
    uint32_t whence = GetStackArg(ctx, 2);
    
    return SeekFileInternal(fd, offset, whence);
}

int RecycledSyscalls::SysFork(X86_32GuestContext& ctx) {
    LogSyscall("fork", SYS_FORK, 0);
    return ForkProcessInternal();
}

int RecycledSyscalls::SysExecve(X86_32GuestContext& ctx) {
    const char* path = (const char*)GetStackArg(ctx, 0);
    char* const* argv = (char* const*)GetStackArg(ctx, 1);
    char* const* envp = (char* const*)GetStackArg(ctx, 2);
    
    std::string resolved_path = ResolvePath(path);
    int result = ExecuteProcessInternal(resolved_path.c_str(), argv, envp);
    LogSyscall("execve", SYS_EXECVE, result);
    return result;
}

int RecycledSyscalls::SysWait4(X86_32GuestContext& ctx) {
    uint32_t pid = GetStackArg(ctx, 0);
    uint32_t status_addr = GetStackArg(ctx, 1);
    uint32_t options = GetStackArg(ctx, 2);
    uint32_t rusage_addr = GetStackArg(ctx, 3);
    
    int status = 0;
    int result = waitpid(pid, &status, options);
    
    // TODO: Write status back to guest memory
    if (status_addr != 0) {
        // Write status to guest memory
    }
    
    LogSyscall("wait4", SYS_WAIT4, result);
    return result;
}

int RecycledSyscalls::SysGetpid(X86_32GuestContext& ctx) {
    LogSyscall("getpid", SYS_GETPID, fProcessId);
    return fProcessId;
}

int RecycledSyscalls::SysGetuid(X86_32GuestContext& ctx) {
    int result = getuid();
    LogSyscall("getuid", SYS_GETUID, result);
    return result;
}

int RecycledSyscalls::SysGetgid(X86_32GuestContext& ctx) {
    int result = getgid();
    LogSyscall("getgid", SYS_GETGID, result);
    return result;
}

int RecycledSyscalls::SysKill(X86_32GuestContext& ctx) {
    uint32_t pid = GetStackArg(ctx, 0);
    uint32_t signal = GetStackArg(ctx, 1);
    
    LogSyscall("kill", SYS_KILL, 0);
    return kill(pid, signal);
}

int RecycledSyscalls::SysSigaction(X86_32GuestContext& ctx) {
    uint32_t signum = GetStackArg(ctx, 0);
    uint32_t action_addr = GetStackArg(ctx, 1);
    uint32_t oldaction_addr = GetStackArg(ctx, 2);
    
    // TODO: Read action from guest memory
    return SetSignalAction(signum, (const void*)action_addr, (const void*)oldaction_addr);
}

int RecycledSyscalls::SysSigreturn(X86_32GuestContext& ctx) {
    LogSyscall("sigreturn", SYS_SIGRETURN, 0);
    return SignalReturn();
}

// File descriptor management
int RecycledSyscalls::AllocateFd() {
    for (int i = BASE_FD; i < MAX_FDS; i++) {
        if (!fFdTable[i].is_open) {
            fFdTable[i].is_open = true;
            fFdTable[i].host_fd = -1;
            return i;
        }
    }
    return -EMFILE; // Too many open files
}

void RecycledSyscalls::FreeFd(int fd) {
    if (fd >= 0 && fd < MAX_FDS) {
        if (fFdTable[fd].is_open && fFdTable[fd].host_fd >= 0) {
            close(fFdTable[fd].host_fd);
        }
        fFdTable[fd].is_open = false;
        fFdTable[fd].host_fd = -1;
        fFdTable[fd].path.clear();
    }
}

RecycledSyscalls::FileInfo* RecycledSyscalls::GetFileInfo(int fd) {
    if (fd >= 0 && fd < MAX_FDS && fFdTable[fd].is_open) {
        return &fFdTable[fd];
    }
    return nullptr;
}

bool RecycledSyscalls::IsValidFd(int fd) const {
    return fd >= 0 && fd < MAX_FDS && fFdTable[fd].is_open;
}

// File operation implementations
int RecycledSyscalls::OpenFileInternal(const char* path, int flags, mode_t mode) {
    int host_fd = open(path, flags, mode);
    if (host_fd < 0) {
        return -errno;
    }
    
    int guest_fd = AllocateFd();
    if (guest_fd < 0) {
        close(host_fd);
        return guest_fd;
    }
    
    FileInfo* info = GetFileInfo(guest_fd);
    info->host_fd = host_fd;
    info->path = path;
    info->flags = flags;
    info->mode = mode;
    info->offset = 0;
    
    return guest_fd;
}

int RecycledSyscalls::ReadFileInternal(int fd, void* buffer, size_t count) {
    FileInfo* info = GetFileInfo(fd);
    if (!info) {
        return -EBADF;
    }
    
    ssize_t result = read(info->host_fd, buffer, count);
    if (result < 0) {
        return -errno;
    }
    
    info->offset += result;
    return (int)result;
}

int RecycledSyscalls::WriteFileInternal(int fd, const void* buffer, size_t count) {
    FileInfo* info = GetFileInfo(fd);
    if (!info) {
        return -EBADF;
    }
    
    ssize_t result = write(info->host_fd, buffer, count);
    if (result < 0) {
        return -errno;
    }
    
    info->offset += result;
    return (int)result;
}

int RecycledSyscalls::SeekFileInternal(int fd, off_t offset, int whence) {
    FileInfo* info = GetFileInfo(fd);
    if (!info) {
        return -EBADF;
    }
    
    off_t result = lseek(info->host_fd, offset, whence);
    if (result < 0) {
        return -errno;
    }
    
    info->offset = result;
    return (int)result;
}

int RecycledSyscalls::CloseFileInternal(int fd) {
    FileInfo* info = GetFileInfo(fd);
    if (!info) {
        return -EBADF;
    }
    
    if (info->host_fd >= 0) {
        close(info->host_fd);
    }
    
    FreeFd(fd);
    return 0;
}

int RecycledSyscalls::ForkProcessInternal() {
    pid_t result = fork();
    if (result < 0) {
        return -errno;
    }
    return result;
}

int RecycledSyscalls::ExecuteProcessInternal(const char* path, char* const* argv, char* const* envp) {
    int result = execve(path, argv, envp);
    return result; // execve only returns on error
}

int RecycledSyscalls::WaitProcessInternal(pid_t pid, int* status, int options) {
    int local_status;
    int result = waitpid(pid, &local_status, options);
    
    if (result > 0 && status) {
        // TODO: Write status back to guest memory
    }
    
    return result;
}

// Utility methods
uint32_t RecycledSyscalls::GetStackArg(X86_32GuestContext& ctx, int arg_index) {
    uint32_t esp = ctx.esp;
    uint32_t arg_addr = esp + 4 + (arg_index * 4); // Skip return address
    
    // TODO: Read from guest memory properly
    // For now, assume direct memory access
    return *(uint32_t*)arg_addr;
}

void RecycledSyscalls::InitializeStandardStreams() {
    // Setup stdin, stdout, stderr
    fFdTable[STDIN_FILENO].host_fd = STDIN_FILENO;
    fFdTable[STDIN_FILENO].is_open = true;
    
    fFdTable[STDOUT_FILENO].host_fd = STDOUT_FILENO;
    fFdTable[STDOUT_FILENO].is_open = true;
    
    fFdTable[STDERR_FILENO].host_fd = STDERR_FILENO;
    fFdTable[STDERR_FILENO].is_open = true;
    
    printf("[SYSCALL] Standard streams initialized\n");
}

std::string RecycledSyscalls::ResolvePath(const char* guest_path) const {
    if (!guest_path) {
        return "";
    }
    
    std::string path(guest_path);
    
    // Handle absolute paths
    if (IsPathAbsolute(guest_path)) {
        return SYSROOT_PREFIX + path;
    }
    
    // Handle relative paths
    return fWorkingDirectory + "/" + path;
}

bool RecycledSyscalls::IsPathAbsolute(const char* path) const {
    return path && path[0] == '/';
}

bool RecycledSyscalls::SetupHandlers() {
    printf("[SYSCALL] Setting up syscall handlers\n");
    
    // Setup main syscalls
    fHandlers[SYS_EXIT] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysExit(ctx);
    };
    
    fHandlers[SYS_WRITE] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysWrite(ctx);
    };
    
    fHandlers[SYS_READ] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysRead(ctx);
    };
    
    fHandlers[SYS_OPEN] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysOpen(ctx);
    };
    
    fHandlers[SYS_CLOSE] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysClose(ctx);
    };
    
    fHandlers[SYS_SEEK] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysSeek(ctx);
    };
    
    fHandlers[SYS_FORK] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysFork(ctx);
    };
    
    fHandlers[SYS_EXECVE] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysExecve(ctx);
    };
    
    fHandlers[SYS_WAIT4] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysWait4(ctx);
    };
    
    fHandlers[SYS_GETPID] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysGetpid(ctx);
    };
    
    fHandlers[SYS_GETUID] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysGetuid(ctx);
    };
    
    fHandlers[SYS_GETGID] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysGetgid(ctx);
    };
    
    fHandlers[SYS_KILL] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        printf("[X86_SYSCALLS] INT 0x80 - kill(%d, %d) - REAL HANDLER\n", 
               ctx.eax, ctx.ebx);
        return SignalHandling::HandleTermination();
    };
    
    fHandlers[SYS_SIGACTION] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysSigaction(ctx);
    };
    
    fHandlers[SYS_SIGRETURN] = [this](X86_32GuestContext& ctx, const uint32_t* args) {
        return SysSigreturn();
    };
    
    return true;
}

bool RecycledSyscalls::SetupFileDescriptors() {
    InitializeStandardStreams();
    fProcessId = getpid();
    return true;
}

bool RecycledSyscalls::SetupProcessInfo() {
    fWorkingDirectory = "/";
    return true;
}

void RecycledSyscalls::LogSyscall(const char* name, uint32_t syscall_num, int result) const {
    if (result < 0) {
            printf("[RECYCLED_SYSCALL] %s (%u) failed: %d\n", name, syscall_num, result);
        } else {
            printf("[RECYCLED_SYSCALL] %s (%u) succeeded: %d\n", name, syscall_num, result);
        }
    }
}

int RecycledSyscalls::SetSignalAction(int signum, const void* action, const void* oldaction) {
    // TODO: Implement signal action handling
    return 0;
}

int RecycledSyscalls::SignalReturn() {
    // TODO: Implement signal return
    return 0;
}

void RecycledSyscalls::ResetMetrics() {
    fMetrics = SyscallMetrics();
}

void RecycledSyscalls::PrintMetrics() const {
    printf("[SYSCALL] Performance Metrics:\n");
    printf("[SYSCALL] Total syscalls: %llu\n", (unsigned long long)fMetrics.total_syscalls);
    printf("[SYSCALL] Successful: %llu\n", (unsigned long long)fMetrics.successful_syscalls);
    printf("[SYSCALL] Failed: %llu\n", (unsigned long long)fMetrics.failed_syscalls);
    printf("[SYSCALL] Fast path: %llu\n", (unsigned long long)fMetrics.fast_path_syscalls);
    printf("[SYSCALL] Syscall distribution:\n");
    
    for (const auto& pair : fMetrics.syscall_counts) {
        printf("[SYSCALL]   %u: %llu\n", pair.first, (unsigned long long)pair.second);
    }
}