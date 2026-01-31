/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <SupportDefs.h>
#include <stdint.h>

class GuestContextBase;

/**
 * Abstract base class for system call dispatchers
 * Implementations: Haiku32SyscallDispatcher, Linux32SyscallDispatcher, etc.
 */
class SyscallDispatcherBase {
public:
    virtual ~SyscallDispatcherBase() = default;

    /**
     * Dispatch a system call
     * @param syscall_num The syscall number (architecture-specific)
     * @param context Guest execution context (for reading/writing registers, memory)
     * @return B_OK on success, error code on failure
     */
    virtual status_t Dispatch(uint32_t syscall_num, GuestContextBase& context) = 0;

    /**
     * Optional: Get information about the dispatcher
     */
    virtual const char* GetDispatcherName() const {
        return "SyscallDispatcher";
    }

    /**
     * Optional: Get OS/ABI target
     */
    virtual const char* GetTargetOS() const {
        return "Unknown";
    }
};
