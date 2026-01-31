/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <SupportDefs.h>

class GuestContext;
class AddressSpace;

// libc function emulation layer
// Provides minimal libc support for guest programs
class InterpreterLibc {
public:
    InterpreterLibc(AddressSpace& addressSpace);
    ~InterpreterLibc();
    
    // Get address of a libc function stub
    // Returns guest virtual address, or 0 if not found
    uint32_t GetFunctionStub(const char* name);
    
    // Initialize the libc stub area in guest memory
    status_t InitializeStubs();
    
    // Supported libc functions
    enum LibcFunction {
        LIBC_PRINTF = 1,
        LIBC_MALLOC = 2,
        LIBC_FREE = 3,
        LIBC_STRLEN = 4,
        LIBC_STRCPY = 5,
        LIBC_MEMCPY = 6,
        LIBC_MEMSET = 7,
        LIBC_EXIT = 8,
        LIBC_PUTS = 9,
        LIBC_GETENV = 10,
    };

private:
    AddressSpace& fAddressSpace;
    uint32_t fStubArea;      // Starting address of stubs in guest memory
    uint32_t fNextStubAddr;  // Next available stub address
};
