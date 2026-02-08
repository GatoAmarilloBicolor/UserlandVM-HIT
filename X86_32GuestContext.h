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
	uint32_t eip; // Instruction Pointer
	uint32_t esp; // Stack Pointer
	uint32_t ebp; // Base Pointer
	uint32_t eax; // Accumulator
	uint32_t ebx; // Base
	uint32_t ecx; // Counter
	uint32_t edx; // Data
	uint32_t esi; // Source Index
	uint32_t edi; // Destination Index
	uint32_t eflags; // Flags Register
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
	
	// In direct memory mode on 64-bit host, we need to store 64-bit pointers
	// Store the actual EIP as a 64-bit pointer
	void SetEIP64(uintptr_t eip64) { fEIP64 = eip64; }
	uintptr_t GetEIP64() const { return fEIP64; }

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
	uintptr_t fEIP64 = 0;  // Store 64-bit EIP for direct memory mode on 64-bit host
};