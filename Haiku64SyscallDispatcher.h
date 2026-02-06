/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include "SupportDefs.h"
#include "SyscallDispatcher.h"
#include <cstdint>

class GuestContext;

// Haiku 64-bit system call dispatcher
class Haiku64SyscallDispatcher : public SyscallDispatcher {
public:
    Haiku64SyscallDispatcher();
    virtual ~Haiku64SyscallDispatcher() = default;

    // Main syscall dispatch method
    virtual status_t Dispatch(GuestContext& context);

private:
    // Essential system calls
    status_t SyscallExit(uint64_t code);
    status_t SyscallWrite(uint64_t fd, const void* buffer, uint64_t size, uint64_t& result);
    status_t SyscallRead(uint64_t fd, void* buffer, uint64_t size, uint64_t& result);
    status_t SyscallOpen(const char* path, uint64_t flags, uint64_t mode, uint64_t& result);
    status_t SyscallClose(uint64_t fd, uint64_t& result);
    
    // Memory management syscalls
    status_t SyscallBrk(uint64_t addr, uint64_t& result);
    status_t SyscallMmap(uint64_t addr, uint64_t length, uint64_t prot, 
                         uint64_t flags, uint64_t fd, uint64_t offset, uint64_t& result);
    status_t SyscallMunmap(uint64_t addr, uint64_t length, uint64_t& result);
    
    // Directory syscalls
    status_t SyscallGetCwd(char* buffer, uint64_t size, uint64_t& result);
    status_t SyscallChdir(const char* path, uint64_t& result);
    
    // Syscall constants (x86-64 uses different syscall numbers)
    static const uint64_t SYSCALL_EXIT = 60;
    static const uint64_t SYSCALL_WRITE = 1;
    static const uint64_t SYSCALL_READ = 0;
    static const uint64_t SYSCALL_OPEN = 2;
    static const uint64_t SYSCALL_CLOSE = 3;
    static const uint64_t SYSCALL_BRK = 12;
    static const uint64_t SYSCALL_MMAP = 9;
    static const uint64_t SYSCALL_MUNMAP = 11;
    static const uint64_t SYSCALL_GETCWD = 79;
    static const uint64_t SYSCALL_CHDIR = 80;
};