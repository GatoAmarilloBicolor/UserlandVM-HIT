/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <SupportDefs.h>
#include <stdint.h>

/**
 * Architecture enumeration - used for multi-arch support
 */
enum class Architecture {
    X86_32,
    ARM32,
    RISCV32,
    UNKNOWN
};

/**
 * Abstract base class for guest execution context
 * All architecture-specific contexts must inherit from this
 */
class GuestContextBase {
public:
    virtual ~GuestContextBase() = default;

    // Architecture information
    virtual Architecture GetArchitecture() const = 0;
    virtual const char* GetArchitectureName() const = 0;

    // Program counter / Instruction pointer
    virtual uint32_t GetPC() const = 0;
    virtual void SetPC(uint32_t pc) = 0;

    // Stack pointer
    virtual uint32_t GetSP() const = 0;
    virtual void SetSP(uint32_t sp) = 0;

    // General purpose register access (architecture-agnostic)
    // reg_id: 0-7 for common registers (eax, ecx, edx, etc.)
    virtual uint32_t GetRegister(uint32_t reg_id) const = 0;
    virtual void SetRegister(uint32_t reg_id, uint32_t value) = 0;

    // Flags / Status register
    virtual uint32_t GetFlags() const = 0;
    virtual void SetFlags(uint32_t flags) = 0;

    // Individual flag access
    virtual bool GetFlag(uint32_t flag_bit) const = 0;
    virtual void SetFlag(uint32_t flag_bit, bool value) = 0;

    // Return registers (for system calls and function returns)
    virtual uint32_t GetReturnValue() const = 0;
    virtual void SetReturnValue(uint32_t value) = 0;
};
