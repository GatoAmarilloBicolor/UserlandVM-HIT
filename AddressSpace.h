/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include <SupportDefs.h>
#include <stdint.h>

// Interfaz para la gestión de la memoria del invitado.
// Abstrae si la memoria es un bloque contiguo, paginada, etc.
class AddressSpace {
public:
	virtual ~AddressSpace() = default;

	virtual status_t Read(uint32_t guestAddress, void* buffer, size_t size) = 0;
	virtual status_t ReadString(uint32_t guestAddress, char* buffer, size_t bufferSize) = 0;
	virtual status_t Write(uint32_t guestAddress, const void* buffer, size_t size) = 0;

	// Virtual address mapping (optional, default implementations do nothing)
	virtual status_t RegisterMapping(uint32_t guest_vaddr, uint32_t guest_offset, size_t size)
	{
		return B_OK;  // Default: ignore mapping
	}
	
	virtual uint32_t TranslateAddress(uint32_t guest_vaddr) const
	{
		return guest_vaddr;  // Default: no translation
	}
	
	// TLS area setup (optional)
	virtual status_t MapTLSArea(uint32_t guest_vaddr, size_t size)
	{
		return B_OK;  // Default: ignore
	}
	
	// Direct memory operations (optional, can be overridden)
	virtual status_t ReadMemory(uint32_t guestAddress, void* data, size_t size)
	{
		return Read(guestAddress, data, size);
	}
	
	virtual status_t WriteMemory(uint32_t guestAddress, const void* data, size_t size)
	{
		return Write(guestAddress, data, size);
	}

	// Helper para leer un solo valor
	template<typename T>
	status_t Read(uint32_t guestAddress, T& value)
	{
		return Read(guestAddress, &value, sizeof(T));
	}

	// Helper para escribir un solo valor
	template<typename T>
	status_t Write(uint32_t guestAddress, const T& value)
	{
		return Write(guestAddress, &value, sizeof(T));
	}
};
