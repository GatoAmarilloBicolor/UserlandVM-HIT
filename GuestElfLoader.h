/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#pragma once

#include "AddressSpace.h"
#include "X86_32GuestContext.h"
#include "SymbolResolver.h"

#include <SupportDefs.h>
#include <elf.h>
#include <cstdio>
#include <memory>

// Clase para cargar binarios ELF de 32 bits para x86 en el espacio de memoria del invitado.
// Esta clase es compatible con la licencia MIT/BSD.
class GuestElfLoader {
public:
	// Carga el binario ELF especificado por 'path' en el 'context' del invitado.
	// Configura el punto de entrada (EIP) y la pila inicial con argc, argv, envp.
	// Si symbol_resolver es proporcionado, lo usa para resolver símbolos dinámicos
	static status_t Load(const char* path, X86_32GuestContext& context,
		AddressSpace& addressSpace, int argc, char* argv[], char* envp[],
		SymbolResolver* symbol_resolver = nullptr);

private:
	// No instanciable
	GuestElfLoader() = delete;
	~GuestElfLoader() = delete;
	
	// Helper: Process relocations for dynamic binaries
	static status_t ProcessRelocations(FILE* f, AddressSpace& addressSpace,
		uint32_t load_base, uint32_t rel_addr, uint32_t rel_size,
		uint32_t rel_entry_size, SymbolResolver* symbol_resolver = nullptr,
		const char* strtab = nullptr, const Elf32_Sym* symtab = nullptr,
		uint32_t symcount = 0);
};
