#pragma once

#include "AddressSpace.h"
#include "PlatformTypes.h"
#include <cstring>
#include <cstdio>

// Real implementation of AddressSpace for guest memory
class RealAddressSpace : public AddressSpace {
private:
    uint8_t *memory;
    size_t size;
    
public:
    RealAddressSpace(void *base, size_t sz) 
        : memory((uint8_t *)base), size(sz) {
        printf("[RealAddressSpace] Created with base=%p, size=%zu\n", base, sz);
    }
    
    virtual ~RealAddressSpace() {}
    
    virtual status_t Read(uintptr_t guestAddress, void* buffer, size_t sz) override {
        // guestAddress is an offset from base 0 in guest memory
        // memory points to the host memory where guest memory is mapped
        if (guestAddress + sz > size) {
            printf("[RealAddressSpace] Read out of bounds: addr=0x%lx, size=%zu, limit=%zu\n", 
                   guestAddress, sz, size);
            return B_BAD_VALUE;
        }
        memcpy(buffer, memory + guestAddress, sz);
        //printf("[RealAddressSpace] Read 0x%lx: %zu bytes\n", guestAddress, sz);
        return B_OK;
    }
    
    virtual status_t ReadString(uintptr_t guestAddress, char* buffer, size_t bufferSize) override {
        if (guestAddress >= size) return B_BAD_VALUE;
        
        size_t available = size - guestAddress;
        size_t to_read = (bufferSize - 1 < available) ? (bufferSize - 1) : available;
        
        memcpy(buffer, memory + guestAddress, to_read);
        buffer[to_read] = '\0';
        return B_OK;
    }
    
    virtual status_t Write(uintptr_t guestAddress, const void* buffer, size_t sz) override {
        if (guestAddress + sz > size) {
            printf("[RealAddressSpace] Write out of bounds: addr=0x%lx, size=%zu, limit=%zu\n",
                   guestAddress, sz, size);
            return B_BAD_VALUE;
        }
        memcpy(memory + guestAddress, buffer, sz);
        return B_OK;
    }
    
    void *GetPointer(uintptr_t addr) {
        if (addr >= size) return nullptr;
        return memory + addr;
    }
};
