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
    // Cast to X86_32GuestContext to access registers
    X86_32GuestContext* x86_context = dynamic_cast<X86_32GuestContext*>(&context);
    if (!x86_context) {
        printf("[SYSCALL] ERROR: Invalid context type for x86-32 dispatcher\n");
        return B_ERROR;
    }
    
    // Get syscall number from EAX
    uint32_t syscall_num = x86_context->Registers().eax;
    
    printf("[SYSCALL] Dispatching syscall %d\n", syscall_num);
    
    switch (syscall_num) {
        case SYSCALL_EXIT:
            return SyscallExit(x86_context->Registers().ebx);
            
        case SYSCALL_WRITE:
            return SyscallWrite(
                x86_context->Registers().ebx,    // fd
                (void*)x86_context->Registers().ecx,  // buffer
                x86_context->Registers().edx,   // count
                x86_context->Registers().eax    // result will be stored here
            );
            
        case SYSCALL_READ:
            return SyscallRead(
                x86_context->Registers().ebx,    // fd
                (void*)x86_context->Registers().ecx,  // buffer
                x86_context->Registers().edx,   // count
                x86_context->Registers().eax    // result will be stored here
            );
            
        case SYSCALL_MMAP2:
            return SyscallMmap2(
                x86_context->Registers().ebx,    // addr
                x86_context->Registers().ecx,    // length
                x86_context->Registers().edx,    // prot
                x86_context->Registers().esi,    // flags
                x86_context->Registers().edi,    // fd
                x86_context->Registers().ebp,    // offset
                x86_context->Registers().eax     // result will be stored here
            );
            
        case SYSCALL_MUNMAP:
            return SyscallMunmap(
                x86_context->Registers().ebx,    // addr
                x86_context->Registers().ecx,    // length
                x86_context->Registers().eax     // result will be stored here
            );
            
        case SYSCALL_OPEN:
            return SyscallOpen(
                (char*)x86_context->Registers().ebx,  // path
                x86_context->Registers().ecx,           // flags
                x86_context->Registers().edx,           // mode
                x86_context->Registers().eax            // result will be stored here
            );
            
        case SYSCALL_CLOSE:
            return SyscallClose(
                x86_context->Registers().ebx,    // fd
                x86_context->Registers().eax     // result will be stored here
            );
            
        case SYSCALL_BRK:
            return SyscallBrk(
                x86_context->Registers().ebx,    // addr
                x86_context->Registers().eax     // result will be stored here
            );
            
        case SYSCALL_GETCWD:
            return SyscallGetCwd(
                (char*)x86_context->Registers().ebx,  // buffer
                x86_context->Registers().ecx,           // size
                x86_context->Registers().eax            // result will be stored here
            );
            
        case SYSCALL_SETCWD:
            return SyscallChdir(
                (char*)x86_context->Registers().ebx,  // path
                x86_context->Registers().eax             // result will be stored here
            );
            
        default:
            printf("[SYSCALL] Unimplemented syscall: %d\n", syscall_num);
            x86_context->Registers().eax = (uint32_t)(-1);  // ENOSYS
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
            // Read from stdin on host
            ssize_t bytes_read = read(fd, buffer, size);
            if (bytes_read < 0) {
                result = (uint32_t)(-1);
                printf("[SYSCALL] Read failed: %s\n", strerror(errno));
            } else {
                result = (uint32_t)bytes_read;
                printf("[SYSCALL] Read successful: %zd bytes\n", bytes_read);
            }
        } else {
            result = 0;
        }
    } else {
        // Read from file descriptor
        ssize_t bytes_read = read(fd, buffer, size);
        if (bytes_read < 0) {
            result = (uint32_t)(-1);
            printf("[SYSCALL] Read failed: %s\n", strerror(errno));
        } else {
            result = (uint32_t)bytes_read;
            printf("[SYSCALL] Read successful: %zd bytes\n", bytes_read);
        }
    }
    
    return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallOpen(const char* path, uint32_t flags, uint32_t mode, uint32_t& result) {
    printf("[SYSCALL] open(\"%s\", 0x%x, 0x%x)\n", path, flags, mode);
    
    if (!path) {
        result = (uint32_t)(-1);
        return B_ERROR;
    }
    
    // Convert Haiku flags to host flags
    int host_flags = 0;
    if (flags & 0x01) host_flags |= O_RDONLY;
    if (flags & 0x02) host_flags |= O_WRONLY;
    if (flags & 0x04) host_flags |= O_RDWR;
    if (flags & 0x08) host_flags |= O_CREAT;
    if (flags & 0x10) host_flags |= O_EXCL;
    if (flags & 0x20) host_flags |= O_TRUNC;
    if (flags & 0x40) host_flags |= O_APPEND;
    
    // Open the file on the host system
    int fd = open(path, host_flags, mode);
    if (fd < 0) {
        result = (uint32_t)(-1);
        printf("[SYSCALL] Open failed: %s\n", strerror(errno));
        return B_ERROR;
    }
    
    result = (uint32_t)fd;
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

status_t Haiku32SyscallDispatcher::SyscallMmap2(uint32_t addr, uint32_t length, uint32_t prot, 
                                                  uint32_t flags, uint32_t fd, uint32_t offset, uint32_t& result) {
    printf("[SYSCALL] mmap2(0x%x, %d, 0x%x, 0x%x, %d, 0x%x)\n", 
           addr, length, prot, flags, fd, offset);
    
    // Real mmap2 implementation for actual guest memory allocation
    // Use the address space to allocate actual memory
    if (length == 0) {
        result = 0;
        printf("[SYSCALL] mmap2: zero length, returning 0\n");
        return B_OK;
    }
    
    // For now, try to allocate from address space
    // In a real implementation, this would:
    // 1. Check if addr is specified and valid for requested flags
    // 2. Allocate actual memory in guest address space
    // 3. Set memory protection bits
    // 4. Return mapped address
    
    // Simplified implementation: allocate from address space
    void* allocated_addr = fAddressSpace->Allocate(length, 4096); // Page-aligned
    if (!allocated_addr) {
        printf("[SYSCALL] mmap2: allocation failed\n");
        result = 0xFFFFFFFF; // MAP_FAILED
        return B_ERROR;
    }
    
    result = (uint32_t)allocated_addr;
    printf("[SYSCALL] mmap2: Successfully allocated %d bytes at 0x%x\n", length, result);
    return B_OK;
}

status_t Haiku32SyscallDispatcher::SyscallMunmap(uint32_t addr, uint32_t length, uint32_t& result) {
    printf("[SYSCALL] munmap(0x%x, %d)\n", addr, length);
    
    // Always succeed
    result = 0;
    return B_OK;
}