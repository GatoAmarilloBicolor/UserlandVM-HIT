/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _SYMBOL_RESOLVER_H_
#define _SYMBOL_RESOLVER_H_

#include <SupportDefs.h>
#include <elf.h>
#include <map>
#include <string>
#include <vector>

// Forward declarations
struct DynamicInfo;

// Información de un símbolo
struct Symbol {
	std::string name;
	uint32_t address;
	uint32_t size;
	uint8_t binding;       // STB_LOCAL, STB_GLOBAL, STB_WEAK
	uint8_t type;          // STT_OBJECT, STT_FUNC, STT_SECTION, etc.
	uint16_t shndx;        // Índice de sección
};

// Información de una biblioteca
struct Library {
	std::string path;
	std::string soname;
	uint32_t baseAddress;
	uint32_t size;
	std::vector<Symbol> symbols;
};

// Resolutor de símbolos para linking dinámico
// Mantiene un registro de librerías cargadas y resuelve símbolos
// Soporta:
// - Búsqueda de símbolos en múltiples librerías
// - Resolución de símbolos débiles (weak)
// - Shadowing de símbolos
class SymbolResolver {
public:
	SymbolResolver();
	~SymbolResolver();

	// Registrar una biblioteca con sus símbolos
	status_t RegisterLibrary(const Library& lib);

	// Resolver un símbolo global
	// Retorna la dirección o 0 si no se encuentra
	uint32_t ResolveSymbol(const char* name);

	// Resolver un símbolo con información detallada
	status_t ResolveSymbolWithInfo(const char* name, Symbol& out_symbol);

	// Listar todos los símbolos cargados
	void PrintAllSymbols() const;

	// Listar símbolos de una librería específica
	void PrintLibrarySymbols(const char* lib_path) const;

	// Buscar un símbolo en una librería específica
	uint32_t ResolveSymbolInLibrary(const char* symbol_name, const char* lib_path);

	// Verificar si un símbolo está disponible
	bool SymbolExists(const char* name) const;

	// Contar símbolos resueltos
	size_t SymbolCount() const { return fSymbolTable.size(); }

	// Resolver símbolo desde tablas ELF crudas (para DYNAMIC segment)
	// Busca un símbolo en la tabla de símbolos ELF dada
	static uint32_t ResolveSymbolFromELF(
		const Elf32_Sym* symtab, uint32_t symcount,
		const char* strtab, uint32_t strtab_size,
		const char* symbol_name,
		uint32_t base_address);

private:
	// Tabla de símbolos: nombre → información de símbolo
	std::map<std::string, Symbol> fSymbolTable;

	// Registro de librerías
	std::vector<Library> fLibraries;

	// Tabla de símbolos débiles (pueden ser sobrescritos)
	std::map<std::string, Symbol> fWeakSymbols;
};

#endif // _SYMBOL_RESOLVER_H_
