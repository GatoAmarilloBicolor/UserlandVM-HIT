/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "X86_32GuestContext.h"

#include <string.h>
#include <OS.h> // Incluido explícitamente aquí

X86_32GuestContext::X86_32GuestContext(AddressSpace& addressSpace)
	: fAddressSpace(addressSpace)
{
	// Inicializar todos los registros a cero por defecto.
	memset(&fRegisters, 0, sizeof(fRegisters));
	
	// Stack initialization (CRITICAL for GUI programs)
	// Stack is at 0xbfbf0000-0xbfff8000 (last 256KB of user space)
	// Initialize ESP to near top with safe margin
	fRegisters.esp = 0xbfff8000;  // Top of stack
	fRegisters.ebp = 0xbfff8000;  // Frame pointer = stack pointer initially
	
	// Initialize segment registers for 32-bit flat model
	// These may not be strictly needed in flat model but good practice
	// fRegisters.cs = 0;  // Code segment
	// fRegisters.ds = 0;  // Data segment
	// fRegisters.ss = 0;  // Stack segment
	// fRegisters.es = 0;  // Extra segment
	
	// Initialize FPU
	fFPU = std::make_unique<FloatingPointUnit>();
	fFPU->Init();
}


X86_32GuestContext::~X86_32GuestContext()
{
}


status_t
X86_32GuestContext::ReadGuestMemory(uint32_t guestAddress, void* buffer, size_t size)
{
	return fAddressSpace.Read(guestAddress, buffer, size);
}


status_t
X86_32GuestContext::WriteGuestMemory(uint32_t guestAddress, const void* buffer, size_t size)
{
	return fAddressSpace.Write(guestAddress, buffer, size);
}