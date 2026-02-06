/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under terms of MIT License.
 */

#include "Haiku32SyscallDispatcher.h"
#include "X86_32GuestContext.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>

Haiku32SyscallDispatcher::Haiku32SyscallDispatcher() {
    printf("[SYSCALL] Haiku32 syscall dispatcher initialized\n");
}

status_t Haiku32SyscallDispatcher::Dispatch(GuestContext& context) {
    // Get syscall number from EAX
    uint32_t syscall_num = context.Registers().eax;
    
    printf("[SYSCALL] Dispatching syscall %d\n", syscall_num);
    
    switch (syscall_num) {
        case SYSCALL_EXIT:
            return SyscallExit(context.Registers().ebx);
            
        case SYSCALL_WRITE:
            return SyscallWrite(
                context.Registers().ebx,    // fd
                (void*)context.Registers().ecx,  // buffer
                context.Registers().edx,   // count
                context.Registers().eax    // result will be stored here
            );
            
        case SYSCALL_READ:
            return SyscallRead(
                context.Registers().ebx,    // fd
                (void*)context.Registers().ecx,  // buffer
                context.Registers().edx,   // count
                context.Registers().eax    // result will be stored here
            );
            
        case SYSCALL_OPEN:
            return SyscallOpen(
                (char*)context.Registers().ebx,  // path
                context.Registers().ecx,           // flags
                context.Registers().edx,           // mode
                context.Registers().eax            // result will be stored here
            );
            
        case SYSCALL_CLOSE:
            return SyscallClose(
                context.Registers().ebx,    // fd
                context.Registers().eax     // result will be stored here
            );
            
        case SYSCALL_BRK:
            return SyscallBrk(
                context.Registers().ebx,    // addr
                context.Registers().eax     // result will be stored here
            );
            
        case SYSCALL_GETCWD:
            return SyscallGetCwd(
                (char*)context.Registers().ebx,  // buffer
                context.Registers().ecx,           // size
                context.Registers().eax            // result will be stored here
            );
            
        case SYSCALL_SETCWD:
            return SyscallChdir(
                (char*)context.Registers().ebx,  // path
                context.Registers().eax             // result will be stored here
            );
            
        default:
            printf("[SYSCALL] Unimplemented syscall: %d\n", syscall_num);
            context.Registers().eax = (uint32_t)(-1);  // ENOSYS
            return B_ERROR;
    }
}

status_t Haiku32SyscallDispatcher::SyscallExit(uint32_t code) {
    printf("[SYSCALL] exit(%d)\n", code);
    
    // Exit the process - for now just return the code
    // In a real implementation, this would terminate the guest process
    return code;
}

status_t Haiku32SyscallDispatcher::SyscallWrite(uint32_t fd, const void* buffer, uint32_t size, uint32_t& result) {
    printf("[SYSCALL] write(%d, %p, %d)\n", fd, buffer, size);
    
    if (fd == 1 || fd == 2) {  // stdout or stderr
        FILE* stream = (fd == 1) ? stdout : stderr;
        
        if (buffer && size > 0) {
            size_t written = fwrite(buffer, 1, size, stream);
            result = (uint32_t)written;
            printf("[SYSCALL] Write successful: wrote %zu bytes\n", written);
        } else {
            result = 0;
        }
    } else {
        // For other file descriptors, just pretend success
        result = size;
    }
    
    return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallRead(uint32_t fd, void* buffer, uint32_t size, uint32_t& result) {
    printf("[SYSCALL] read(%d, %p, %d)\n", fd, buffer, size);
    
    if (fd == 0) {  // stdin
        if (buffer && size > 0) {
            // For simplicity, just return EOF (0 bytes read)
            result = 0;
        } else {
            result = (uint32_t)(-1);
        }
    } else {
        // For other file descriptors, return error
        result = (uint32_t)(-1);
    }
    
    return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallOpen(const char* path, uint32_t flags, uint32_t mode, uint32_t& result) {
    printf("[SYSCALL] open(\"%s\", 0x%x, 0x%x)\n", path, flags, mode);
    
    if (!path) {
        result = (uint32_t)(-1);
        return B_ERROR;
    }
    
    // For simplicity, always return a valid file descriptor (3, 4, 5, etc.)
    // This allows programs to continue without failing
    static uint32_t next_fd = 3;
    result = next_fd++;
    
    printf("[SYSCALL] Opened file descriptor: %d\n", result);
    return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallClose(uint32_t fd, uint32_t& result) {
    printf("[SYSCALL] close(%d)\n", fd);
    
    // Always succeed
    result = 0;
    return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallBrk(uint32_t addr, uint32_t& result) {
    printf("[SYSCALL] brk(0x%x)\n", addr);
    
    // For simplicity, return the requested address
    // In a real implementation, this would manage the heap
    result = addr;
    return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallGetCwd(char* buffer, uint32_t size, uint32_t& result) {
    printf("[SYSCALL] getcwd(%p, %d)\n", buffer, size);
    
    if (!buffer || size == 0) {
        result = (uint32_t)(-1);
        return B_ERROR;
    }
    
    // Return a simple current directory
    const char* cwd = "/boot/home";
    size_t cwd_len = strlen(cwd);
    
    if (cwd_len < size) {
        strcpy(buffer, cwd);
        result = (uint32_t)cwd_len;
    } else {
        result = (uint32_t)(-1);
    }
    
    return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallChdir(const char* path, uint32_t& result) {
    printf("[SYSCALL] chdir(\"%s\")\n", path);
    
    // Always succeed
    result = 0;
    return B_OK;
}