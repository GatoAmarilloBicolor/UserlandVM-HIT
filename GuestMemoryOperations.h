// UserlandVM-HIT Guest Memory Operations
// Implements proper guest memory reading/writing for syscall handlers
// Author: Guest Memory Implementation 2026-02-06

#pragma once

#include <cstdint>
#include <cstring>
#include "AddressSpace.h"
#include "X86_32GuestContext.h"

/**
 * GuestMemoryOperations - Safe memory access for guest programs
 * Provides functions to safely read/write between guest and host memory
 */
class GuestMemoryOperations {
private:
    AddressSpace* address_space;
    
public:
    GuestMemoryOperations(AddressSpace* space) : address_space(space) {}
    
    // Read operations
    bool ReadFromGuest(uint32_t guest_addr, void* host_buffer, size_t size) {
        if (!address_space || !address_space->IsAddressValid(guest_addr)) {
            printf("[linux.cosmoe] [GUEST_MEM] Invalid guest address: 0x%x\n", guest_addr);
            return false;
        }
        
        void* guest_ptr = address_space->GetPointer(guest_addr);
        if (!guest_ptr) {
            printf("[linux.cosmoe] [GUEST_MEM] Cannot map guest address: 0x%x\n", guest_addr);
            return false;
        }
        
        memcpy(host_buffer, guest_ptr, size);
        printf("[linux.cosmoe] [GUEST_MEM] Read %zu bytes from 0x%x\n", size, guest_addr);
        return true;
    }
    
    bool ReadStringFromGuest(uint32_t guest_addr, char* host_buffer, size_t max_size) {
        if (!address_space || !address_space->IsAddressValid(guest_addr)) {
            printf("[linux.cosmoe] [GUEST_MEM] Invalid string address: 0x%x\n", guest_addr);
            return false;
        }
        
        void* guest_ptr = address_space->GetPointer(guest_addr);
        if (!guest_ptr) {
            return false;
        }
        
        // Safe string copy with length limit
        strncpy(host_buffer, static_cast<char*>(guest_ptr), max_size - 1);
        host_buffer[max_size - 1] = '\0';
        
        printf("[linux.cosmoe] [GUEST_MEM] Read string: '%s' from 0x%x\n", host_buffer, guest_addr);
        return true;
    }
    
    template<typename T>
    bool ReadValueFromGuest(uint32_t guest_addr, T& value) {
        return ReadFromGuest(guest_addr, &value, sizeof(T));
    }
    
    // Write operations
    bool WriteToGuest(uint32_t guest_addr, const void* host_buffer, size_t size) {
        if (!address_space || !address_space->IsAddressValid(guest_addr)) {
            printf("[linux.cosmoe] [GUEST_MEM] Invalid guest address: 0x%x\n", guest_addr);
            return false;
        }
        
        void* guest_ptr = address_space->GetPointer(guest_addr);
        if (!guest_ptr) {
            printf("[linux.cosmoe] [GUEST_MEM] Cannot map guest address: 0x%x\n", guest_addr);
            return false;
        }
        
        memcpy(guest_ptr, host_buffer, size);
        printf("[linux.cosmoe] [GUEST_MEM] Wrote %zu bytes to 0x%x\n", size, guest_addr);
        return true;
    }
    
    template<typename T>
    bool WriteValueToGuest(uint32_t guest_addr, const T& value) {
        return WriteToGuest(guest_addr, &value, sizeof(T));
    }
    
    // Stack operations
    uint32_t GetStackArgument(X86_32GuestContext* ctx, int arg_index) {
        // Stack layout: [argc][argv0][argv1]...[env0][env1]...
        uint32_t stack_ptr = ctx->GetStackPointer();
        uint32_t argc_addr = stack_ptr;
        
        // Read argc
        uint32_t argc;
        if (!ReadValueFromGuest<uint32_t>(argc_addr, argc)) {
            return 0;
        }
        
        // Calculate argv address
        uint32_t argv_addr = argc_addr + 4;
        
        // Return argv[arg_index]
        if (arg_index >= argc) {
            return 0;
        }
        
        uint32_t arg_ptr_addr = argv_addr + (arg_index * 4);
        uint32_t arg_ptr;
        if (!ReadValueFromGuest<uint32_t>(arg_ptr_addr, arg_ptr)) {
            return 0;
        }
        
        return arg_ptr;
    }
    
    bool WriteStatusToGuest(uint32_t status_addr, int status) {
        return WriteValueToGuest<int>(status_addr, status);
    }
    
    bool WriteResultToGuest(uint32_t result_addr, int result) {
        return WriteValueToGuest<int>(result_addr, result);
    }
    
    // Validation
    bool IsValidGuestAddress(uint32_t addr) const {
        return address_space && address_space->IsAddressValid(addr);
    }
    
    void* GetGuestPointer(uint32_t addr) const {
        if (!address_space || !address_space->IsAddressValid(addr)) {
            return nullptr;
        }
        return address_space->GetPointer(addr);
    }
};

// Convenience macros for guest memory operations
#define READ_GUEST_STRING(guest_addr, buffer, max_size) \
    guest_mem.ReadStringFromGuest(guest_addr, buffer, max_size)

#define READ_GUEST_VALUE(guest_addr, type, value) \
    guest_mem.ReadValueFromGuest<type>(guest_addr, value)

#define WRITE_GUEST_VALUE(guest_addr, type, value) \
    guest_mem.WriteValueToGuest<type>(guest_addr, value)

#define WRITE_GUEST_STATUS(status_addr, status) \
    guest_mem.WriteStatusToGuest(status_addr, status)

#define WRITE_GUEST_RESULT(result_addr, result) \
    guest_mem.WriteResultToGuest(result_addr, result)

#define GET_STACK_ARG(ctx, arg_index) \
    guest_mem.GetStackArgument(ctx, arg_index)