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

	// Haiku OS Native Memory Management: Use create_area for optimal performance
	// This allows proper memory protection, caching, and integration with Haiku kernel
	void *memoryBase = NULL;
	area_id guestArea = create_area("userlandvm_guest_memory", &memoryBase,
		B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	
	if (guestArea < B_OK) {
		printf("[HAIKU] Failed to create guest memory area: %s\n", strerror(guestArea));
		return guestArea;
	}
	
	fArea = guestArea;
	fGuestBaseAddress = (addr_t)memoryBase;
	fGuestSize = size;
	
	printf("[HAIKU] Created guest memory area: id=%d, base=%p, size=%zu\n", 
		guestArea, memoryBase, size);

	return B_OK;
}

status_t
DirectAddressSpace::Read(uintptr_t guestAddress, void* buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;
	
	// If using direct memory mode, guest addresses are direct offsets into guest memory
	if (fUseDirectMemory) {
		// In direct memory mode, guestAddress is a guest virtual address that maps directly
		// to an offset in the allocated guest memory
		if (guestAddress + size > fGuestSize) {
			printf("[DirectAddressSpace::Read] ERROR: Address out of bounds in direct mode: guestAddr=0x%lx, size=%zu, guestSize=0x%lx\n",
				guestAddress, size, fGuestSize);
			return B_BAD_VALUE;
		}
		uint8_t* source = (uint8_t*)fGuestBaseAddress + guestAddress;
		memcpy(buffer, source, size);
		return B_OK;
	}
	
	// Translate virtual address to offset using mappings
	uintptr_t offset = TranslateAddress(guestAddress);
	
	// Check bounds: offset + size must not exceed fGuestSize
	// (uintptr_t)-1 is returned for unmapped addresses, will definitely fail this check
	if (offset == (uintptr_t)-1 || offset + size > fGuestSize) {
		printf("[DirectAddressSpace::Read] ERROR: Address not mapped: guestAddr=0x%lx, offset=0x%lx, size=%zu, guestSize=0x%lx\n",
			guestAddress, offset, size, fGuestSize);
		printf("[DirectAddressSpace::Read] Available mappings: %d\n", fMappingCount);
		for (int i = 0; i < fMappingCount; i++) {
			printf("  Mapping %d: 0x%lx-0x%lx -> offset 0x%lx\n",
				i, fMappings[i].vaddr_start, fMappings[i].vaddr_end, fMappings[i].offset);
		}
		return B_BAD_VALUE;
	}

	uint8_t* source = (uint8_t*)fGuestBaseAddress + offset;
	memcpy(buffer, source, size);
	return B_OK;
}

status_t
DirectAddressSpace::ReadString(uintptr_t guestAddress, char* buffer, size_t bufferSize)
{
	if (!buffer || bufferSize == 0)
		return B_BAD_VALUE;

	uintptr_t currentGuestAddress = guestAddress;
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
DirectAddressSpace::Write(uintptr_t guestAddress, const void* buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;
	
	// If using direct memory mode, guest addresses are direct offsets into guest memory
	if (fUseDirectMemory) {
		// In direct memory mode, guestAddress is a guest virtual address that maps directly
		// to an offset in the allocated guest memory
		if (guestAddress + size > fGuestSize) {
			printf("[DirectAddressSpace::Write] ERROR: Address out of bounds in direct mode: guestAddr=0x%lx, size=%zu, guestSize=0x%lx\n",
				guestAddress, size, fGuestSize);
			return B_BAD_VALUE;
		}
		uint8_t* dest = (uint8_t*)fGuestBaseAddress + guestAddress;
		memcpy(dest, buffer, size);
		return B_OK;
	}
	
	// Translate virtual address to offset
	uintptr_t offset = TranslateAddress(guestAddress);
	
	// Check bounds: offset + size must not exceed fGuestSize
	// (uintptr_t)-1 is returned for unmapped addresses, will definitely fail this check
	if (offset == (uintptr_t)-1 || offset + size > fGuestSize) {
		if (guestAddress >= 0x401e7b1c) {
			printf("DirectAddressSpace: Write FAILED: gaddr=0x%lx, offset=0x%lx, size=%zu, guestSize=0x%lx, offset+size=0x%lx\n",
				guestAddress, offset, size, fGuestSize, (uintptr_t)offset + size);
		}
		fflush(stdout);
		return B_BAD_VALUE;
	}

	uint8_t* dest = (uint8_t*)fGuestBaseAddress + offset;
	memcpy(dest, buffer, size);
	return B_OK;
}

status_t
DirectAddressSpace::RegisterMapping(uintptr_t guest_vaddr, uintptr_t guest_offset, size_t size)
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

uintptr_t
DirectAddressSpace::TranslateAddress(uintptr_t guest_vaddr) const
{
	for (int i = 0; i < fMappingCount; i++) {
		if (guest_vaddr >= fMappings[i].vaddr_start && 
		    guest_vaddr < fMappings[i].vaddr_end) {
			uintptr_t offset_in_region = guest_vaddr - fMappings[i].vaddr_start;
			uintptr_t result = fMappings[i].offset + offset_in_region;
			// Debug: log data section accesses
			if (guest_vaddr >= 0x401e7b1c && guest_vaddr < 0x40204298) {
				printf("[TranslateAddress] data access: vaddr=0x%08x → offset=0x%08x (mapping %d: 0x%08x-0x%08x+0x%08x)\n",
					guest_vaddr, result, i, fMappings[i].vaddr_start, fMappings[i].vaddr_end, fMappings[i].offset);
			}
			return result;
		}
	}

	// If no mapping found, print warning and return invalid offset
	// This will cause bounds check to fail in Read/Write
	if (guest_vaddr >= 0x40000000) {  // Only warn for guest VM addresses
		printf("DirectAddressSpace: WARNING - No mapping for guest vaddr 0x%lx (checked %d mappings)\n", 
			guest_vaddr, fMappingCount);
	}
	return (uintptr_t)-1;  // Return invalid offset that will fail bounds check
}

status_t
DirectAddressSpace::MapTLSArea(uintptr_t guest_vaddr, size_t size)
{
	// Register TLS area mapping
	// The TLS area is backed by real memory in fGuestBaseAddress
	
	if (fMappingCount >= MAX_MAPPINGS) {
		printf("DirectAddressSpace: Cannot map TLS - too many mappings\n");
		return B_NO_MEMORY;
	}
	
	// Calculate offset within guest memory
	// We'll place TLS at the very end of allocated memory
	uintptr_t offset = fGuestSize - size;
	
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
DirectAddressSpace::ReadMemory(uintptr_t guest_vaddr, void* data, size_t size)
{
	if (!data)
		return B_BAD_VALUE;
	
	return Read(guest_vaddr, data, size);
}

status_t
DirectAddressSpace::WriteMemory(uintptr_t guest_vaddr, const void* data, size_t size)
{
	if (!data)
		return B_BAD_VALUE;
	
	return Write(guest_vaddr, data, size);
}
