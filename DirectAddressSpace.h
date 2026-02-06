/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _DIRECT_ADDRESS_SPACE_H
#define _DIRECT_ADDRESS_SPACE_H

// Fix for __GLIBC_USE not defined errors on Linux
#ifdef __linux__
#include <features.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// Standard type definitions for cross-platform compatibility
typedef uint32_t addr_t;
typedef uint32_t area_id;

// Include our interfaces
#include "AddressSpace.h"

// Use standard POSIX includes instead of Haiku-specific
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <algorithm>

// Implementación de AddressSpace que usa un área de memoria contigua
// con páginas de guarda para el espacio de direcciones del invitado.
class DirectAddressSpace : public AddressSpace {
public:
  DirectAddressSpace();
  virtual ~DirectAddressSpace();

  virtual status_t Init(size_t size);
  
  // Set guest memory base for direct memory access (bypass translation)
  // This allows using malloc'd host memory directly for guest images
  void SetGuestMemoryBase(addr_t base, size_t size) {
    fGuestBaseAddress = base;
    fGuestSize = size;
    fUseDirectMemory = true;
  }

  virtual status_t Read(uintptr_t guestAddress, void *buffer,
                        size_t size) override;
  virtual status_t Write(uintptr_t guestAddress, const void *buffer,
                         size_t size) override;
  virtual status_t ReadString(uintptr_t guestAddress, char *buffer,
                              size_t bufferSize) override;

  // Devuelve la dirección base del espacio de memoria del invitado en el host.
  // Útil para la traducción directa de punteros (guest_ptr + base_offset).
  addr_t GuestBaseAddress() const { return fGuestBaseAddress; }

  // Virtual address mapping: Maps guest virtual addresses to offsets
  // Standard x86-32 binaries use 0x08048000 as code base
  virtual status_t RegisterMapping(uintptr_t guest_vaddr, uintptr_t guest_offset,
                                   size_t size) override;
  virtual uintptr_t TranslateAddress(uintptr_t guest_vaddr) const override;

  // TLS area setup - Maps TLS memory region
  virtual status_t MapTLSArea(uintptr_t guest_vaddr, size_t size) override;

  // Direct memory read/write operations
  virtual status_t ReadMemory(uintptr_t guest_vaddr, void *data,
                              size_t size) override;
  virtual status_t WriteMemory(uintptr_t guest_vaddr, const void *data,
                               size_t size) override;

private:
  area_id fArea;
  addr_t fGuestBaseAddress;
  size_t fGuestSize;
  bool fUseDirectMemory = false;  // If true, bypass translation and use direct memory access

  // Simple address translation table
  struct AddressMap {
    uintptr_t vaddr_start;
    uintptr_t vaddr_end;
    uintptr_t offset;
  };

  static const int MAX_MAPPINGS = 16;
  AddressMap fMappings[MAX_MAPPINGS];
  int fMappingCount;
};

#endif
