/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include "GuestContext.h"
#include "AddressSpace.h"
#include "FloatingPointUnit.h"

#include <SupportDefs.h>
#include <OS.h> // Incluido explícitamente para status_t y otros tipos de Haiku
#include <stdint.h>
#include <memory>

// Estructura para representar los registros de la CPU x86-32 del invitado.
// Se mantiene simple por ahora, se expandirá según sea necesario.
struct X86_32Registers {
	uint32 eip; // Instruction Pointer
	uint32 esp; // Stack Pointer
	uint32 ebp; // Base Pointer
	uint32 eax; // Accumulator
	uint32 ebx; // Base
	uint32 ecx; // Counter
	uint32 edx; // Data
	uint32 esi; // Source Index
	uint32 edi; // Destination Index
	uint32 eflags; // Flags Register
	// Otros registros de segmento (cs, ds, ss, es, fs, gs) y de control (cr0, cr3, etc.)
	// se añadirán cuando sea necesario para una emulación más completa.
};

// Implementación de GuestContext para la arquitectura x86-32.
class X86_32GuestContext : public GuestContext {
public:
	X86_32GuestContext(AddressSpace& addressSpace);
	virtual ~X86_32GuestContext();

	// Acceso a los registros del invitado.
	X86_32Registers& Registers() { return fRegisters; }
	const X86_32Registers& Registers() const { return fRegisters; }

	// Métodos para leer/escribir en la memoria del invitado a través del contexto.
	// Estos delegan en el AddressSpace proporcionado.
	virtual status_t ReadGuestMemory(uint32_t guestAddress, void* buffer, size_t size) override;
	virtual status_t WriteGuestMemory(uint32_t guestAddress, const void* buffer, size_t size) override;

	// Guest exit status
	virtual bool ShouldExit() const override { return fShouldExit; }
	virtual void SetExit(bool exit_flag) override { fShouldExit = exit_flag; }

	// Image base address (for relative address calculations in PLT/GOT)
	uint32_t GetImageBase() const { return fImageBase; }
	void SetImageBase(uint32_t base) { fImageBase = base; }

	// FPU (Floating Point Unit) access
	FloatingPointUnit* GetFPU() { return fFPU.get(); }
	const FloatingPointUnit* GetFPU() const { return fFPU.get(); }

private:
	X86_32Registers fRegisters;
	AddressSpace& fAddressSpace; // Referencia al espacio de direcciones del invitado
	bool fShouldExit = false;
	uint32_t fImageBase = 0x40000000;  // Default base for ET_DYN binaries
	std::unique_ptr<FloatingPointUnit> fFPU;  // Floating Point Unit for x87 operations
};