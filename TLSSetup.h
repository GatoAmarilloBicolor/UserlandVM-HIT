/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef TLSSETUP_H
#define TLSSETUP_H

#include <OS.h>

// Forward declaration
class AddressSpace;

// TLS (Thread Local Storage) configuration for Haiku32
// The FS segment in x86-32 points to per-thread TLS data

class TLSSetup {
public:
	// Setup TLS area in guest memory
	// Must be called after guest address space is initialized
	static status_t Initialize(AddressSpace& addressSpace, uint32_t thread_id);
	
	// Get TLS base address (constant)
	static uint32_t GetTLSBase() { return TLS_BASE; }
	
	// Get size of TLS area
	static uint32_t GetTLSSize() { return TLS_SIZE; }

private:
	// TLS (Thread Local Storage) area base address
	// This is at the high end of user-space memory
	static const uint32_t TLS_BASE = 0xbffff000;
	
	// Size of TLS area (4KB)
	static const uint32_t TLS_SIZE = 0x1000;
	
	// TLS field offsets
	static const uint32_t TLS_THREAD_ID_OFFSET = 0;      // uint32_t thread_id
	static const uint32_t TLS_THREAD_INFO_OFFSET = 4;    // uint32_t thread_info_ptr
	static const uint32_t TLS_ERRNO_OFFSET = 8;          // uint32_t errno_location
	static const uint32_t TLS_ERRNO_STORAGE_OFFSET = 0x100;  // int errno storage
	
	// Helper to write a 32-bit value to guest memory
	static status_t WriteTLSValue(AddressSpace& addressSpace,
		uint32_t offset, uint32_t value);
};

#endif // TLSSETUP_H
