/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TLSSetup.h"
#include "AddressSpace.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

status_t TLSSetup::Initialize(AddressSpace& addressSpace, uint32_t thread_id)
{
	printf("[TLS] Initializing TLS area at 0x%08x (size=0x%x)\n", TLS_BASE, TLS_SIZE);
	
	// Step 1: Map TLS area in guest address space
	status_t status = addressSpace.MapTLSArea(TLS_BASE, TLS_SIZE);
	if (status != B_OK) {
		fprintf(stderr, "[TLS] Failed to map TLS area: %s\n", strerror(status));
		return status;
	}
	
	printf("[TLS] TLS area mapped successfully\n");
	
	// Step 2: Initialize thread_id field at offset 0
	status = WriteTLSValue(addressSpace, TLS_THREAD_ID_OFFSET, thread_id);
	if (status != B_OK) {
		fprintf(stderr, "[TLS] Failed to write thread_id\n");
		return status;
	}
	printf("[TLS] Written thread_id=0x%08x at offset 0x%x\n", 
		thread_id, TLS_THREAD_ID_OFFSET);
	
	// Step 3: Initialize thread_info_ptr at offset 4
	// For now, point to TLS base (self-reference as placeholder)
	uint32_t self_ptr = TLS_BASE;
	status = WriteTLSValue(addressSpace, TLS_THREAD_INFO_OFFSET, self_ptr);
	if (status != B_OK) {
		fprintf(stderr, "[TLS] Failed to write thread_info_ptr\n");
		return status;
	}
	printf("[TLS] Written thread_info_ptr=0x%08x at offset 0x%x\n", 
		self_ptr, TLS_THREAD_INFO_OFFSET);
	
	// Step 4: Initialize errno location at offset 8
	// Point to errno storage location within TLS
	uint32_t errno_location = TLS_BASE + TLS_ERRNO_STORAGE_OFFSET;
	status = WriteTLSValue(addressSpace, TLS_ERRNO_OFFSET, errno_location);
	if (status != B_OK) {
		fprintf(stderr, "[TLS] Failed to write errno_location\n");
		return status;
	}
	printf("[TLS] Written errno_location=0x%08x at offset 0x%x\n", 
		errno_location, TLS_ERRNO_OFFSET);
	
	// Step 5: Initialize errno variable at its storage location
	int errno_value = 0;
	status = addressSpace.WriteMemory(errno_location, &errno_value, sizeof(errno_value));
	if (status != B_OK) {
		fprintf(stderr, "[TLS] Failed to initialize errno variable\n");
		return status;
	}
	printf("[TLS] Initialized errno=0 at location 0x%08x\n", errno_location);
	
	printf("[TLS] TLS initialization complete\n");
	return B_OK;
}

status_t TLSSetup::WriteTLSValue(AddressSpace& addressSpace,
	uint32_t offset, uint32_t value)
{
	uint32_t address = TLS_BASE + offset;
	return addressSpace.WriteMemory(address, &value, sizeof(value));
}
