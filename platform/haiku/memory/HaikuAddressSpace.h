/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * Haiku OS Native AddressSpace Interface
 * Uses Haiku's native area-based memory management
 */

#ifndef _HAIKU_ADDRESS_SPACE_H
#define _HAIKU_ADDRESS_SPACE_H

// Haiku OS native includes
#include <OS.h>
#include <kernel/OS.h>
#include <SupportDefs.h>

// Haiku-specific memory management
class HaikuAddressSpace {
public:
    virtual ~HaikuAddressSpace() = default;

    // Native Haiku memory operations using areas
    virtual status_t Read(uintptr_t guestAddress, void* buffer, size_t size) = 0;
    virtual status_t ReadString(uintptr_t guestAddress, char* buffer, size_t bufferSize) = 0;
    virtual status_t Write(uintptr_t guestAddress, const void* buffer, size_t size) = 0;

    // Haiku area-based mapping
    virtual status_t RegisterAreaMapping(uintptr_t guest_vaddr, area_id hostArea, size_t size) {
        return B_OK;  // Default: store mapping
    }
    
    virtual uintptr_t TranslateAddress(uintptr_t guest_vaddr) const {
        return guest_vaddr;  // Default: 1:1 mapping
    }
    
    // Haiku TLS area setup
    virtual status_t MapTLSArea(uintptr_t guest_vaddr, size_t size) {
        return B_OK;  // Default: allocate TLS area
    }
    
    // Direct memory operations (optimized for Haiku)
    virtual status_t ReadMemory(uintptr_t guestAddress, void* data, size_t size) {
        return Read(guestAddress, data, size);
    }
    
    virtual status_t WriteMemory(uintptr_t guestAddress, const void* data, size_t size) {
        return Write(guestAddress, data, size);
    }

    // Get the underlying area ID for direct access
    virtual area_id GetAreaID() const = 0;
    virtual void* GetBaseAddress() const = 0;
    virtual size_t GetSize() const = 0;

    // Haiku-native template helpers
    template<typename T>
    status_t Read(uintptr_t guestAddress, T& value) {
        return Read(guestAddress, &value, sizeof(T));
    }

    template<typename T>
    status_t Write(uintptr_t guestAddress, const T& value) {
        return Write(guestAddress, &value, sizeof(T));
    }
};

#endif // _HAIKU_ADDRESS_SPACE_H