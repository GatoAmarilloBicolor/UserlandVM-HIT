/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef GUEST_MEMORY_ALLOCATOR_H
#define GUEST_MEMORY_ALLOCATOR_H

#include <stdint.h>
#include <cstddef>

/**
 * Global memory allocator for guest address space.
 * Ensures no overlap between segments loaded by different components.
 */
class GuestMemoryAllocator {
public:
    static GuestMemoryAllocator& Get() {
        static GuestMemoryAllocator instance;
        return instance;
    }
    
    /**
     * Allocate a block of guest memory at the next available offset.
     * @param size Size to allocate (will be aligned to alignment boundary)
     * @param alignment Alignment boundary (default 4096 bytes = 1 page)
     * @return Offset in guest memory, or 0xFFFFFFFF if out of memory
     */
    uint32_t Allocate(size_t size, uint32_t alignment = 4096) {
        if (alignment == 0) alignment = 4096;
        
        // Align size to alignment boundary
        size = (size + alignment - 1) & ~(alignment - 1);
        
        // Check bounds (256MB max guest memory)
        if (current_offset + size > MAX_GUEST_MEMORY) {
            printf("[GuestMemoryAllocator] ERROR: Out of guest memory! "
                   "Requested 0x%zx bytes at offset 0x%08x, "
                   "but max is 0x%08x\n",
                   size, current_offset, MAX_GUEST_MEMORY);
            return 0xFFFFFFFF;
        }
        
        uint32_t result = current_offset;
        current_offset += size;
        
        printf("[GuestMemoryAllocator] Allocated 0x%zx bytes at offset 0x%08x "
               "(alignment 0x%x, next available: 0x%08x)\n",
               size, result, alignment, current_offset);
        
        return result;
    }
    
    /**
     * Get the current offset (for debugging).
     */
    uint32_t GetCurrentOffset() const {
        return current_offset;
    }
    
    /**
     * Reset allocator (for testing only).
     */
    void Reset() {
        current_offset = 0;
    }
    
private:
    GuestMemoryAllocator() : current_offset(0) {
        printf("[GuestMemoryAllocator] Initialized (max 256MB)\n");
    }
    
    ~GuestMemoryAllocator() = default;
    
    // Prevent copy/move
    GuestMemoryAllocator(const GuestMemoryAllocator&) = delete;
    GuestMemoryAllocator& operator=(const GuestMemoryAllocator&) = delete;
    
    static constexpr uint32_t MAX_GUEST_MEMORY = 256 * 1024 * 1024;  // 256MB
    uint32_t current_offset;
};

#endif // GUEST_MEMORY_ALLOCATOR_H
