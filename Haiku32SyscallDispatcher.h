/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under terms of MIT License.
 */
#pragma once

#include "SupportDefs.h"
#include <cstdint>

class GuestContext;

// Haiku 32-bit system call dispatcher
class Haiku32SyscallDispatcher {
public:
    Haiku32SyscallDispatcher();
    virtual ~Haiku32SyscallDispatcher() = default;

    // Main syscall dispatch method
    virtual status_t Dispatch(GuestContext& context);

private:
    // Essential system calls
    status_t SyscallExit(uint32_t code);
    status_t SyscallWrite(uint32_t fd, const void* buffer, uint32_t size, uint32_t& result);
    status_t SyscallRead(uint32_t fd, void* buffer, uint32_t size, uint32_t& result);
    status_t SyscallOpen(const char* path, uint32_t flags, uint32_t mode, uint32_t& result);
    status_t SyscallClose(uint32_t fd, uint32_t& result);
    
    // Memory management syscalls
    status_t SyscallBrk(uint32_t addr, uint32_t& result);
    status_t SyscallMmap2(uint32_t addr, uint32_t length, uint32_t prot, 
                         uint32_t flags, uint32_t fd, uint32_t offset, uint32_t& result);
    status_t SyscallMunmap(uint32_t addr, uint32_t length, uint32_t& result);
    
    // Directory syscalls
    status_t SyscallGetCwd(char* buffer, uint32_t size, uint32_t& result);
    status_t SyscallChdir(const char* path, uint32_t& result);
    
    // Syscall constants
    static const uint32_t SYSCALL_EXIT = 41;
    static const uint32_t SYSCALL_WRITE = 151;
    static const uint32_t SYSCALL_READ = 149;
    static const uint32_t SYSCALL_OPEN = 114;
    static const uint32_t SYSCALL_CLOSE = 158;
    static const uint32_t SYSCALL_BRK = 119;
    static const uint32_t SYSCALL_MMAP2 = 166;
    static const uint32_t SYSCALL_MUNMAP = 148;
    static const uint32_t SYSCALL_GETCWD = 146;
    static const uint32_t SYSCALL_SETCWD = 147;
};