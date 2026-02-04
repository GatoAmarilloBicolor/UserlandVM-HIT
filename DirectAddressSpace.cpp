/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "DirectAddressSpace.h"

#include <string.h>
#include <cstdio>
#include <stdlib.h>
#include <OS.h>
#include <kernel/OS.h>

DirectAddressSpace::DirectAddressSpace()
    : fArea(-1),
      fGuestBaseAddress(0),
      fGuestSize(0),
      fMappingCount(0)
{
	// Initialize address mapping table
	for (int i = 0; i < MAX_MAPPINGS; i++) {
		fMappings[i].vaddr_start = 0;
		fMappings[i].vaddr_end = 0;
		fMappings[i].offset = 0;
	}
}

DirectAddressSpace::~DirectAddressSpace()
{
	if (fArea >= 0) {
		delete_area(fArea);
	}
}

status_t
DirectAddressSpace::Init(size_t size)
{
	if (fArea >= 0)
		return B_BAD_VALUE;

	// Align to page size
	size_t pageSize = B_PAGE_SIZE;
	size = (size + pageSize - 1) & ~(pageSize - 1);

	// For Sprint 2: Use malloc instead of create_area due to space constraints
	// Full area-based implementation will be optimized in Sprint 3+
	fGuestBaseAddress = (addr_t)malloc(size);
	if (!fGuestBaseAddress) {
		printf("DirectAddressSpace: Failed to allocate guest memory\n");
		return B_NO_MEMORY;
	}

	fArea = 1;  // Mark as initialized
	fGuestSize = size;
	printf("DirectAddressSpace: Memory allocated at 0x%lx size %ld bytes\n",
		fGuestBaseAddress, fGuestSize);

	return B_OK;
}

status_t
DirectAddressSpace::Read(uint32_t guestAddress, void* buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;
	
	// If using direct memory mode, treat guest address as direct host pointer
	if (fUseDirectMemory) {
		// Direct access: guestAddress is a host pointer value (64-bit value)
		uintptr_t hostAddr = (uintptr_t)guestAddress;
		uint8_t* source = (uint8_t*)hostAddr;
		memcpy(buffer, source, size);
		return B_OK;
	}
	
	// Translate virtual address to offset
	uint32_t offset = TranslateAddress(guestAddress);
	
	// Check bounds: offset + size must not exceed fGuestSize
	// 0xFFFFFFFF is returned for unmapped addresses, will definitely fail this check
	if (offset == 0xFFFFFFFF || offset + size > fGuestSize) {
		printf("[DirectAddressSpace::Read] ERROR: Address not mapped: guestAddr=0x%08x, offset=0x%08x, size=%zu, guestSize=0x%lx\n",
			guestAddress, offset, size, fGuestSize);
		printf("[DirectAddressSpace::Read] Available mappings: %d\n", fMappingCount);
		for (int i = 0; i < fMappingCount; i++) {
			printf("  Mapping %d: 0x%08x-0x%08x -> offset 0x%08x\n",
				i, fMappings[i].vaddr_start, fMappings[i].vaddr_end, fMappings[i].offset);
		}
		return B_BAD_VALUE;
	}

	uint8_t* source = (uint8_t*)fGuestBaseAddress + offset;
	memcpy(buffer, source, size);
	return B_OK;
}

status_t
DirectAddressSpace::ReadString(uint32_t guestAddress, char* buffer, size_t bufferSize)
{
	if (!buffer || bufferSize == 0)
		return B_BAD_VALUE;

	uint32_t currentGuestAddress = guestAddress;
	for (size_t i = 0; i < bufferSize; ++i) {
		uint8_t byte;
		status_t status = Read(currentGuestAddress, &byte, 1);
		if (status != B_OK) {
			buffer[i] = '\0'; // Ensure null termination on error
			return status;
		}
		buffer[i] = (char)byte;
		if (byte == '\0')
			return B_OK;
		currentGuestAddress++;
	}

	buffer[bufferSize - 1] = '\0'; // Ensure null termination if buffer is full
	return B_BUFFER_OVERFLOW;
}

status_t
DirectAddressSpace::Write(uint32_t guestAddress, const void* buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;
	
	// If using direct memory mode, treat guest address as direct host pointer
	if (fUseDirectMemory) {
		// Direct access: guestAddress is a host pointer value
		uint8_t* dest = (uint8_t*)guestAddress;
		memcpy(dest, buffer, size);
		return B_OK;
	}
	
	// Translate virtual address to offset
	uint32_t offset = TranslateAddress(guestAddress);
	
	// Check bounds: offset + size must not exceed fGuestSize
	// 0xFFFFFFFF is returned for unmapped addresses, will definitely fail this check
	if (offset == 0xFFFFFFFF || offset + size > fGuestSize) {
		if (guestAddress >= 0x401e7b1c) {
			printf("DirectAddressSpace: Write FAILED: gaddr=0x%08x, offset=0x%08x, size=%zu, guestSize=0x%lx, offset+size=0x%lx\n",
				guestAddress, offset, size, fGuestSize, (uint64_t)offset + size);
		}
		fflush(stdout);
		return B_BAD_VALUE;
	}

	uint8_t* dest = (uint8_t*)fGuestBaseAddress + offset;
	memcpy(dest, buffer, size);
	return B_OK;
}

status_t
DirectAddressSpace::RegisterMapping(uint32_t guest_vaddr, uint32_t guest_offset, size_t size)
{
	if (fMappingCount >= MAX_MAPPINGS) {
		printf("DirectAddressSpace: Too many address mappings\n");
		return B_NO_MEMORY;
	}

	fMappings[fMappingCount].vaddr_start = guest_vaddr;
	fMappings[fMappingCount].vaddr_end = guest_vaddr + size;
	fMappings[fMappingCount].offset = guest_offset;
	fMappingCount++;

	printf("DirectAddressSpace: Registered mapping 0x%08x-0x%08lx -> offset 0x%08x\n",
		guest_vaddr, (long unsigned int)(guest_vaddr + size), guest_offset);

	return B_OK;
}

uint32_t
DirectAddressSpace::TranslateAddress(uint32_t guest_vaddr) const
{
	for (int i = 0; i < fMappingCount; i++) {
		if (guest_vaddr >= fMappings[i].vaddr_start && 
		    guest_vaddr < fMappings[i].vaddr_end) {
			uint32_t offset_in_region = guest_vaddr - fMappings[i].vaddr_start;
			uint32_t result = fMappings[i].offset + offset_in_region;
			// Debug: log data section accesses
			if (guest_vaddr >= 0x401e7b1c && guest_vaddr < 0x40204298) {
				printf("[TranslateAddress] data access: vaddr=0x%08x → offset=0x%08x (mapping %d: 0x%08x-0x%08x+0x%08x)\n",
					guest_vaddr, result, i, fMappings[i].vaddr_start, fMappings[i].vaddr_end, fMappings[i].offset);
			}
			return result;
		}
	}

	// If no mapping found, print warning and return offset beyond bounds
	// This will cause bounds check to fail in Read/Write
	if (guest_vaddr >= 0x40000000) {  // Only warn for guest VM addresses
		printf("DirectAddressSpace: WARNING - No mapping for guest vaddr 0x%08x (checked %d mappings)\n", 
			guest_vaddr, fMappingCount);
	}
	return 0xFFFFFFFF;  // Return invalid offset that will fail bounds check
}

status_t
DirectAddressSpace::MapTLSArea(uint32_t guest_vaddr, size_t size)
{
	// Register TLS area mapping
	// The TLS area is backed by real memory in fGuestBaseAddress
	
	if (fMappingCount >= MAX_MAPPINGS) {
		printf("DirectAddressSpace: Cannot map TLS - too many mappings\n");
		return B_NO_MEMORY;
	}
	
	// Calculate offset within guest memory
	// We'll place TLS at the very end of allocated memory
	uint32_t offset = fGuestSize - size;
	
	// Register the mapping
	fMappings[fMappingCount].vaddr_start = guest_vaddr;
	fMappings[fMappingCount].vaddr_end = guest_vaddr + size;
	fMappings[fMappingCount].offset = offset;
	fMappingCount++;
	
	printf("DirectAddressSpace: Mapped TLS area 0x%08x-0x%08lx -> offset 0x%08x\n",
		guest_vaddr, guest_vaddr + size, offset);
	
	return B_OK;
}

status_t
DirectAddressSpace::ReadMemory(uint32_t guest_vaddr, void* data, size_t size)
{
	if (!data)
		return B_BAD_VALUE;
	
	return Read(guest_vaddr, data, size);
}

status_t
DirectAddressSpace::WriteMemory(uint32_t guest_vaddr, const void* data, size_t size)
{
	if (!data)
		return B_BAD_VALUE;
	
	return Write(guest_vaddr, data, size);
}
