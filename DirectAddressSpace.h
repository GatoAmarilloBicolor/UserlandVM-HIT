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

#include "AddressSpace.h"
#include <SupportDefs.h>

// Implementación de AddressSpace que usa un área de memoria contigua
// con páginas de guarda para el espacio de direcciones del invitado.
class DirectAddressSpace : public AddressSpace {
public:
  DirectAddressSpace();
  virtual ~DirectAddressSpace();

  virtual status_t Init(size_t size);

  virtual status_t Read(uint32_t guestAddress, void *buffer,
                        size_t size) override;
  virtual status_t Write(uint32_t guestAddress, const void *buffer,
                         size_t size) override;
  virtual status_t ReadString(uint32_t guestAddress, char *buffer,
                              size_t bufferSize) override;

  // Devuelve la dirección base del espacio de memoria del invitado en el host.
  // Útil para la traducción directa de punteros (guest_ptr + base_offset).
  addr_t GuestBaseAddress() const { return fGuestBaseAddress; }

  // Virtual address mapping: Maps guest virtual addresses to offsets
  // Standard x86-32 binaries use 0x08048000 as code base
  virtual status_t RegisterMapping(uint32_t guest_vaddr, uint32_t guest_offset,
                                   size_t size) override;
  virtual uint32_t TranslateAddress(uint32_t guest_vaddr) const override;

  // TLS area setup - Maps TLS memory region
  virtual status_t MapTLSArea(uint32_t guest_vaddr, size_t size) override;

  // Direct memory read/write operations
  virtual status_t ReadMemory(uint32_t guest_vaddr, void *data,
                              size_t size) override;
  virtual status_t WriteMemory(uint32_t guest_vaddr, const void *data,
                               size_t size) override;

private:
  area_id fArea;
  addr_t fGuestBaseAddress;
  size_t fGuestSize;

  // Simple address translation table
  struct AddressMap {
    uint32_t vaddr_start;
    uint32_t vaddr_end;
    uint32_t offset;
  };

  static const int MAX_MAPPINGS = 16;
  AddressMap fMappings[MAX_MAPPINGS];
  int fMappingCount;
};
