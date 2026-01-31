/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include "core/GuestContextBase.h"
#include "core/ExecutionEngineBase.h"
#include "core/SyscallDispatcherBase.h"
#include "core/AddressSpace.h"

#include <SupportDefs.h>
#include <elf.h>

/**
 * Factory class for creating architecture-specific components
 * Handles instantiation of ExecutionEngine, SyscallDispatcher, and GuestContext
 * based on detected or specified architecture
 */
class ArchitectureFactory {
public:
    /**
     * Detect architecture from ELF header
     * @param e_machine ELF e_machine field
     * @return Detected architecture
     */
    static Architecture DetectFromELFMachine(uint16_t e_machine);

    /**
     * Create a guest execution context for the specified architecture
     * @param arch Target architecture
     * @return Newly created context (caller owns)
     */
    static GuestContextBase* CreateGuestContext(Architecture arch);

    /**
     * Create an execution engine for the specified architecture
     * @param arch Target architecture
     * @param addressSpace Address space for memory access
     * @param dispatcher Syscall dispatcher for syscall handling
     * @return Newly created execution engine (caller owns)
     */
    static ExecutionEngineBase* CreateExecutionEngine(
        Architecture arch,
        AddressSpace& addressSpace,
        SyscallDispatcherBase& dispatcher);

    /**
     * Create a syscall dispatcher for the specified architecture and target OS
     * @param arch Target architecture
     * @param target_os Target operating system (e.g., "haiku32")
     * @param sysroot_path Path to sysroot directory
     * @return Newly created dispatcher (caller owns)
     */
    static SyscallDispatcherBase* CreateSyscallDispatcher(
        Architecture arch,
        const char* target_os,
        const char* sysroot_path);

    /**
     * Get human-readable name for architecture
     */
    static const char* GetArchitectureName(Architecture arch);

    /**
     * Get ELF e_machine constant for architecture
     */
    static uint16_t GetELFMachine(Architecture arch);
};
