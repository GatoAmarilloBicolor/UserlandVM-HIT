/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include <SupportDefs.h> // Para status_t
#include <stdint.h> // Para uint32_t

class GuestContext {
public:
	virtual ~GuestContext() = default;

	// Métodos para leer/escribir en la memoria del invitado a través del contexto.
	// Estas son funciones virtuales puras que deben ser implementadas por las clases derivadas.
	virtual status_t ReadGuestMemory(uint32_t guestAddress, void* buffer, size_t size) = 0;
	virtual status_t WriteGuestMemory(uint32_t guestAddress, const void* buffer, size_t size) = 0;

	// Guest exit status
	virtual bool ShouldExit() const { return false; }
	virtual void SetExit(bool exit_flag) {}

	// Futuras interfaces para acceder a registros, estado de la FPU, etc.
	// Ejemplo: virtual uint32_t GetRegister(int reg) = 0;
	// Ejemplo: virtual void SetRegister(int reg, uint32_t value) = 0;
};