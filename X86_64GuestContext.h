/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include "GuestContext.h"
#include "AddressSpace.h"
#include <SupportDefs.h>
#include <stdint.h>
#include <memory>

// Structure for x86-64 CPU registers
struct X86_64Registers {
    uint64_t rip;    // Instruction Pointer
    uint64_t rsp;    // Stack Pointer
    uint64_t rbp;    // Base Pointer
    uint64_t rax;    // Accumulator (also for syscall return)
    uint64_t rbx;    // Base
    uint64_t rcx;    // Counter (4th syscall arg)
    uint64_t rdx;    // Data (3rd syscall arg)
    uint64_t rsi;    // Source Index (2nd syscall arg)
    uint64_t rdi;    // Destination Index (1st syscall arg)
    uint64_t r8;     // 5th syscall arg
    uint64_t r9;     // 6th syscall arg
    uint64_t r10;    // 7th syscall arg
    uint64_t r11;    // 8th syscall arg
    uint64_t r12;    // Callee-saved
    uint64_t r13;    // Callee-saved
    uint64_t r14;    // Callee-saved
    uint64_t r15;    // Callee-saved
    uint64_t rflags; // Flags Register
    uint64_t cs, ss, ds, es, fs, gs; // Segment registers
};

// Guest context for x86-64 architecture
class X86_64GuestContext : public GuestContext {
public:
    X86_64GuestContext(AddressSpace& addressSpace);
    virtual ~X86_64GuestContext();

    // Access to 64-bit registers
    X86_64Registers& Registers() { return fRegisters; }
    const X86_64Registers& Registers() const { return fRegisters; }

    // Memory access methods
    virtual status_t ReadGuestMemory(uint32_t guestAddress, void* buffer, size_t size) override;
    virtual status_t WriteGuestMemory(uint32_t guestAddress, const void* buffer, size_t size) override;

    // Guest exit status
    virtual bool ShouldExit() const override { return fShouldExit; }
    virtual void SetExit(bool exit_flag) override { fShouldExit = exit_flag; }

    // Image base address (for position-dependent binaries)
    uint64_t GetImageBase() const { return fImageBase; }
    void SetImageBase(uint64_t base) { fImageBase = base; }

private:
    X86_64Registers fRegisters;
    AddressSpace& fAddressSpace;
    bool fShouldExit = false;
    uint64_t fImageBase = 0x400000UL; // Default base for ET_DYN binaries
};