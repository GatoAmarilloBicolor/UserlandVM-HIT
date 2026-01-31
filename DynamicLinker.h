/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _DYNAMIC_LINKER_H_
#define _DYNAMIC_LINKER_H_

#include <SupportDefs.h>
#include <elf.h>
#include <map>
#include <string>
#include <vector>

// Forward declaration
struct DynamicInfo;

class AddressSpace;
class X86_32GuestContext;

// Estructura para representar un símbolo resuelto
struct ResolvedSymbol {
	std::string name;
	uint32_t address;      // Dirección en guest memory
	uint32_t size;
	uint8_t binding;       // STB_LOCAL, STB_GLOBAL, STB_WEAK
	uint8_t type;          // STT_OBJECT, STT_FUNC, etc.
	bool resolved;
};

// Estructura para representar una biblioteca cargada
struct LoadedLibrary {
	std::string path;
	uint32_t baseAddress;  // Dirección base en guest memory
	uint32_t size;
	Elf32_Sym* symtab;     // Tabla de símbolos
	uint32_t symcount;
	char* strtab;          // Tabla de strings
	uint32_t strtab_size;
};

// Dynamic Linker para cargar y enlazar librerías compartidas x86-32
// Soporta:
// - Parsing de segmento ELF DYNAMIC
// - Resolución de símbolos
// - Carga de librerías .so
// - Relocation processing
class DynamicLinker {
public:
	DynamicLinker(AddressSpace& addressSpace, X86_32GuestContext& context);
	~DynamicLinker();

	// Cargar un binario dinámico (ELF con DYNAMIC segment)
	status_t LoadDynamicBinary(const char* path, uint32& entry_point);

	// Cargar una biblioteca compartida (.so)
	status_t LoadSharedLibrary(const char* path, uint32& base_address);

	// Resolver un símbolo global
	status_t ResolveSymbol(const char* name, ResolvedSymbol& out_symbol);

	// Procesar relocaciones de un binario
	status_t ProcessRelocations(const char* binary_path);

	// Apply relocations from DYNAMIC info to loaded memory
	status_t ApplyRelocations(const DynamicInfo& dyn_info, uint32_t base_address);

	// Apply a single relocation entry
	status_t ApplySingleRelocation(const Elf32_Rel* rel, uint32_t base_address,
		const DynamicInfo& dyn_info);

	// Apply a single relocation entry with addend
	status_t ApplySingleRela(const Elf32_Rela* rela, uint32_t base_address,
		const DynamicInfo& dyn_info);

	// Obtener la dirección de un símbolo resuelto
	uint32_t GetSymbolAddress(const char* name);

	// Debug: listar símbolos cargados
	void PrintLoadedSymbols() const;

	// Parse DYNAMIC segment
	// base_address: load base for ET_DYN binaries (0 for ET_EXEC)
	status_t ParseDynamic(const void* dynamicData, uint32_t dynamicSize,
		DynamicInfo& outInfo, uint32_t base_address = 0);

	// Load symbol table from parsed DYNAMIC info and register with resolver
	status_t LoadSymbolsFromDynamic(const DynamicInfo& dyn_info, 
		const char* library_path, uint32_t base_address);

	// Resolve a symbol from loaded libraries
	uint32_t ResolveSymbolAddress(const char* symbol_name);

	// Extract library names from DYNAMIC segment (DT_NEEDED entries)
	status_t ExtractNeededLibraries(const DynamicInfo& dyn_info,
		std::vector<std::string>& out_libraries);

	// Find library file in sysroot paths
	// Returns full path or empty string if not found
	std::string FindLibraryFile(const char* lib_name);

	// Load a shared library and register its symbols
	status_t LoadLibrary(const char* lib_path, const char* lib_name);

	// Load all required libraries from DT_NEEDED
	status_t LoadRequiredLibraries(const DynamicInfo& dyn_info);

private:
	AddressSpace& fAddressSpace;
	X86_32GuestContext& fContext;

	// Mapeo de librerías cargadas
	std::map<std::string, LoadedLibrary> fLoadedLibraries;

	// Cache de símbolos resueltos
	std::map<std::string, ResolvedSymbol> fResolvedSymbols;

	// Tabla de símbolos del binario principal (para resolver símbolos débiles)
	Elf32_Sym* fMainSymtab;
	uint32_t fMainSymcount;
	char* fMainStrtab;
	uint32_t fMainStrtab_size;
	uint32_t fMainBase;

	// Dirección actual de carga para librerías
	uint32_t fNextLoadAddress;

	// Métodos privados para parsing ELF
	status_t ParseDynamicSegment(FILE* file, Elf32_Ehdr* ehdr, 
		Elf32_Phdr** dyn_phdr_out);

	status_t LoadSymbolTable(FILE* file, const LoadedLibrary& lib);

	status_t ProcessDynamicRelocations(FILE* file, Elf32_Ehdr* ehdr, 
		const LoadedLibrary& lib);

	// Helper para encontrar sección por nombre
	Elf32_Shdr* FindSectionByName(FILE* file, const char* name,
		Elf32_Ehdr* ehdr, Elf32_Shdr* shdrs);

	// Cargar información de tabla de símbolos y strings
	status_t LoadSymbolInfo(FILE* file, LoadedLibrary& lib);

	// Cargar tabla de símbolos del binario principal para resolver weak symbols
	status_t LoadMainBinarySymbolTable(FILE* file, const Elf32_Ehdr& ehdr, uint32_t base_address);

	// Register stub symbols for missing functions
	void RegisterStubSymbols();
};

#endif // _DYNAMIC_LINKER_H_
