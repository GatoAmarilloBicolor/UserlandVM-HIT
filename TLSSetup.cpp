/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TLSSetup.h"
#include "AddressSpace.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

status_t TLSSetup::Initialize(AddressSpace &addressSpace, uint32_t thread_id) {
  printf("[TLS] Initializing TLS area at 0x%08x (size=0x%x)\n", TLS_BASE,
         TLS_SIZE);

  // Step 1: Map TLS area in guest address space
  status_t status = addressSpace.MapTLSArea(TLS_BASE, TLS_SIZE);
  if (status != B_OK) {
    fprintf(stderr, "[TLS] Failed to map TLS area: %s\n", strerror(status));
    return status;
  }

  printf("[TLS] TLS area mapped successfully\n");

  // Step 2: Initialize Base Address field at slot 0 (TLS_BASE_ADDRESS_SLOT)
  status = WriteTLSValue(addressSpace, 0, TLS_BASE);
  if (status != B_OK) {
    fprintf(stderr, "[TLS] Failed to write TLS base address\n");
    return status;
  }
  printf("[TLS] Written TLS base address=0x%08x at slot 0\n", TLS_BASE);

  // Step 3: Initialize thread_id field at slot 1 (TLS_THREAD_ID_SLOT)
  status = WriteTLSValue(addressSpace, 4, thread_id);
  if (status != B_OK) {
    fprintf(stderr, "[TLS] Failed to write thread_id\n");
    return status;
  }
  printf("[TLS] Written thread_id=0x%08x at slot 1\n", thread_id);

  // Step 4: Initialize errno slot at slot 2 (TLS_ERRNO_SLOT)
  uint32_t errno_ptr = 0; // Will be set to 0 initially
  status = WriteTLSValue(addressSpace, 8, errno_ptr);
  if (status != B_OK) {
    fprintf(stderr, "[TLS] Failed to initialize errno slot\n");
    return status;
  }
  printf("[TLS] Initialized errno slot 2 to 0\n");

  printf("[TLS] TLS initialization complete\n");
  return B_OK;
}

status_t TLSSetup::WriteTLSValue(AddressSpace &addressSpace, uint32_t offset,
                                 uint32_t value) {
  uint32_t address = TLS_BASE + offset;
  return addressSpace.WriteMemory(address, &value, sizeof(value));
}
