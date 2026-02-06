/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Haiku64SyscallDispatcher.h"
#include "GuestContext.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>

Haiku64SyscallDispatcher::Haiku64SyscallDispatcher() {
    printf("[SYSCALL64] Haiku64 syscall dispatcher initialized\n");
}

status_t Haiku64SyscallDispatcher::Dispatch(GuestContext& context) {
    // For now, we need to create an X86_64GuestContext but it doesn't exist yet
    // We'll use a simple approach with basic register access
    printf("[SYSCALL64] Dispatching 64-bit syscall\n");
    
    // TODO: Implement proper x86-64 register access
    // For now, we'll just return success for any syscall
    printf("[SYSCALL64] x86-64 support not fully implemented yet\n");
    return B_OK;
}

status_t Haiku64SyscallDispatcher::SyscallExit(uint64_t code) {
    printf("[SYSCALL64] exit(%lu)\n", code);
    return code;
}

status_t Haiku64SyscallDispatcher::SyscallWrite(uint64_t fd, const void* buffer, uint64_t size, uint64_t& result) {
    printf("[SYSCALL64] write(%lu, %p, %lu)\n", fd, buffer, size);
    
    if (fd == 1 || fd == 2) {  // stdout or stderr
        FILE* stream = (fd == 1) ? stdout : stderr;
        
        if (buffer && size > 0) {
            size_t written = fwrite(buffer, 1, size, stream);
            result = (uint64_t)written;
            printf("[SYSCALL64] Write successful: wrote %zu bytes\n", written);
        } else {
            result = 0;
        }
    } else {
        result = size;
    }
    
    return B_OK;
}

status_t Haiku64SyscallDispatcher::SyscallRead(uint64_t fd, void* buffer, uint64_t size, uint64_t& result) {
    printf("[SYSCALL64] read(%lu, %p, %lu)\n", fd, buffer, size);
    
    if (fd == 0) {  // stdin
        if (buffer && size > 0) {
            ssize_t bytes_read = read(fd, buffer, size);
            if (bytes_read < 0) {
                result = (uint64_t)(-1);
                printf("[SYSCALL64] Read failed: %s\n", strerror(errno));
            } else {
                result = (uint64_t)bytes_read;
                printf("[SYSCALL64] Read successful: %zd bytes\n", bytes_read);
            }
        } else {
            result = 0;
        }
    } else {
        ssize_t bytes_read = read(fd, buffer, size);
        if (bytes_read < 0) {
            result = (uint64_t)(-1);
            printf("[SYSCALL64] Read failed: %s\n", strerror(errno));
        } else {
            result = (uint64_t)bytes_read;
            printf("[SYSCALL64] Read successful: %zd bytes\n", bytes_read);
        }
    }
    
    return B_OK;
}

status_t Haiku64SyscallDispatcher::SyscallOpen(const char* path, uint64_t flags, uint64_t mode, uint64_t& result) {
    printf("[SYSCALL64] open(\"%s\", 0x%lx, 0x%lx)\n", path, flags, mode);
    
    if (!path) {
        result = (uint64_t)(-1);
        return B_ERROR;
    }
    
    // Convert flags to host flags (x86-64 uses Linux convention)
    int host_flags = 0;
    if (flags & 0x01) host_flags |= O_RDONLY;
    if (flags & 0x02) host_flags |= O_WRONLY;
    if (flags & 0x04) host_flags |= O_RDWR;
    if (flags & 0x08) host_flags |= O_CREAT;
    if (flags & 0x10) host_flags |= O_EXCL;
    if (flags & 0x20) host_flags |= O_TRUNC;
    if (flags & 0x40) host_flags |= O_APPEND;
    
    int fd = open(path, host_flags, mode);
    if (fd < 0) {
        result = (uint64_t)(-1);
        printf("[SYSCALL64] Open failed: %s\n", strerror(errno));
        return B_ERROR;
    }
    
    result = (uint64_t)fd;
    printf("[SYSCALL64] Opened file descriptor: %d\n", result);
    return B_OK;
}

status_t Haiku64SyscallDispatcher::SyscallClose(uint64_t fd, uint64_t& result) {
    printf("[SYSCALL64] close(%lu)\n", fd);
    
    result = 0;
    return B_OK;
}

status_t Haiku64SyscallDispatcher::SyscallBrk(uint64_t addr, uint64_t& result) {
    printf("[SYSCALL64] brk(0x%lx)\n", addr);
    
    result = addr;
    return B_OK;
}

status_t Haiku64SyscallDispatcher::SyscallMmap(uint64_t addr, uint64_t length, uint64_t prot, 
                                               uint64_t flags, uint64_t fd, uint64_t offset, uint64_t& result) {
    printf("[SYSCALL64] mmap(0x%lx, %lu, 0x%lx, 0x%lx, %lu, %lu)\n", 
           addr, length, prot, flags, fd, offset);
    
    static uint64_t next_mmap_addr = 0x50000000UL;
    result = next_mmap_addr;
    next_mmap_addr += length;
    
    printf("[SYSCALL64] mmap returned: 0x%lx\n", result);
    return B_OK;
}

status_t Haiku64SyscallDispatcher::SyscallMunmap(uint64_t addr, uint64_t length, uint64_t& result) {
    printf("[SYSCALL64] munmap(0x%lx, %lu)\n", addr, length);
    
    result = 0;
    return B_OK;
}

status_t Haiku64SyscallDispatcher::SyscallGetCwd(char* buffer, uint64_t size, uint64_t& result) {
    printf("[SYSCALL64] getcwd(%p, %lu)\n", buffer, size);
    
    if (!buffer || size == 0) {
        result = (uint64_t)(-1);
        return B_ERROR;
    }
    
    const char* cwd = "/boot/home";
    size_t cwd_len = strlen(cwd);
    
    if (cwd_len < size) {
        strcpy(buffer, cwd);
        result = (uint64_t)cwd_len;
    } else {
        result = (uint64_t)(-1);
    }
    
    return B_OK;
}

status_t Haiku64SyscallDispatcher::SyscallChdir(const char* path, uint64_t& result) {
    printf("[SYSCALL64] chdir(\"%s\")\n", path);
    
    result = 0;
    return B_OK;
}