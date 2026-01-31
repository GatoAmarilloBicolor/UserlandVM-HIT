/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "SymbolResolver.h"
#include <stdio.h>
#include <string.h>

SymbolResolver::SymbolResolver()
{
}

SymbolResolver::~SymbolResolver()
{
}

status_t SymbolResolver::RegisterLibrary(const Library& lib)
{
	printf("[SymbolResolver] Registering library: %s (base=0x%08x, %zu symbols)\n",
		lib.path.c_str(), lib.baseAddress, lib.symbols.size());

	// Registrar todos los símbolos de la librería
	for (const auto& sym : lib.symbols) {
		if (sym.name.empty()) continue;  // Skip unnamed symbols

		uint8_t binding = ELF32_ST_BIND(sym.binding);
		
		// Manejar símbolos débiles
		if (binding == STB_WEAK) {
			// Si ya existe un símbolo fuerte, no sobrescribir
			auto it = fSymbolTable.find(sym.name);
			if (it != fSymbolTable.end()) {
				uint8_t existing_binding = ELF32_ST_BIND(it->second.binding);
				if (existing_binding != STB_WEAK) {
					printf("[SymbolResolver]   Weak symbol '%s' shadowed by strong symbol\n",
						sym.name.c_str());
					continue;
				}
			}
			fWeakSymbols[sym.name] = sym;
		}

		// Registrar símbolo
		if (fSymbolTable.find(sym.name) != fSymbolTable.end()) {
			printf("[SymbolResolver]   Symbol '%s' already registered, updating\n",
				sym.name.c_str());
		}
		fSymbolTable[sym.name] = sym;
		printf("[SymbolResolver]   + %s @ 0x%08x\n", sym.name.c_str(), sym.address);
	}

	// Registrar librería
	fLibraries.push_back(lib);
	return B_OK;
}

uint32_t SymbolResolver::ResolveSymbol(const char* name)
{
	if (!name) return 0;

	auto it = fSymbolTable.find(name);
	if (it != fSymbolTable.end()) {
		return it->second.address;
	}

	printf("[SymbolResolver] Symbol '%s' not found\n", name);
	return 0;
}

status_t SymbolResolver::ResolveSymbolWithInfo(const char* name, Symbol& out_symbol)
{
	if (!name) return B_BAD_VALUE;

	auto it = fSymbolTable.find(name);
	if (it != fSymbolTable.end()) {
		out_symbol = it->second;
		return B_OK;
	}

	return B_NAME_NOT_FOUND;
}

void SymbolResolver::PrintAllSymbols() const
{
	printf("\n[SymbolResolver] === All Loaded Symbols ===\n");
	printf("Total: %zu symbols\n\n", fSymbolTable.size());

	for (const auto& pair : fSymbolTable) {
		const Symbol& sym = pair.second;
		const char* type_str = "?";
		
		switch (ELF32_ST_TYPE(sym.type)) {
			case STT_OBJECT:  type_str = "OBJ"; break;
			case STT_FUNC:    type_str = "FUN"; break;
			case STT_SECTION: type_str = "SEC"; break;
			case STT_FILE:    type_str = "FIL"; break;
			case STT_NOTYPE:  type_str = "NON"; break;
		}

		const char* bind_str = "?";
		switch (ELF32_ST_BIND(sym.binding)) {
			case STB_LOCAL:  bind_str = "LOC"; break;
			case STB_GLOBAL: bind_str = "GLB"; break;
			case STB_WEAK:   bind_str = "WEK"; break;
		}

		printf("  [%s] [%s] 0x%08x %6u  %s\n",
			bind_str, type_str, sym.address, sym.size, sym.name.c_str());
	}
	printf("\n");
}

void SymbolResolver::PrintLibrarySymbols(const char* lib_path) const
{
	for (const auto& lib : fLibraries) {
		if (lib.path.find(lib_path) != std::string::npos) {
			printf("\n[SymbolResolver] === Symbols in %s ===\n", lib.path.c_str());
			printf("Base: 0x%08x, Size: 0x%08x\n", lib.baseAddress, lib.size);
			printf("Total: %zu symbols\n\n", lib.symbols.size());

			for (const auto& sym : lib.symbols) {
				if (sym.name.empty()) continue;
				printf("  0x%08x %6u  %s\n", sym.address, sym.size, sym.name.c_str());
			}
			printf("\n");
			return;
		}
	}
	printf("[SymbolResolver] Library '%s' not found\n", lib_path);
}

uint32_t SymbolResolver::ResolveSymbolInLibrary(const char* symbol_name, const char* lib_path)
{
	for (const auto& lib : fLibraries) {
		if (lib.path.find(lib_path) == std::string::npos) continue;

		for (const auto& sym : lib.symbols) {
			if (sym.name == symbol_name) {
				return sym.address;
			}
		}
	}
	return 0;
}

bool SymbolResolver::SymbolExists(const char* name) const
{
	if (!name) return false;
	return fSymbolTable.find(name) != fSymbolTable.end();
}

// Static method to resolve symbol from raw ELF tables (from DYNAMIC segment)
uint32_t SymbolResolver::ResolveSymbolFromELF(
	const Elf32_Sym* symtab, uint32_t symcount,
	const char* strtab, uint32_t strtab_size,
	const char* symbol_name,
	uint32_t base_address)
{
	if (!symtab || !strtab || !symbol_name) {
		printf("[SymbolResolver] ResolveSymbolFromELF: invalid parameters\n");
		return 0;
	}

	if (symcount == 0) {
		printf("[SymbolResolver] ResolveSymbolFromELF: empty symbol table\n");
		return 0;
	}

	printf("[SymbolResolver] ResolveSymbolFromELF: searching for '%s' in %u symbols\n",
		symbol_name, symcount);

	// Linear search through symbol table
	for (uint32_t i = 0; i < symcount; i++) {
		const Elf32_Sym& sym = symtab[i];

		// Skip unnamed symbols
		if (sym.st_name == 0) continue;

		// Bounds check on string table offset
		if (sym.st_name >= strtab_size) {
			printf("[SymbolResolver]   WARNING: Symbol %u has invalid string offset 0x%x >= 0x%x\n",
				i, sym.st_name, strtab_size);
			continue;
		}

		const char* sym_name = &strtab[sym.st_name];

		// Match symbol name
		if (strcmp(sym_name, symbol_name) == 0) {
			// Found the symbol
			uint32_t symbol_address = base_address + sym.st_value;

			uint8_t binding = ELF32_ST_BIND(sym.st_info);
			uint8_t type = ELF32_ST_TYPE(sym.st_info);

			const char* binding_str = "?";
			switch (binding) {
				case STB_LOCAL:  binding_str = "LOCAL"; break;
				case STB_GLOBAL: binding_str = "GLOBAL"; break;
				case STB_WEAK:   binding_str = "WEAK"; break;
			}

			const char* type_str = "?";
			switch (type) {
				case STT_NOTYPE:  type_str = "NOTYPE"; break;
				case STT_OBJECT:  type_str = "OBJECT"; break;
				case STT_FUNC:    type_str = "FUNC"; break;
				case STT_SECTION: type_str = "SECTION"; break;
				case STT_FILE:    type_str = "FILE"; break;
			}

			printf("[SymbolResolver]   FOUND: '%s' @ 0x%08x (size=%u, binding=%s, type=%s, shndx=%u)\n",
				sym_name, symbol_address, sym.st_size, binding_str, type_str, sym.st_shndx);

			return symbol_address;
		}
	}

	printf("[SymbolResolver] Symbol '%s' not found in symbol table\n", symbol_name);
	return 0;
}
