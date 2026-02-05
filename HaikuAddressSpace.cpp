/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * Haiku OS Native AddressSpace Implementation
 * Uses Haiku's native area-based memory management
 */

#include "HaikuAddressSpace.h"
#include <string.h>
#include <cstdio>
#include <OS.h>
#include <kernel/OS.h>

HaikuAddressSpace::HaikuAddressSpace()
    : fArea(-1),
      fBaseAddress(nullptr),
      fSize(0),
      fMappingCount(0)
{
    // Initialize area mapping table
    for (int i = 0; i < MAX_AREAS; i++) {
        fAreas[i].vaddr_start = 0;
        fAreas[i].vaddr_end = 0;
        fAreas[i].offset = 0;
        fAreas[i].size = 0;
        fAreas[i].area_id = -1;
    }
}

HaikuAddressSpace::~HaikuAddressSpace()
{
    if (fArea >= 0) {
        delete_area(fArea);
    }
}

status_t
HaikuAddressSpace::Init(size_t size)
{
    if (fArea >= 0)
        return B_BAD_VALUE;

    // Haiku OS Native Memory Management: Use create_area for optimal performance
    // This allows proper memory protection, caching, and integration with Haiku kernel
    void *memoryBase = nullptr;
    fArea = create_area("userlandvm_guest_memory", &memoryBase,
                        B_ANY_ADDRESS, size, B_NO_LOCK, 
                        B_READ_AREA | B_WRITE_AREA);

    if (fArea < B_OK) {
        printf("[HAIKU] Failed to create guest memory area: %s\n", strerror(fArea));
        return fArea;
    }

    fBaseAddress = (addr_t)memoryBase;
    fSize = size;

    printf("[HAIKU] Haiku native memory area created: id=%ld, base=%p, size=%zu\n", 
           fArea, memoryBase, size);

    return B_OK;
}

status_t
HaikuAddressSpace::Read(uintptr_t guestAddress, void *buffer, size_t size)
{
    if (!buffer)
        return B_BAD_VALUE;
    
    if (fArea < 0)
        return B_NO_INIT;

    // Direct memory access using Haiku area mapping
    void *hostAddress = (void*)(fBaseAddress + guestAddress);
    
    // Check bounds
    if (guestAddress + size > fSize) {
        printf("[HAIKU] Read bounds error: addr=0x%lx, size=%zu, max=%zu\n", 
               guestAddress, size, fSize);
        return B_BAD_ADDRESS;
    }

    // Haiku-optimized: Use memcpy for maximum performance
    memcpy(buffer, hostAddress, size);
    return B_OK;
}

status_t
HaikuAddressSpace::Write(uintptr_t guestAddress, const void *buffer, size_t size)
{
    if (!buffer)
        return B_BAD_VALUE;
    
    if (fArea < 0)
        return B_NO_INIT;

    // Direct memory access using Haiku area mapping
    void *hostAddress = (void*)(fBaseAddress + guestAddress);
    
    // Check bounds
    if (guestAddress + size > fSize) {
        printf("[HAIKU] Write bounds error: addr=0x%lx, size=%zu, max=%zu\n", 
               guestAddress, size, fSize);
        return B_BAD_ADDRESS;
    }

    // Haiku-optimized: Use memcpy for maximum performance
    memcpy(hostAddress, buffer, size);
    return B_OK;
}

status_t
HaikuAddressSpace::ReadString(uintptr_t guestAddress, char *buffer, size_t bufferSize)
{
    if (!buffer || !bufferSize)
        return B_BAD_VALUE;
    
    if (fArea < 0)
        return B_NO_INIT;

    // Safe string read within Haiku area
    void *hostAddress = (void*)(fBaseAddress + guestAddress);
    
    size_t remaining = fSize - guestAddress;
    size_t readSize = (bufferSize < remaining) ? bufferSize : remaining;
    
    if (readSize > 0) {
        memcpy(buffer, hostAddress, readSize);
        buffer[readSize - 1] = '\0';  // Ensure null termination
    }
    
    return B_OK;
}

status_t
HaikuAddressSpace::RegisterMapping(uintptr_t guest_vaddr, uintptr_t guest_offset, size_t size)
{
    if (fMappingCount >= MAX_AREAS)
        return B_NO_MEMORY;
    
    // Store mapping in Haiku native format
    AreaMapping *mapping = &fAreas[fMappingCount++];
    mapping->vaddr_start = guest_vaddr;
    mapping->vaddr_end = guest_vaddr + size;
    mapping->offset = guest_offset;
    mapping->size = size;
    
    // Create a separate Haiku area for this mapping if needed
    void *mapBase = nullptr;
    area_id mapArea = create_area("userlandvm_mapping", &mapBase,
                               B_ANY_ADDRESS, size, B_NO_LOCK,
                               B_READ_AREA | B_WRITE_AREA);
    
    if (mapArea < B_OK) {
        printf("[HAIKU] Failed to create mapping area: %s\n", strerror(mapArea));
        return mapArea;
    }
    
    mapping->area_id = mapArea;
    
    printf("[HAIKU] Mapping registered: vaddr=0x%lx->0x%lx, size=%zu, area=%ld\n",
           guest_vaddr, guest_vaddr + size, size, mapArea);
    
    return B_OK;
}

uintptr_t
HaikuAddressSpace::TranslateAddress(uintptr_t guest_vaddr) const
{
    // Direct address translation using Haiku area base
    return fBaseAddress + guest_vaddr;
}

status_t
HaikuAddressSpace::MapTLSArea(uintptr_t guest_vaddr, size_t size)
{
    // Create a separate Haiku area for TLS
    void *tlsBase = nullptr;
    area_id tlsArea = create_area("userlandvm_tls", &tlsBase,
                             B_ANY_ADDRESS, size, B_NO_LOCK,
                             B_READ_AREA | B_WRITE_AREA);
    
    if (tlsArea < B_OK) {
        printf("[HAIKU] Failed to create TLS area: %s\n", strerror(tlsArea));
        return tlsArea;
    }
    
    printf("[HAIKU] TLS area created: base=%p, size=%zu\n", tlsBase, size);
    return B_OK;
}

status_t
HaikuAddressSpace::ReadMemory(uintptr_t guestAddress, void *data, size_t size)
{
    return Read(guestAddress, data, size);
}

status_t
HaikuAddressSpace::WriteMemory(uintptr_t guestAddress, const void *data, size_t size)
{
    return Write(guestAddress, data, size);
}

addr_t
HaikuAddressSpace::GuestBaseAddress() const
{
    return fBaseAddress;
}

size_t
HaikuAddressSpace::GuestSize() const
{
    return fSize;
}

// Haiku-specific debugging
void
HaikuAddressSpace::DumpMemoryInfo() const
{
    printf("[HAIKU] Memory Dump:\n");
    printf("  Main area ID: %ld\n", fArea);
    printf("  Base address: 0x%lx\n", fBaseAddress);
    printf("  Total size: %zu bytes\n", fSize);
    printf("  Mappings: %d/%d\n", fMappingCount, MAX_AREAS);
    
    for (int i = 0; i < fMappingCount; i++) {
        const AreaMapping *m = &fAreas[i];
        printf("  Mapping[%d]: vaddr=0x%lx->0x%lx, size=%zu, area=%ld\n",
               i, m->vaddr_start, m->vaddr_end, m->size, m->area_id);
    }
}