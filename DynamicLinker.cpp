/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "DynamicLinker.h"
#include "SymbolResolver.h"
#include "AddressSpace.h"
#include "X86_32GuestContext.h"
#include "ElfDynamic.h"
#include "GuestMemoryAllocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global symbol resolver (for now, can be made per-context later)
static SymbolResolver g_symbol_resolver;

DynamicLinker::DynamicLinker(AddressSpace& addressSpace, X86_32GuestContext& context)
	: fAddressSpace(addressSpace)
	, fContext(context)
	, fMainSymtab(nullptr)
	, fMainSymcount(0)
	, fMainStrtab(nullptr)
	, fMainStrtab_size(0)
	, fMainBase(0)
	, fNextLoadAddress(0x40000000)  // Dirección base para librerías: 1GB
{
	printf("[DynamicLinker] Initialized (base load address: 0x%08x)\n", fNextLoadAddress);
}

DynamicLinker::~DynamicLinker()
{
	// Limpiar recursos
	for (auto& pair : fLoadedLibraries) {
		LoadedLibrary& lib = pair.second;
		if (lib.symtab) free(lib.symtab);
		if (lib.strtab) free(lib.strtab);
	}
	fLoadedLibraries.clear();
	fResolvedSymbols.clear();

	// Limpiar símbolos del binario principal
	if (fMainSymtab) free(fMainSymtab);
	if (fMainStrtab) free(fMainStrtab);
}

status_t DynamicLinker::LoadDynamicBinary(const char* path, uint32& entry_point)
{
	if (!path) return B_BAD_VALUE;

	printf("[DynamicLinker] LoadDynamicBinary: %s\n", path);

	FILE* f = fopen(path, "rb");
	if (!f) {
		printf("[DynamicLinker] ERROR: Cannot open %s\n", path);
		return B_ERROR;
	}

	// Leer ELF header
	Elf32_Ehdr ehdr;
	fseek(f, 0, SEEK_SET);
	if (fread(&ehdr, sizeof(Elf32_Ehdr), 1, f) != 1) {
		printf("[DynamicLinker] ERROR: Cannot read ELF header\n");
		fclose(f);
		return B_ERROR;
	}

	// Verificar magia
	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
		printf("[DynamicLinker] ERROR: Not ELF\n");
		fclose(f);
		return B_BAD_VALUE;
	}

	// Verificar i386
	if (ehdr.e_machine != EM_386) {
		printf("[DynamicLinker] ERROR: Not i386 (machine=%d)\n", ehdr.e_machine);
		fclose(f);
		return B_BAD_VALUE;
	}

	// Solo procesar dinámicos
	if (ehdr.e_type != ET_DYN) {
		printf("[DynamicLinker] ERROR: Not ET_DYN (type=%d)\n", ehdr.e_type);
		fclose(f);
		return B_BAD_VALUE;
	}

	printf("[DynamicLinker] entry=0x%08x, phnum=%d\n", ehdr.e_entry, ehdr.e_phnum);

	// Leer program headers
	Elf32_Phdr* phdrs = (Elf32_Phdr*)malloc(ehdr.e_phentsize * ehdr.e_phnum);
	if (!phdrs) {
		fclose(f);
		return B_NO_MEMORY;
	}

	fseek(f, ehdr.e_phoff, SEEK_SET);
	if (fread(phdrs, ehdr.e_phentsize, ehdr.e_phnum, f) != (size_t)ehdr.e_phnum) {
		printf("[DynamicLinker] ERROR: Cannot read phdrs\n");
		free(phdrs);
		fclose(f);
		return B_ERROR;
	}

	// Hallar base (mínimo vaddr de PT_LOAD)
	Elf32_Phdr* dynamic_phdr = nullptr;
	uint32_t load_base = 0xFFFFFFFF;

	for (int i = 0; i < ehdr.e_phnum; i++) {
		if (phdrs[i].p_type == PT_DYNAMIC) {
			dynamic_phdr = &phdrs[i];
		}
		if (phdrs[i].p_type == PT_LOAD && phdrs[i].p_vaddr < load_base) {
			load_base = phdrs[i].p_vaddr;
		}
	}

	if (!dynamic_phdr) {
		printf("[DynamicLinker] ERROR: No DYNAMIC segment\n");
		free(phdrs);
		fclose(f);
		return B_BAD_VALUE;
	}

	// Para ET_DYN, elegir base real de carga (típicamente 0x40000000)
	uint32_t actual_base = 0x40000000;
	printf("[DynamicLinker] load_base=0x%08x → actual=0x%08x\n", load_base, actual_base);

	// Calcular total size necesario e ir registrando mappings
	// IMPORTANTE: Los offsets deben estar dentro del rango [0, 256MB)
	// FIXED: Use global allocator instead of static offset
	GuestMemoryAllocator& allocator = GuestMemoryAllocator::Get();
	uint32_t segment_offset_start = 0;  // Will be set by allocator

	// 1. Preregistrar PT_LOAD mappings
	printf("[DynamicLinker] Registering PT_LOAD mappings...\n");
	
	// First, calculate total size needed
	uint32_t total_size = 0;
	for (int i = 0; i < ehdr.e_phnum; i++) {
		if (phdrs[i].p_type == PT_LOAD) {
			total_size += phdrs[i].p_memsz;
		}
	}
	
	// Allocate space for the entire binary at once
	segment_offset_start = allocator.Allocate(total_size, 0x1000);
	uint32_t guest_offset = segment_offset_start;
	
	for (int i = 0; i < ehdr.e_phnum; i++) {
		if (phdrs[i].p_type != PT_LOAD) continue;

		uint32_t vaddr = phdrs[i].p_vaddr + actual_base;
		uint32_t memsz = phdrs[i].p_memsz;

		printf("[DynamicLinker]   vaddr=0x%08x (0x%x in file), memsz=%u → offset=0x%08x\n",
			vaddr, phdrs[i].p_vaddr, memsz, guest_offset);

		fAddressSpace.RegisterMapping(vaddr, guest_offset, memsz);
		guest_offset += memsz;
	}

	// 2. Cargar todos los PT_LOAD
	printf("[DynamicLinker] Loading PT_LOAD segments...\n");
	uint32_t segment_load_offset = segment_offset_start;

	for (int i = 0; i < ehdr.e_phnum; i++) {
		if (phdrs[i].p_type != PT_LOAD) continue;

		uint32_t vaddr = phdrs[i].p_vaddr + actual_base;
		uint32_t filesz = phdrs[i].p_filesz;
		uint32_t memsz = phdrs[i].p_memsz;
		uint32_t offset = phdrs[i].p_offset;

		printf("[DynamicLinker]   Loading 0x%08x (filesz=%u, memsz=%u)\n", vaddr, filesz, memsz);

		// Cargar parte en archivo
		if (filesz > 0) {
			uint8_t buffer[4096];
			uint32_t remaining = filesz;
			uint32_t dest_addr = vaddr;

			fseek(f, offset, SEEK_SET);
			while (remaining > 0) {
				uint32_t chunk = (remaining > 4096) ? 4096 : remaining;
				if (fread(buffer, 1, chunk, f) != chunk) {
					printf("[DynamicLinker] ERROR: Cannot read segment data\n");
					free(phdrs);
					fclose(f);
					return B_ERROR;
				}

				if (fAddressSpace.Write(dest_addr, buffer, chunk) != B_OK) {
					printf("[DynamicLinker] ERROR: Cannot write to guest memory 0x%08x\n", dest_addr);
					free(phdrs);
					fclose(f);
					return B_ERROR;
				}

				dest_addr += chunk;
				remaining -= chunk;
			}
		}

		// Llenar resto con ceros (BSS)
		if (memsz > filesz) {
			uint32_t bss_addr = vaddr + filesz;
			uint32_t bss_size = memsz - filesz;
			uint8_t zeros[4096] = {0};

			printf("[DynamicLinker]   BSS: 0x%08x + %u\n", bss_addr, bss_size);

			uint32_t remaining = bss_size;
			while (remaining > 0) {
				uint32_t chunk = (remaining > 4096) ? 4096 : remaining;
				if (fAddressSpace.Write(bss_addr, zeros, chunk) != B_OK) {
					printf("[DynamicLinker] ERROR: Cannot zero BSS\n");
					free(phdrs);
					fclose(f);
					return B_ERROR;
				}
				bss_addr += chunk;
				remaining -= chunk;
			}
		}
	}

	printf("[DynamicLinker] PT_LOAD segments loaded OK\n");

	// Update fNextLoadAddress for library loading
	// Libraries will be loaded after the main binary
	fNextLoadAddress = actual_base + (guest_offset - segment_offset_start) + 0x100000;

	// 3. Leer DYNAMIC segment
	printf("[DynamicLinker] Parsing DYNAMIC segment...\n");
	Elf32_Dyn* dyn_data = (Elf32_Dyn*)malloc(dynamic_phdr->p_filesz);
	if (!dyn_data) {
		free(phdrs);
		fclose(f);
		return B_NO_MEMORY;
	}

	fseek(f, dynamic_phdr->p_offset, SEEK_SET);
	if (fread(dyn_data, 1, dynamic_phdr->p_filesz, f) != dynamic_phdr->p_filesz) {
		printf("[DynamicLinker] ERROR: Cannot read DYNAMIC\n");
		free(dyn_data);
		free(phdrs);
		fclose(f);
		return B_ERROR;
	}

	// 4. Parse DYNAMIC
	DynamicInfo dyn_info;
	status_t parse_stat = ParseDynamic(dyn_data, dynamic_phdr->p_filesz, dyn_info, actual_base);
	free(dyn_data);

	if (parse_stat != B_OK) {
		printf("[DynamicLinker] ERROR: ParseDynamic failed\n");
		free(phdrs);
		fclose(f);
		return parse_stat;
	}

	// 5. Cargar tabla de símbolos del binario principal (para weak symbols)
	printf("[DynamicLinker] Loading main binary symbol table...\n");
	status_t sym_stat = LoadMainBinarySymbolTable(f, ehdr, actual_base);
	if (sym_stat != B_OK) {
		printf("[DynamicLinker] WARNING: Could not load main symbol table\n");
	}

	// 6. Cargar librerías requeridas (CRITICAL for ET_DYN execution)
	printf("[DynamicLinker] Loading required libraries...\n");
	status_t lib_stat = LoadRequiredLibraries(dyn_info);
	if (lib_stat != B_OK) {
		printf("[DynamicLinker] WARNING: Some libraries not loaded\n");
	}

	// 6a. Debug: Show loaded symbols
	printf("[DynamicLinker] === LOADED SYMBOLS AFTER LIBRARIES ===\n");
	int symbol_count = 0;
	for (const auto& sym_pair : fResolvedSymbols) {
		symbol_count++;
		if (symbol_count <= 15) {  // Show first 15
			printf("[DynamicLinker]   %s @ 0x%08x\n", sym_pair.first.c_str(), sym_pair.second.address);
		}
	}
	printf("[DynamicLinker] Total loaded symbols: %zu\n", fResolvedSymbols.size());

	// 7. Apply relocations
	printf("[DynamicLinker] Applying relocations...\n");
	status_t reloc_stat = ApplyRelocations(dyn_info, actual_base);
	if (reloc_stat != B_OK) {
		printf("[DynamicLinker] WARNING: Relocations had issues\n");
	}

	// Punto de entrada ajustado con base
	entry_point = ehdr.e_entry + actual_base;

	// Store image base in context for relocation handling
	fContext.SetImageBase(actual_base);

	free(phdrs);
	fclose(f);

	printf("[DynamicLinker] Binary loaded: entry=0x%08x, base=0x%08x\n", entry_point, actual_base);
	return B_OK;
}

status_t DynamicLinker::LoadSharedLibrary(const char* path, uint32& base_address)
{
	if (!path) return B_BAD_VALUE;

	printf("[DynamicLinker] Loading shared library: %s\n", path);

	FILE* f = fopen(path, "rb");
	if (!f) {
		printf("[DynamicLinker] Cannot open library: %s\n", path);
		return B_ERROR;
	}

	// Leer encabezado ELF
	Elf32_Ehdr ehdr;
	fseek(f, 0, SEEK_SET);
	if (fread(&ehdr, sizeof(Elf32_Ehdr), 1, f) != 1) {
		printf("[DynamicLinker] Failed to read ELF header for %s\n", path);
		fclose(f);
		return B_ERROR;
	}

	// Verificar magia ELF
	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
		printf("[DynamicLinker] Not a valid ELF file: %s\n", path);
		fclose(f);
		return B_BAD_VALUE;
	}

	// Crear nueva entrada en librerías cargadas
	LoadedLibrary lib;
	lib.path = path;
	lib.baseAddress = fNextLoadAddress;
	lib.size = 0;
	lib.symtab = nullptr;
	lib.symcount = 0;
	lib.strtab = nullptr;
	lib.strtab_size = 0;

	// Leer segmentos PT_LOAD
	Elf32_Phdr* phdrs = (Elf32_Phdr*)malloc(ehdr.e_phentsize * ehdr.e_phnum);
	if (!phdrs) {
		fclose(f);
		return B_NO_MEMORY;
	}

	fseek(f, ehdr.e_phoff, SEEK_SET);
	if (fread(phdrs, ehdr.e_phentsize, ehdr.e_phnum, f) != (size_t)ehdr.e_phnum) {
		printf("[DynamicLinker] Failed to read program headers\n");
		free(phdrs);
		fclose(f);
		return B_ERROR;
	}

	// Calcular tamaño total
	uint32_t min_addr = 0xFFFFFFFF;
	uint32_t max_addr = 0;

	for (int i = 0; i < ehdr.e_phnum; i++) {
		if (phdrs[i].p_type == PT_LOAD) {
			if (phdrs[i].p_vaddr < min_addr) min_addr = phdrs[i].p_vaddr;
			if (phdrs[i].p_vaddr + phdrs[i].p_memsz > max_addr) {
				max_addr = phdrs[i].p_vaddr + phdrs[i].p_memsz;
			}
		}
	}

	if (max_addr > min_addr) {
		lib.size = max_addr - min_addr;
	}

	printf("[DynamicLinker] Library size: 0x%08x (0x%08x - 0x%08x)\n",
		lib.size, min_addr, max_addr);

	// TODO: Cargar segmentos PT_LOAD en guest memory
	// TODO: Cargar tabla de símbolos

	base_address = lib.baseAddress;
	fNextLoadAddress += (lib.size + 0x1000) & ~0xFFF;  // Alinear a página

	free(phdrs);
	fclose(f);

	// Registrar librería
	fLoadedLibraries[path] = lib;

	printf("[DynamicLinker] Library loaded at 0x%08x\n", lib.baseAddress);
	return B_OK;
}

status_t DynamicLinker::ResolveSymbol(const char* name, ResolvedSymbol& out_symbol)
{
	if (!name) return B_BAD_VALUE;

	auto it = fResolvedSymbols.find(name);
	if (it != fResolvedSymbols.end()) {
		out_symbol = it->second;
		return B_OK;
	}

	printf("[DynamicLinker] Symbol '%s' not resolved yet\n", name);
	return B_NAME_NOT_FOUND;
}

uint32_t DynamicLinker::GetSymbolAddress(const char* name)
{
	if (!name) return 0;

	// First check loaded library symbols
	auto it = fResolvedSymbols.find(name);
	if (it != fResolvedSymbols.end()) {
		return it->second.address;
	}

	// Then check main binary symbols
	if (fMainSymtab && fMainStrtab && fMainSymcount > 0) {
		for (uint32_t i = 0; i < fMainSymcount; i++) {
			if (fMainSymtab[i].st_name >= fMainStrtab_size) continue;
			if (fMainSymtab[i].st_name == 0) continue;
			
			const char* sym_name = &fMainStrtab[fMainSymtab[i].st_name];
			if (strcmp(sym_name, name) == 0) {
				uint32_t addr = fMainBase + fMainSymtab[i].st_value;
				printf("[DynamicLinker] Symbol '%s' found in main binary @ 0x%08x\n", name, addr);
				return addr;
			}
		}
	}

	// printf("[DynamicLinker] Symbol '%s' not found\n", name);
	return 0;
}

void DynamicLinker::PrintLoadedSymbols() const
{
	printf("\n[DynamicLinker] === Loaded Libraries and Symbols ===\n");
	
	for (const auto& pair : fLoadedLibraries) {
		const LoadedLibrary& lib = pair.second;
		printf("\nLibrary: %s\n", lib.path.c_str());
		printf("  Base: 0x%08x, Size: 0x%08x\n", lib.baseAddress, lib.size);
		printf("  Symbols: %u\n", lib.symcount);
	}

	printf("\n=== Resolved Symbols ===\n");
	for (const auto& pair : fResolvedSymbols) {
		printf("  %s @ 0x%08x\n", pair.first.c_str(), pair.second.address);
	}
	printf("\n");
}

// Métodos privados

status_t DynamicLinker::ParseDynamicSegment(FILE* file, Elf32_Ehdr* ehdr,
	Elf32_Phdr** dyn_phdr_out)
{
	if (!file || !ehdr || !dyn_phdr_out) return B_BAD_VALUE;

	printf("[DynamicLinker] ParseDynamicSegment: analyzing DYNAMIC entries\n");

	// Allocate space for program headers
	Elf32_Phdr* phdrs = (Elf32_Phdr*)malloc(ehdr->e_phentsize * ehdr->e_phnum);
	if (!phdrs) return B_NO_MEMORY;

	fseek(file, ehdr->e_phoff, SEEK_SET);
	if (fread(phdrs, ehdr->e_phentsize, ehdr->e_phnum, file) != (size_t)ehdr->e_phnum) {
		printf("[DynamicLinker] Failed to read program headers\n");
		free(phdrs);
		return B_ERROR;
	}

	// Find DYNAMIC segment
	for (int i = 0; i < ehdr->e_phnum; i++) {
		if (phdrs[i].p_type == PT_DYNAMIC) {
			*dyn_phdr_out = &phdrs[i];
			printf("[DynamicLinker] Found DYNAMIC segment at offset 0x%08x, size 0x%08x\n",
				phdrs[i].p_offset, phdrs[i].p_filesz);
			return B_OK;
		}
	}

	free(phdrs);
	printf("[DynamicLinker] No DYNAMIC segment found\n");
	return B_NAME_NOT_FOUND;
}

status_t DynamicLinker::LoadSymbolInfo(FILE* file, LoadedLibrary& lib)
{
	printf("[DynamicLinker] LoadSymbolInfo: loading symbol tables for %s\n", lib.path.c_str());

	// Read ELF header
	Elf32_Ehdr ehdr;
	fseek(file, 0, SEEK_SET);
	if (fread(&ehdr, sizeof(Elf32_Ehdr), 1, file) != 1) {
		printf("[DynamicLinker] Failed to read ELF header\n");
		return B_ERROR;
	}

	// Read section headers
	Elf32_Shdr* shdrs = (Elf32_Shdr*)malloc(ehdr.e_shentsize * ehdr.e_shnum);
	if (!shdrs) return B_NO_MEMORY;

	fseek(file, ehdr.e_shoff, SEEK_SET);
	if (fread(shdrs, ehdr.e_shentsize, ehdr.e_shnum, file) != (size_t)ehdr.e_shnum) {
		printf("[DynamicLinker] Failed to read section headers\n");
		free(shdrs);
		return B_ERROR;
	}

	// Read string table for section names
	Elf32_Shdr& shstrtab_hdr = shdrs[ehdr.e_shstrndx];
	char* shstrtab = (char*)malloc(shstrtab_hdr.sh_size);
	if (!shstrtab) {
		free(shdrs);
		return B_NO_MEMORY;
	}

	fseek(file, shstrtab_hdr.sh_offset, SEEK_SET);
	if (fread(shstrtab, 1, shstrtab_hdr.sh_size, file) != shstrtab_hdr.sh_size) {
		printf("[DynamicLinker] Failed to read section name string table\n");
		free(shstrtab);
		free(shdrs);
		return B_ERROR;
	}

	// Find .dynsym section
	Elf32_Shdr* dynsym_hdr = nullptr;
	Elf32_Shdr* dynstr_hdr = nullptr;

	for (int i = 0; i < ehdr.e_shnum; i++) {
		const char* sec_name = &shstrtab[shdrs[i].sh_name];
		if (strcmp(sec_name, ".dynsym") == 0) {
			dynsym_hdr = &shdrs[i];
			printf("[DynamicLinker] Found .dynsym at offset 0x%08x, size 0x%08x\n",
				shdrs[i].sh_offset, shdrs[i].sh_size);
		}
		if (strcmp(sec_name, ".dynstr") == 0) {
			dynstr_hdr = &shdrs[i];
			printf("[DynamicLinker] Found .dynstr at offset 0x%08x, size 0x%08x\n",
				shdrs[i].sh_offset, shdrs[i].sh_size);
		}
	}

	// Load symbol table
	if (dynsym_hdr) {
		lib.symcount = dynsym_hdr->sh_size / sizeof(Elf32_Sym);
		lib.symtab = (Elf32_Sym*)malloc(dynsym_hdr->sh_size);
		if (!lib.symtab) {
			free(shstrtab);
			free(shdrs);
			return B_NO_MEMORY;
		}

		fseek(file, dynsym_hdr->sh_offset, SEEK_SET);
		if (fread(lib.symtab, 1, dynsym_hdr->sh_size, file) != dynsym_hdr->sh_size) {
			printf("[DynamicLinker] Failed to read symbol table\n");
			free(lib.symtab);
			lib.symtab = nullptr;
			free(shstrtab);
			free(shdrs);
			return B_ERROR;
		}
		printf("[DynamicLinker] Loaded %u symbols\n", lib.symcount);
	}

	// Load string table
	if (dynstr_hdr) {
		lib.strtab_size = dynstr_hdr->sh_size;
		lib.strtab = (char*)malloc(dynstr_hdr->sh_size);
		if (!lib.strtab) {
			free(lib.symtab);
			lib.symtab = nullptr;
			free(shstrtab);
			free(shdrs);
			return B_NO_MEMORY;
		}

		fseek(file, dynstr_hdr->sh_offset, SEEK_SET);
		if (fread(lib.strtab, 1, dynstr_hdr->sh_size, file) != dynstr_hdr->sh_size) {
			printf("[DynamicLinker] Failed to read string table\n");
			free(lib.strtab);
			lib.strtab = nullptr;
			free(lib.symtab);
			lib.symtab = nullptr;
			free(shstrtab);
			free(shdrs);
			return B_ERROR;
		}
		printf("[DynamicLinker] Loaded string table (%u bytes)\n", lib.strtab_size);
	}

	free(shstrtab);
	free(shdrs);
	return B_OK;
}

status_t DynamicLinker::LoadSymbolTable(FILE* file, const LoadedLibrary& lib)
{
	printf("[DynamicLinker] LoadSymbolTable: creating symbol records for %s\n", lib.path.c_str());

	if (!lib.symtab || !lib.strtab) {
		printf("[DynamicLinker] Symbol table or string table not loaded\n");
		return B_BAD_VALUE;
	}

	// Extract symbols and register them
	SymbolResolver resolver;
	Library sym_lib;
	sym_lib.path = lib.path;
	sym_lib.baseAddress = lib.baseAddress;
	sym_lib.size = lib.size;

	for (uint32_t i = 0; i < lib.symcount; i++) {
		Elf32_Sym& sym = lib.symtab[i];
		
		if (sym.st_name == 0) continue;  // Skip unnamed symbols

		const char* sym_name = &lib.strtab[sym.st_name];
		if (!sym_name || sym_name[0] == '\0') continue;

		Symbol s;
		s.name = sym_name;
		s.address = lib.baseAddress + sym.st_value;
		s.size = sym.st_size;
		s.binding = sym.st_info;
		s.type = sym.st_info;
		s.shndx = sym.st_shndx;

		sym_lib.symbols.push_back(s);
	}

	printf("[DynamicLinker] Extracted %zu symbols from %s\n", sym_lib.symbols.size(), lib.path.c_str());
	return B_OK;
}

status_t DynamicLinker::ProcessDynamicRelocations(FILE* file, Elf32_Ehdr* ehdr,
	const LoadedLibrary& lib)
{
	printf("[DynamicLinker] ProcessDynamicRelocations: stub for %s\n", lib.path.c_str());
	// TODO: Implement relocation processing
	return B_OK;
}

Elf32_Shdr* DynamicLinker::FindSectionByName(FILE* file, const char* name,
	Elf32_Ehdr* ehdr, Elf32_Shdr* shdrs)
{
	if (!file || !name || !ehdr || !shdrs) return nullptr;

	// Read section header string table
	Elf32_Shdr& shstrtab_hdr = shdrs[ehdr->e_shstrndx];
	char* shstrtab = (char*)malloc(shstrtab_hdr.sh_size);
	if (!shstrtab) return nullptr;

	fseek(file, shstrtab_hdr.sh_offset, SEEK_SET);
	if (fread(shstrtab, 1, shstrtab_hdr.sh_size, file) != shstrtab_hdr.sh_size) {
		free(shstrtab);
		return nullptr;
	}

	// Find section by name
	for (int i = 0; i < ehdr->e_shnum; i++) {
		const char* sec_name = &shstrtab[shdrs[i].sh_name];
		if (strcmp(sec_name, name) == 0) {
			free(shstrtab);
			return &shdrs[i];
		}
	}

	free(shstrtab);
	return nullptr;
}

status_t DynamicLinker::ProcessRelocations(const char* binary_path)
{
	printf("[DynamicLinker] ProcessRelocations: stub for %s\n", binary_path);
	// TODO: Implement relocation processing
	return B_OK;
}

// Apply relocations from DYNAMIC info to loaded memory
status_t DynamicLinker::ApplyRelocations(const DynamicInfo& dyn_info, uint32_t base_address)
{
	printf("[DynamicLinker] ApplyRelocations: base_address=0x%08x\n", base_address);

	int rel_count = 0;
	int rela_count = 0;
	int skip_count = 0;

	// Process DT_REL entries from guest memory
	if (dyn_info.rel_vaddr && dyn_info.rel_size > 0) {
		printf("[DynamicLinker]   Processing %u bytes of DT_REL @ 0x%08x...\n", dyn_info.rel_size, dyn_info.rel_vaddr);
		uint32_t rel_entries = dyn_info.rel_size / sizeof(Elf32_Rel);
		printf("[DynamicLinker]   Found %u REL entries\n", rel_entries);
		
		for (uint32_t i = 0; i < rel_entries; i++) {
			Elf32_Rel rel;
			status_t read_status = fAddressSpace.Read(dyn_info.rel_vaddr + i * sizeof(Elf32_Rel), 
				&rel, sizeof(Elf32_Rel));
			if (read_status != B_OK) {
				// If we can't read the relocation table from guest memory,
				// it might not be loaded yet (e.g., for the main binary).
				// This is not necessarily a fatal error.
				if (i == 0) {
					printf("[DynamicLinker]   WARNING: Cannot read REL table from 0x%08x, trying alternative\n", 
						dyn_info.rel_vaddr);
				}
				// Skip this entry but continue trying
				continue;
			}

			uint32_t rel_type = ELF32_R_TYPE(rel.r_info);
			uint32_t rel_offset = rel.r_offset;
			uint32_t rel_addr = base_address + rel_offset;

			if (rel_type == R_386_RELATIVE) {
				// Read addend from memory, apply relocation
				uint32_t addend = 0;
				status_t read_status = fAddressSpace.Read(rel_addr, &addend, sizeof(uint32_t));
				if (read_status == B_OK) {
					uint32_t new_value = base_address + addend;
					status_t write_status = fAddressSpace.Write(rel_addr, &new_value, sizeof(uint32_t));
					if (write_status == B_OK) {
						if (i < 10) {  // Log first 10 for debugging
							printf("[DynamicLinker]     REL[%u] R_386_RELATIVE @ 0x%08x: read=0x%08x, calc=0x%08x + 0x%08x = write=0x%08x\n",
								i, rel_addr, addend, base_address, addend, new_value);
						}
						rel_count++;
						
						// Verification: read back and verify
						uint32_t verify = 0;
						if (fAddressSpace.Read(rel_addr, &verify, sizeof(uint32_t)) == B_OK && verify != new_value && i < 5) {
							printf("[DynamicLinker]     REL[%u] VERIFY FAILED: wrote 0x%08x but read 0x%08x\n", i, new_value, verify);
						}
					} else {
						printf("[DynamicLinker]     REL[%u] R_386_RELATIVE @ 0x%08x: WRITE FAILED (status=%d)\n",
							i, rel_addr, write_status);
					}
				} else {
					printf("[DynamicLinker]     REL[%u] R_386_RELATIVE @ 0x%08x: READ FAILED (status=%d)\n",
						i, rel_addr, read_status);
				}
			} else if (rel_type == R_386_GLOB_DAT || rel_type == R_386_JMP_SLOT) {
				// S (symbol address) - need to resolve symbol
				uint32_t sym_index = ELF32_R_SYM(rel.r_info);
				
				if (sym_index > 0 && dyn_info.symtab_vaddr) {
					// Read symbol from symbol table
					Elf32_Sym sym;
					status_t sym_status = fAddressSpace.Read(
						dyn_info.symtab_vaddr + sym_index * sizeof(Elf32_Sym),
						&sym, sizeof(Elf32_Sym));
					
					if (sym_status == B_OK && sym.st_name < dyn_info.strtab_size) {
						// Read symbol name
						char sym_name[256];
						status_t name_status = fAddressSpace.ReadString(
							dyn_info.strtab_vaddr + sym.st_name,
							sym_name, sizeof(sym_name));
						
						if (name_status == B_OK && sym_name[0] != '\0') {
							// Try to resolve symbol
							uint32_t sym_addr = GetSymbolAddress(sym_name);
							
							if (sym_addr == 0) {
								// Symbol not found - use stub address
								printf("[DynamicLinker] Symbol '%s' not found\n", sym_name);
								// Use a stub that won't crash if called
								// Try to write a function pointer that will safely return
								// For now, just point to a safe location in the main binary
								sym_addr = dyn_info.symtab_vaddr ? dyn_info.symtab_vaddr : 0xbffd0000;
								status_t write_status = fAddressSpace.Write(rel_addr, &sym_addr, sizeof(uint32_t));
								if (write_status == B_OK) {
									printf("[DynamicLinker]     REL[%u] R_386_GLOB_DAT @ 0x%08x: %s (stub)=0x%08x\n",
										i, rel_addr, sym_name, sym_addr);
									rel_count++;
								} else {
									printf("[DynamicLinker]     REL[%u] WRITE FAILED @ 0x%08x: %s, status=%d\n",
										i, rel_addr, sym_name, write_status);
									skip_count++;
								}
							} else {
								status_t write_status = fAddressSpace.Write(rel_addr, &sym_addr, sizeof(uint32_t));
								if (write_status == B_OK) {
									const char* type_name = (rel_type == R_386_GLOB_DAT) ? "GLOB_DAT" : "JMP_SLOT";
									printf("[DynamicLinker]     REL[%u] R_386_%s @ 0x%08x: %s=0x%08x\n",
										i, type_name, rel_addr, sym_name, sym_addr);
									rel_count++;
								} else {
									printf("[DynamicLinker]     REL[%u] WRITE FAILED @ 0x%08x: %s (found), status=%d\n",
										i, rel_addr, sym_name, write_status);
								}
							}
						}
					}
				} else {
					printf("[DynamicLinker]     REL[%u] INVALID: sym_index=%u or no symtab_vaddr\n", i, sym_index);
					skip_count++;
				}
			} else if (rel_type != R_386_NONE) {
				skip_count++;
				// Only log first few unknown types to avoid spam
				if (i < 10) {
					printf("[DynamicLinker]     REL[%u] type=%u (offset=0x%x, sym=%u): skipped\n", 
						i, rel_type, rel_offset, ELF32_R_SYM(rel.r_info));
				}
			}
		}
	}

	// Process DT_RELA entries
	if (dyn_info.rela_vaddr && dyn_info.rela_size > 0) {
		printf("[DynamicLinker]   Processing %u bytes of DT_RELA...\n", dyn_info.rela_size);
		uint32_t rela_entries = dyn_info.rela_size / sizeof(Elf32_Rela);
		printf("[DynamicLinker]   Found %u RELA entries\n", rela_entries);
		
		for (uint32_t i = 0; i < rela_entries; i++) {
			Elf32_Rela rela;
			status_t read_status = fAddressSpace.Read(dyn_info.rela_vaddr + i * sizeof(Elf32_Rela),
				&rela, sizeof(Elf32_Rela));
			if (read_status != B_OK) {
				skip_count++;
				continue;
			}

			uint32_t rel_type = ELF32_R_TYPE(rela.r_info);
			uint32_t rel_offset = rela.r_offset;
			uint32_t rel_addr = base_address + rel_offset;
			uint32_t addend = rela.r_addend;

			switch (rel_type) {
				case R_386_NONE:
					// No relocation
					break;

				case R_386_RELATIVE: {
					// Addend is in r_addend field
					uint32_t new_value = base_address + addend;
					if (fAddressSpace.Write(rel_addr, &new_value, sizeof(uint32_t)) == B_OK) {
						printf("[DynamicLinker]     RELA[%u] R_386_RELATIVE @ 0x%08x: 0x%08x + 0x%x = 0x%08x\n",
							i, rel_addr, base_address, addend, new_value);
						rela_count++;
					}
					break;
				}

				case R_386_32:
				case R_386_GLOB_DAT:
				case R_386_JMP_SLOT:
					// Need symbol resolution - skip for now
					printf("[DynamicLinker]     RELA[%u] type=%u: skipped (needs symbol resolution)\n", i, rel_type);
					skip_count++;
					break;

				default:
					printf("[DynamicLinker]     RELA[%u] WARNING: unsupported type %u\n", i, rel_type);
					skip_count++;
			}
		}
	}

	// Process DT_JMPREL entries (PLT relocations - typically R_386_JMP_SLOT)
	if (dyn_info.jmprel_vaddr && dyn_info.jmprel_size > 0) {
		printf("[DynamicLinker]   Processing %u bytes of JMPREL @ 0x%08x...\n", dyn_info.jmprel_size, dyn_info.jmprel_vaddr);
		uint32_t jmprel_entries = dyn_info.jmprel_size / sizeof(Elf32_Rel);
		printf("[DynamicLinker]   Found %u JMPREL entries\n", jmprel_entries);
		

		
		for (uint32_t i = 0; i < jmprel_entries; i++) {
			Elf32_Rel rel;
			status_t read_status = fAddressSpace.Read(dyn_info.jmprel_vaddr + i * sizeof(Elf32_Rel), 
				&rel, sizeof(Elf32_Rel));
			if (read_status != B_OK) {
				skip_count++;
				printf("[DynamicLinker]     JMPREL[%u] ERROR: cannot read from 0x%08x\n", 
					i, dyn_info.jmprel_vaddr + i * sizeof(Elf32_Rel));
				continue;
			}

			uint32_t rel_type = ELF32_R_TYPE(rel.r_info);
			uint32_t rel_offset = rel.r_offset;
			uint32_t rel_addr = base_address + rel_offset;

			if (rel_type == R_386_JMP_SLOT) {
				// S (symbol address) - need to resolve symbol
				uint32_t sym_index = ELF32_R_SYM(rel.r_info);
				
				if (sym_index > 0 && dyn_info.symtab_vaddr) {
					// Read symbol from symbol table
					Elf32_Sym sym;
					status_t sym_status = fAddressSpace.Read(
						dyn_info.symtab_vaddr + sym_index * sizeof(Elf32_Sym),
						&sym, sizeof(Elf32_Sym));
					
					if (sym_status == B_OK && sym.st_name < dyn_info.strtab_size) {
						// Read symbol name
						char sym_name[256];
						status_t name_status = fAddressSpace.ReadString(
							dyn_info.strtab_vaddr + sym.st_name,
							sym_name, sizeof(sym_name));
						
						if (name_status == B_OK && sym_name[0] != '\0') {
							// Try to resolve symbol
							uint32_t sym_addr = GetSymbolAddress(sym_name);
							
							if (sym_addr == 0) {
								// Symbol not found - use stub address
								printf("[DynamicLinker] Symbol '%s' not found in JMPREL\n", sym_name);
								sym_addr = dyn_info.symtab_vaddr ? dyn_info.symtab_vaddr : 0xbffd0000;
								status_t write_status = fAddressSpace.Write(rel_addr, &sym_addr, sizeof(uint32_t));
								if (write_status == B_OK) {
									printf("[DynamicLinker]     JMPREL[%u] R_386_JMP_SLOT @ 0x%08x: %s (stub)=0x%08x\n",
										i, rel_addr, sym_name, sym_addr);
									rel_count++;
								} else {
									printf("[DynamicLinker]     JMPREL[%u] WRITE FAILED @ 0x%08x: %s, status=%d\n",
										i, rel_addr, sym_name, write_status);
									skip_count++;
								}
							} else {
								status_t write_status = fAddressSpace.Write(rel_addr, &sym_addr, sizeof(uint32_t));
								if (write_status == B_OK) {
									printf("[DynamicLinker]     JMPREL[%u] R_386_JMP_SLOT @ 0x%08x: %s=0x%08x\n",
										i, rel_addr, sym_name, sym_addr);
									rel_count++;
								} else {
									printf("[DynamicLinker]     JMPREL[%u] WRITE FAILED @ 0x%08x: %s (found), status=%d\n",
										i, rel_addr, sym_name, write_status);
								}
							}
						}
					}
				} else {
					printf("[DynamicLinker]     JMPREL[%u] INVALID: sym_index=%u or no symtab_vaddr\n", i, sym_index);
					skip_count++;
				}
			} else if (rel_type != R_386_NONE) {
				skip_count++;
				printf("[DynamicLinker]     JMPREL[%u] type=%u: skipped (not JMP_SLOT)\n", i, rel_type);
			}
		}
	}

	printf("[DynamicLinker] ApplyRelocations: complete (REL=%u, RELA=%u, JMPREL=%u, skipped=%u)\n",
		rel_count, rela_count, rel_count, skip_count);
	return B_OK;
}

/* DISABLED - uses obsolete dyn_info.symtab/strtab
// Apply a single relocation entry (DT_REL)
status_t DynamicLinker::ApplySingleRelocation(const Elf32_Rel* rel, uint32_t base_address,
	const DynamicInfo& dyn_info)
{
	if (!rel) {
		return B_BAD_VALUE;
	}

	uint32_t rel_type = ELF32_R_TYPE(rel->r_info);
	uint32_t rel_sym = ELF32_R_SYM(rel->r_info);
	uint32_t rel_offset = rel->r_offset + base_address;

	// Debug output
	printf("[DynamicLinker] Relocation: type=%u, sym=%u, offset=0x%x (0x%x+0x%x)\n",
		rel_type, rel_sym, rel_offset, rel->r_offset, base_address);

	switch (rel_type) {
		case R_386_NONE:
			// No relocation needed
			return B_OK;

		case R_386_RELATIVE: {
			// B + A (where B is base address)
			// For 32-bit: value = base_address + addend
			// The addend is stored at the relocation offset
			
			uint32_t addend = 0;
			status_t status = fAddressSpace.Read<uint32_t>(rel_offset, addend);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot read addend at offset 0x%x\n", rel_offset);
				return status;
			}

			uint32_t new_value = base_address + addend;
			status = fAddressSpace.Write<uint32_t>(rel_offset, new_value);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot write relocation at offset 0x%x\n", rel_offset);
				return status;
			}

			printf("[DynamicLinker]   R_386_RELATIVE: *0x%x = 0x%x + 0x%x = 0x%x\n",
				rel_offset, base_address, addend, new_value);
			return B_OK;
		}

		case R_386_32: {
			// S + A (where S is symbol value)
			// Addend is at relocation offset
			
			uint32_t addend = 0;
			status_t status = fAddressSpace.Read<uint32_t>(rel_offset, addend);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot read addend at offset 0x%x\n", rel_offset);
				return status;
			}

			uint32_t sym_value = 0;
			
			if (rel_sym == 0) {
				// No symbol, treat as R_386_RELATIVE
				sym_value = base_address;
			} else if (rel_sym < dyn_info.symtab_size) {
				// Look up symbol
				const Elf32_Sym* sym = &dyn_info.symtab[rel_sym];
				const char* sym_name = &dyn_info.strtab[sym->st_name];
				
				// Try to resolve symbol
				sym_value = ResolveSymbolAddress(sym_name);
				if (sym_value == 0) {
					printf("[DynamicLinker] Warning: Cannot resolve symbol '%s' for R_386_32\n", sym_name);
					// Some symbols might be undefined but still valid
					// Continue with value 0
				}

				printf("[DynamicLinker]   Symbol: %s = 0x%x\n", sym_name, sym_value);
			}

			uint32_t new_value = sym_value + addend;
			status = fAddressSpace.Write<uint32_t>(rel_offset, new_value);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot write relocation at offset 0x%x\n", rel_offset);
				return status;
			}

			printf("[DynamicLinker]   R_386_32: *0x%x = 0x%x + 0x%x = 0x%x\n",
				rel_offset, sym_value, addend, new_value);
			return B_OK;
		}

		case R_386_PC32: {
			// S + A - P (PC-relative)
			// P = relocation offset
			
			if (rel_sym == 0 || rel_sym >= dyn_info.symtab_size) {
				printf("[DynamicLinker] Invalid symbol %u for R_386_PC32\n", rel_sym);
				return B_BAD_VALUE;
			}

			const Elf32_Sym* sym = &dyn_info.symtab[rel_sym];
			const char* sym_name = &dyn_info.strtab[sym->st_name];
			
			uint32_t sym_value = ResolveSymbolAddress(sym_name);
			if (sym_value == 0) {
				printf("[DynamicLinker] Warning: Cannot resolve symbol '%s' for R_386_PC32\n", sym_name);
			}

			uint32_t addend = 0;
			status_t status = fAddressSpace.Read<uint32_t>(rel_offset, addend);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot read addend at offset 0x%x\n", rel_offset);
				return status;
			}

			uint32_t new_value = sym_value + addend - rel_offset;
			status = fAddressSpace.Write<uint32_t>(rel_offset, new_value);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot write relocation at offset 0x%x\n", rel_offset);
				return status;
			}

			printf("[DynamicLinker]   R_386_PC32: *0x%x = %s(0x%x) + 0x%x - 0x%x = 0x%x\n",
				rel_offset, sym_name, sym_value, addend, rel_offset, new_value);
			return B_OK;
		}

		case R_386_GLOB_DAT:
		case R_386_JMP_SLOT: {
			// S (symbol value directly)
			
			uint32_t symtab_entries = dyn_info.symtab_size > 0 ? dyn_info.symtab_size / sizeof(Elf32_Sym) : 0;
			if (rel_sym == 0 || rel_sym >= symtab_entries) {
				printf("[DynamicLinker] Invalid symbol %u for relocation type %u\n", rel_sym, rel_type);
				return B_BAD_VALUE;
			}

			// Read symbol from guest memory
			Elf32_Sym sym;
			status_t sym_status = fAddressSpace.Read(dyn_info.symtab_vaddr + rel_sym * sizeof(Elf32_Sym),
				&sym, sizeof(Elf32_Sym));
			if (sym_status != B_OK) {
				printf("[DynamicLinker] Cannot read symbol %u\n", rel_sym);
				return sym_status;
			}
			
			// Read symbol name from string table
			char sym_name[256];
			status_t name_status = fAddressSpace.ReadString(dyn_info.strtab_vaddr + sym.st_name,
				sym_name, sizeof(sym_name));
			if (name_status != B_OK) {
				printf("[DynamicLinker] Cannot read symbol name at offset %u\n", sym.st_name);
				strcpy(sym_name, "???");
			}
			
			uint32_t sym_value = ResolveSymbolAddress(sym_name);
			if (sym_value == 0) {
				printf("[DynamicLinker] Warning: Cannot resolve symbol '%s' for relocation %u\n",
					sym_name, rel_type);
			}

			status_t status = fAddressSpace.Write<uint32_t>(rel_offset, sym_value);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot write relocation at offset 0x%x\n", rel_offset);
				return status;
			}

			const char* type_name = (rel_type == R_386_GLOB_DAT) ? "R_386_GLOB_DAT" : "R_386_JMP_SLOT";
			printf("[DynamicLinker]   %s: *0x%x = %s(0x%x)\n",
				type_name, rel_offset, sym_name, sym_value);
			return B_OK;
		}

		case R_386_GOTOFF:
		case R_386_GOTPC:
		case R_386_GOT32:
		case R_386_PLT32:
		case R_386_COPY:
			// These are complex relocations that require GOT/PLT handling
			// For now, just log them as unimplemented
			printf("[DynamicLinker] Relocation type %u not yet implemented (offset 0x%x)\n",
				rel_type, rel_offset);
			return B_OK;  // Don't fail, just skip

		default:
			printf("[DynamicLinker] Unknown relocation type: %u\n", rel_type);
			return B_BAD_VALUE;
	}
}
*/

/* DISABLED - uses obsolete dyn_info.symtab/strtab
// Apply a single relocation entry with addend (DT_RELA)
status_t DynamicLinker::ApplySingleRela(const Elf32_Rela* rela, uint32_t base_address,
	const DynamicInfo& dyn_info)
{
	if (!rela) {
		return B_BAD_VALUE;
	}

	uint32_t rel_type = ELF32_R_TYPE(rela->r_info);
	uint32_t rel_sym = ELF32_R_SYM(rela->r_info);
	uint32_t rel_offset = rela->r_offset + base_address;
	int32_t addend = rela->r_addend;

	printf("[DynamicLinker] Rela: type=%u, sym=%u, offset=0x%x, addend=0x%x\n",
		rel_type, rel_sym, rel_offset, addend);

	switch (rel_type) {
		case R_386_NONE:
			return B_OK;

		case R_386_RELATIVE: {
			// B + A
			uint32_t new_value = base_address + addend;
			status_t status = fAddressSpace.Write<uint32_t>(rel_offset, new_value);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot write relocation at offset 0x%x\n", rel_offset);
				return status;
			}

			printf("[DynamicLinker]   R_386_RELATIVE: *0x%x = 0x%x + 0x%x = 0x%x\n",
				rel_offset, base_address, addend, new_value);
			return B_OK;
		}

		case R_386_32: {
			// S + A
			uint32_t sym_value = 0;
			
			if (rel_sym > 0 && rel_sym < dyn_info.symtab_size) {
				const Elf32_Sym* sym = &dyn_info.symtab[rel_sym];
				const char* sym_name = &dyn_info.strtab[sym->st_name];
				sym_value = ResolveSymbolAddress(sym_name);
				
				printf("[DynamicLinker]   Symbol: %s = 0x%x\n", sym_name, sym_value);
			}

			uint32_t new_value = sym_value + addend;
			status_t status = fAddressSpace.Write<uint32_t>(rel_offset, new_value);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot write relocation at offset 0x%x\n", rel_offset);
				return status;
			}

			printf("[DynamicLinker]   R_386_32: *0x%x = 0x%x + 0x%x = 0x%x\n",
				rel_offset, sym_value, addend, new_value);
			return B_OK;
		}

		case R_386_PC32: {
			// S + A - P
			uint32_t sym_value = 0;
			
			if (rel_sym > 0 && rel_sym < dyn_info.symtab_size) {
				const Elf32_Sym* sym = &dyn_info.symtab[rel_sym];
				const char* sym_name = &dyn_info.strtab[sym->st_name];
				sym_value = ResolveSymbolAddress(sym_name);
			}

			uint32_t new_value = sym_value + addend - rel_offset;
			status_t status = fAddressSpace.Write<uint32_t>(rel_offset, new_value);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot write relocation at offset 0x%x\n", rel_offset);
				return status;
			}

			printf("[DynamicLinker]   R_386_PC32: *0x%x = 0x%x + 0x%x - 0x%x = 0x%x\n",
				rel_offset, sym_value, addend, rel_offset, new_value);
			return B_OK;
		}

		case R_386_GLOB_DAT:
		case R_386_JMP_SLOT: {
			// S (addend ignored)
			if (rel_sym == 0 || rel_sym >= dyn_info.symtab_size) {
				printf("[DynamicLinker] Invalid symbol %u for relocation type %u\n", rel_sym, rel_type);
				return B_BAD_VALUE;
			}

			const Elf32_Sym* sym = &dyn_info.symtab[rel_sym];
			const char* sym_name = &dyn_info.strtab[sym->st_name];
			uint32_t sym_value = ResolveSymbolAddress(sym_name);

			status_t status = fAddressSpace.Write<uint32_t>(rel_offset, sym_value);
			if (status != B_OK) {
				printf("[DynamicLinker] Cannot write relocation at offset 0x%x\n", rel_offset);
				return status;
			}

			const char* type_name = (rel_type == R_386_GLOB_DAT) ? "R_386_GLOB_DAT" : "R_386_JMP_SLOT";
			printf("[DynamicLinker]   %s: *0x%x = 0x%x\n", type_name, rel_offset, sym_value);
			return B_OK;
		}

		default:
			printf("[DynamicLinker] Unknown rela type: %u\n", rel_type);
			return B_OK;  // Don't fail, just skip
	}
}
*/

// Parse DYNAMIC segment and extract dynamic information
status_t DynamicLinker::ParseDynamic(const void* dynamicData, uint32_t dynamicSize,
	DynamicInfo& outInfo, uint32_t base_address)
{
	if (!dynamicData || dynamicSize == 0) {
		printf("[DynamicLinker] ParseDynamic: invalid parameters\n");
		return B_BAD_VALUE;
	}

	printf("[DynamicLinker] ParseDynamic: analyzing DYNAMIC segment (size=0x%x)\n", dynamicSize);

	const Elf32_Dyn* dyn_array = (const Elf32_Dyn*)dynamicData;
	uint32_t dyn_count = dynamicSize / sizeof(Elf32_Dyn);

	// Initialize output structure
	memset(&outInfo, 0, sizeof(DynamicInfo));
	outInfo.dynamic = dyn_array;

	// Parse DYNAMIC entries
	for (uint32_t i = 0; i < dyn_count; i++) {
		const Elf32_Dyn& dyn = dyn_array[i];

		if (dyn.d_tag == DT_NULL) {
			printf("[DynamicLinker] Found DT_NULL, end of DYNAMIC array\n");
			break;
		}

		switch (dyn.d_tag) {
			case DT_NEEDED:
				printf("[DynamicLinker] DT_NEEDED @ index %u (strtab offset=0x%x)\n", i, dyn.d_un.d_val);
				break;

			case DT_SYMTAB:
				outInfo.symtab_vaddr = base_address + dyn.d_un.d_ptr;
				printf("[DynamicLinker] DT_SYMTAB @ 0x%08x (adjusted=0x%08x)\n", 
					dyn.d_un.d_ptr, outInfo.symtab_vaddr);
				break;

			case DT_STRTAB:
				outInfo.strtab_vaddr = base_address + dyn.d_un.d_ptr;
				printf("[DynamicLinker] DT_STRTAB @ 0x%08x (adjusted=0x%08x)\n",
					dyn.d_un.d_ptr, outInfo.strtab_vaddr);
				break;

			case DT_STRSZ:
				outInfo.strtab_size = dyn.d_un.d_val;
				printf("[DynamicLinker] DT_STRSZ = 0x%x (%u bytes)\n", dyn.d_un.d_val, dyn.d_un.d_val);
				break;

			case DT_SYMENT:
				printf("[DynamicLinker] DT_SYMENT = %u bytes\n", dyn.d_un.d_val);
				break;

			case DT_HASH: {
				// Hash table layout: uint32 nbucket, uint32 nchain, uint32 buckets[nbucket], uint32 chains[nchain]
				// nchain = number of symbols
				uint32_t hash_vaddr = base_address + dyn.d_un.d_ptr;
				printf("[DynamicLinker] DT_HASH @ 0x%08x (guest_vaddr=0x%08x)\n", dyn.d_un.d_ptr, hash_vaddr);
				
				// Read hash table header: nbucket (offset 0) and nchain (offset 4)
				uint32_t hash_header[2];
				if (fAddressSpace.Read(hash_vaddr, hash_header, sizeof(hash_header)) == B_OK) {
					uint32_t nbucket = hash_header[0];
					uint32_t nchain = hash_header[1];
					printf("[DynamicLinker]   nbucket=%u, nchain=%u (symtab_size=0x%x bytes)\n", 
						nbucket, nchain, nchain * sizeof(Elf32_Sym));
					outInfo.symtab_size = nchain * sizeof(Elf32_Sym);
				}
				break;
			}

			case DT_REL:
				outInfo.rel_vaddr = base_address + dyn.d_un.d_ptr;
				outInfo.has_rel = true;
				printf("[DynamicLinker] DT_REL @ 0x%08x (adjusted=0x%08x)\n", 
					dyn.d_un.d_ptr, outInfo.rel_vaddr);
				break;

			case DT_RELSZ:
				outInfo.rel_size = dyn.d_un.d_val;
				printf("[DynamicLinker] DT_RELSZ = 0x%x bytes\n", dyn.d_un.d_val);
				break;

			case DT_RELENT:
				outInfo.rel_entry_size = dyn.d_un.d_val;
				printf("[DynamicLinker] DT_RELENT = %u bytes\n", dyn.d_un.d_val);
				break;

			case DT_RELA:
				outInfo.rela_vaddr = base_address + dyn.d_un.d_ptr;
				outInfo.has_rela = true;
				printf("[DynamicLinker] DT_RELA @ 0x%08x (adjusted=0x%08x)\n",
					dyn.d_un.d_ptr, outInfo.rela_vaddr);
				break;

			case DT_RELASZ:
				outInfo.rela_size = dyn.d_un.d_val;
				printf("[DynamicLinker] DT_RELASZ = 0x%x bytes\n", dyn.d_un.d_val);
				break;

			case DT_RELAENT:
				outInfo.rela_entry_size = dyn.d_un.d_val;
				printf("[DynamicLinker] DT_RELAENT = %u bytes\n", dyn.d_un.d_val);
				break;

			case DT_INIT:
				outInfo.init = base_address + dyn.d_un.d_ptr;
				printf("[DynamicLinker] DT_INIT @ 0x%08x (adjusted=0x%08x)\n", 
					dyn.d_un.d_ptr, outInfo.init);
				break;

			case DT_FINI:
				outInfo.fini = base_address + dyn.d_un.d_ptr;
				printf("[DynamicLinker] DT_FINI @ 0x%08x (adjusted=0x%08x)\n",
					dyn.d_un.d_ptr, outInfo.fini);
				break;

			case DT_PLTGOT:
				outInfo.pltgot = base_address + dyn.d_un.d_ptr;
				printf("[DynamicLinker] DT_PLTGOT @ 0x%08x (adjusted=0x%08x)\n",
					dyn.d_un.d_ptr, outInfo.pltgot);
				break;

			case DT_JMPREL:
				outInfo.jmprel_vaddr = base_address + dyn.d_un.d_ptr;
				printf("[DynamicLinker] DT_JMPREL @ 0x%08x (adjusted=0x%08x)\n", 
					dyn.d_un.d_ptr, outInfo.jmprel_vaddr);
				break;

			case DT_PLTRELSZ:
				outInfo.jmprel_size = dyn.d_un.d_val;
				printf("[DynamicLinker] DT_PLTRELSZ = 0x%x bytes\n", dyn.d_un.d_val);
				break;

			case DT_FLAGS:
				outInfo.flags = dyn.d_un.d_val;
				printf("[DynamicLinker] DT_FLAGS = 0x%x\n", dyn.d_un.d_val);
				if (outInfo.flags & DF_BIND_NOW) {
					outInfo.bind_now = true;
					printf("[DynamicLinker]   - DF_BIND_NOW set\n");
				}
				break;

			case DT_INIT_ARRAY:
				outInfo.init_array_vaddr = base_address + dyn.d_un.d_ptr;
				printf("[DynamicLinker] DT_INIT_ARRAY @ 0x%08x (adjusted=0x%08x)\n",
					dyn.d_un.d_ptr, outInfo.init_array_vaddr);
				break;

			case DT_INIT_ARRAYSZ:
				outInfo.init_array_size = dyn.d_un.d_val;
				printf("[DynamicLinker] DT_INIT_ARRAYSZ = %u bytes\n", dyn.d_un.d_val);
				break;

			case DT_FINI_ARRAY:
				outInfo.fini_array_vaddr = base_address + dyn.d_un.d_ptr;
				printf("[DynamicLinker] DT_FINI_ARRAY @ 0x%08x (adjusted=0x%08x)\n",
					dyn.d_un.d_ptr, outInfo.fini_array_vaddr);
				break;

			case DT_FINI_ARRAYSZ:
				outInfo.fini_array_size = dyn.d_un.d_val;
				printf("[DynamicLinker] DT_FINI_ARRAYSZ = %u bytes\n", dyn.d_un.d_val);
				break;

			default:
				// Silently ignore unknown tags
				break;
		}
	}

	printf("[DynamicLinker] ParseDynamic: complete\n");
	printf("[DynamicLinker]   SYMTAB: %s, STRTAB: %s, REL: %s, RELA: %s\n",
		outInfo.symtab_vaddr ? "yes" : "no",
		outInfo.strtab_vaddr ? "yes" : "no",
		outInfo.rel_vaddr ? "yes" : "no",
		outInfo.rela_vaddr ? "yes" : "no");

	return B_OK;
}

// Load symbols from parsed DYNAMIC segment info
/* DISABLED - uses obsolete dyn_info.symtab/strtab
status_t DynamicLinker::LoadSymbolsFromDynamic(const DynamicInfo& dyn_info,
	const char* library_path, uint32_t base_address)
{
	if (!library_path || !dyn_info.symtab_vaddr || !dyn_info.strtab_vaddr) {
		printf("[DynamicLinker] LoadSymbolsFromDynamic: missing required data\n");
		return B_BAD_VALUE;
	}

	printf("[DynamicLinker] LoadSymbolsFromDynamic: %s (base=0x%08x)\n",
		library_path, base_address);

	// Calculate symbol count if not provided
	uint32_t symcount = dyn_info.symtab_size;
	if (symcount == 0) {
		// Estimate from first symbol or assume reasonable default
		printf("[DynamicLinker]   WARNING: symtab_size not set, estimating...\n");
		symcount = 100;  // Conservative estimate
	}

	printf("[DynamicLinker]   Loading %u symbols from symbol table\n", symcount);
	printf("[DynamicLinker]   String table size: %u bytes\n", dyn_info.strtab_size);

	// Create Library structure with symbols from DYNAMIC
	Library lib;
	lib.path = library_path;
	lib.baseAddress = base_address;
	lib.size = 0;  // Set by caller if known

	// Extract symbols from ELF symbol table
	for (uint32_t i = 0; i < symcount; i++) {
		const Elf32_Sym& sym = dyn_info.symtab[i];

		// Skip unnamed or invalid symbols
		if (sym.st_name == 0) continue;
		if (sym.st_name >= dyn_info.strtab_size) {
			printf("[DynamicLinker]   SKIP: Symbol %u has invalid string offset\n", i);
			continue;
		}

		const char* sym_name = &dyn_info.strtab[sym.st_name];
		if (!sym_name || sym_name[0] == '\0') continue;

		// Create Symbol entry
		Symbol symbol;
		symbol.name = sym_name;
		symbol.address = base_address + sym.st_value;
		symbol.size = sym.st_size;
		symbol.binding = sym.st_info;
		symbol.type = sym.st_info;
		symbol.shndx = sym.st_shndx;

		lib.symbols.push_back(symbol);

		uint8_t type = ELF32_ST_TYPE(sym.st_info);
		if (type == STT_FUNC) {
			printf("[DynamicLinker]   [FUNC] %s @ 0x%08x\n", sym_name, symbol.address);
		}
	}

	printf("[DynamicLinker] Loaded %zu symbols total\n", lib.symbols.size());

	// Register library with symbol resolver
	status_t status = g_symbol_resolver.RegisterLibrary(lib);
	if (status != B_OK) {
		printf("[DynamicLinker] Failed to register library with resolver\n");
		return status;
	}

	return B_OK;
}
*/

// Resolve a symbol from all loaded libraries
uint32_t DynamicLinker::ResolveSymbolAddress(const char* symbol_name)
{
	if (!symbol_name) return 0;

	printf("[DynamicLinker] ResolveSymbolAddress: looking for '%s'\n", symbol_name);

	// First check local symbol table
	auto it = fResolvedSymbols.find(symbol_name);
	if (it != fResolvedSymbols.end()) {
		uint32_t addr = it->second.address;
		printf("[DynamicLinker]   RESOLVED (local): %s @ 0x%08x\n", symbol_name, addr);
		return addr;
	}

	// Then query global symbol resolver
	uint32_t addr = g_symbol_resolver.ResolveSymbol(symbol_name);

	if (addr != 0) {
		printf("[DynamicLinker]   RESOLVED (global): %s @ 0x%08x\n", symbol_name, addr);
		return addr;
	}

	// Symbol not found - try to create a stub address
	// For now, allocate a placeholder in high memory that won't be used
	// This prevents crashes from null pointer dereferences
	static uint32_t stub_address = 0x50000000;  // High memory for stubs
	printf("[DynamicLinker]   NOT FOUND: %s (using stub @ 0x%08x)\n", symbol_name, stub_address);
	uint32_t result = stub_address;
	stub_address += 16;  // Each stub gets 16 bytes
	return result;
}

// Extract library names from DT_NEEDED entries
status_t DynamicLinker::ExtractNeededLibraries(const DynamicInfo& dyn_info,
	std::vector<std::string>& out_libraries)
{
	if (!dyn_info.dynamic || !dyn_info.strtab_vaddr) {
		printf("[DynamicLinker] ExtractNeededLibraries: missing DYNAMIC or string table\n");
		return B_BAD_VALUE;
	}

	printf("[DynamicLinker] ExtractNeededLibraries: scanning for DT_NEEDED entries\n");

	out_libraries.clear();
	int needed_count = 0;

	// Iterate through DYNAMIC array looking for DT_NEEDED entries
	const Elf32_Dyn* dyn = dyn_info.dynamic;
	for (uint32_t i = 0; dyn[i].d_tag != DT_NULL; i++) {
		if (dyn[i].d_tag == DT_NEEDED) {
			uint32_t str_offset = dyn[i].d_un.d_val;

			// Bounds check
			if (str_offset >= dyn_info.strtab_size) {
				printf("[DynamicLinker]   SKIP: Invalid string offset 0x%x\n", str_offset);
				continue;
			}

			// Read library name from guest memory
			char lib_name_buffer[256];
			status_t read_status = fAddressSpace.ReadString(dyn_info.strtab_vaddr + str_offset,
				lib_name_buffer, sizeof(lib_name_buffer));
			if (read_status != B_OK || lib_name_buffer[0] == '\0') {
				printf("[DynamicLinker]   SKIP: Cannot read lib name at 0x%08x\n",
					dyn_info.strtab_vaddr + str_offset);
				continue;
			}

			printf("[DynamicLinker]   NEEDED: %s\n", lib_name_buffer);
			out_libraries.push_back(lib_name_buffer);
			needed_count++;
		}
	}

	printf("[DynamicLinker] Found %d required libraries\n", needed_count);
	return B_OK;
}

// Find library file in sysroot or system paths
std::string DynamicLinker::FindLibraryFile(const char* lib_name)
{
	if (!lib_name) return "";

	printf("[DynamicLinker] FindLibraryFile: searching for '%s'\n", lib_name);

	// Search paths - try each until found
	const char* search_paths[] = {
		"./sysroot/haiku32/lib",                  // Local sysroot (Haiku uses /lib)
		"./sysroot/haiku32/system/lib",           // Alternative sysroot path
		"/boot/system/lib",                       // Host system lib
		"/boot/home/src/HaikuSoftware/userlandvm_repo/sysroot/haiku32/lib",
		"/boot/home/src/HaikuSoftware/userlandvm_repo/sysroot/haiku32/system/lib",
		"./lib",                                  // Current directory
		"/boot/lib",                              // Host Haiku default
		NULL
	};

	for (int i = 0; search_paths[i] != NULL; i++) {
		// Try exact name first
		std::string full_path = std::string(search_paths[i]) + "/" + lib_name;
		FILE* f = fopen(full_path.c_str(), "rb");
		if (f) {
			fclose(f);
			printf("[DynamicLinker]   FOUND: %s\n", full_path.c_str());
			return full_path;
		}

		// Try with .so.0 suffix (for libc.so → libc.so.0)
		if (strcmp(lib_name + strlen(lib_name) - 3, ".so") == 0) {
			std::string alt_path = full_path + ".0";
			f = fopen(alt_path.c_str(), "rb");
			if (f) {
				fclose(f);
				printf("[DynamicLinker]   FOUND: %s\n", alt_path.c_str());
				return alt_path;
			}
		}
	}

	printf("[DynamicLinker]   NOT FOUND: '%s' in any search path\n", lib_name);
	return "";
}

// Load a single library file
status_t DynamicLinker::LoadLibrary(const char* lib_path, const char* lib_name)
{
	if (!lib_path || !lib_name) {
		printf("[DynamicLinker] LoadLibrary: invalid parameters\n");
		return B_BAD_VALUE;
	}

	printf("[DynamicLinker] LoadLibrary: %s (%s)\n", lib_name, lib_path);

	// Check if already loaded
	if (fLoadedLibraries.find(lib_path) != fLoadedLibraries.end()) {
		printf("[DynamicLinker]   SKIP: Already loaded\n");
		return B_OK;
	}

	// Open library file
	FILE* f = fopen(lib_path, "rb");
	if (!f) {
		printf("[DynamicLinker]   ERROR: Cannot open file\n");
		return B_ERROR;
	}

	// Read ELF header
	Elf32_Ehdr ehdr;
	fseek(f, 0, SEEK_SET);
	if (fread(&ehdr, sizeof(Elf32_Ehdr), 1, f) != 1) {
		printf("[DynamicLinker]   ERROR: Cannot read ELF header\n");
		fclose(f);
		return B_ERROR;
	}

	// Verify ELF magic
	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
		printf("[DynamicLinker]   ERROR: Not a valid ELF file\n");
		fclose(f);
		return B_BAD_VALUE;
	}

	// Verify i386 architecture
	if (ehdr.e_machine != EM_386) {
		printf("[DynamicLinker]   ERROR: Not an i386 ELF file\n");
		fclose(f);
		return B_BAD_VALUE;
	}

	printf("[DynamicLinker]   ELF valid: %u program headers\n", ehdr.e_phnum);

	// Read program headers
	Elf32_Phdr* phdrs = (Elf32_Phdr*)malloc(ehdr.e_phentsize * ehdr.e_phnum);
	if (!phdrs) {
		fclose(f);
		return B_NO_MEMORY;
	}

	fseek(f, ehdr.e_phoff, SEEK_SET);
	if (fread(phdrs, ehdr.e_phentsize, ehdr.e_phnum, f) != (size_t)ehdr.e_phnum) {
		printf("[DynamicLinker]   ERROR: Cannot read program headers\n");
		free(phdrs);
		fclose(f);
		return B_ERROR;
	}

	// Find DYNAMIC segment and calculate library size
	Elf32_Phdr* dynamic_phdr = NULL;
	uint32_t min_vaddr = 0xFFFFFFFF;
	uint32_t max_vaddr = 0;

	for (int i = 0; i < ehdr.e_phnum; i++) {
		if (phdrs[i].p_type == PT_DYNAMIC) {
			dynamic_phdr = &phdrs[i];
			printf("[DynamicLinker]   DYNAMIC segment found\n");
		}
		if (phdrs[i].p_type == PT_LOAD) {
			if (phdrs[i].p_vaddr < min_vaddr) {
				min_vaddr = phdrs[i].p_vaddr;
			}
			uint32_t end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
			if (end > max_vaddr) {
				max_vaddr = end;
			}
		}
	}

	if (!dynamic_phdr) {
		printf("[DynamicLinker]   WARNING: No DYNAMIC segment in library\n");
		free(phdrs);
		fclose(f);
		return B_BAD_VALUE;
	}

	uint32_t lib_size = (max_vaddr > min_vaddr) ? (max_vaddr - min_vaddr) : 0x10000;
	uint32_t lib_base = fNextLoadAddress;

	printf("[DynamicLinker]   Library size: 0x%x, base: 0x%08x\n", lib_size, lib_base);

	// Register PT_LOAD mappings and load segments
	printf("[DynamicLinker]   Registering PT_LOAD mappings...\n");
	// FIXED: Use global allocator for libraries as well
	GuestMemoryAllocator& allocator = GuestMemoryAllocator::Get();
	uint32_t lib_offset = allocator.Allocate(lib_size, 0x1000);

	for (int i = 0; i < ehdr.e_phnum; i++) {
		if (phdrs[i].p_type != PT_LOAD) continue;

		uint32_t vaddr = phdrs[i].p_vaddr + lib_base;
		uint32_t memsz = phdrs[i].p_memsz;

		printf("[DynamicLinker]     vaddr=0x%08x, memsz=%u → offset=0x%08x\n",
			vaddr, memsz, lib_offset);

		fAddressSpace.RegisterMapping(vaddr, lib_offset, memsz);
		lib_offset += memsz;
	}

	printf("[DynamicLinker]   Loading PT_LOAD segments...\n");

	for (int i = 0; i < ehdr.e_phnum; i++) {
		if (phdrs[i].p_type != PT_LOAD) continue;

		uint32_t vaddr = phdrs[i].p_vaddr + lib_base;
		uint32_t filesz = phdrs[i].p_filesz;
		uint32_t memsz = phdrs[i].p_memsz;
		uint32_t offset = phdrs[i].p_offset;

		// Load file data
		if (filesz > 0) {
			uint8_t buffer[4096];
			uint32_t remaining = filesz;
			uint32_t dest_addr = vaddr;

			fseek(f, offset, SEEK_SET);
			while (remaining > 0) {
				uint32_t chunk = (remaining > 4096) ? 4096 : remaining;
				if (fread(buffer, 1, chunk, f) != chunk) {
					printf("[DynamicLinker]   ERROR: Cannot read segment\n");
					free(phdrs);
					fclose(f);
					return B_ERROR;
				}

				if (fAddressSpace.Write(dest_addr, buffer, chunk) != B_OK) {
					printf("[DynamicLinker]   ERROR: Cannot write segment 0x%08x\n", dest_addr);
					free(phdrs);
					fclose(f);
					return B_ERROR;
				}

				dest_addr += chunk;
				remaining -= chunk;
			}
		}

		// Zero-fill BSS
		if (memsz > filesz) {
			uint32_t bss_addr = vaddr + filesz;
			uint32_t bss_size = memsz - filesz;
			uint8_t zeros[4096] = {0};

			uint32_t remaining = bss_size;
			while (remaining > 0) {
				uint32_t chunk = (remaining > 4096) ? 4096 : remaining;
				if (fAddressSpace.Write(bss_addr, zeros, chunk) != B_OK) {
					free(phdrs);
					fclose(f);
					return B_ERROR;
				}
				bss_addr += chunk;
				remaining -= chunk;
			}
		}
	}

	printf("[DynamicLinker]   PT_LOAD segments loaded\n");

	// Read DYNAMIC segment
	Elf32_Dyn* dyn_data = (Elf32_Dyn*)malloc(dynamic_phdr->p_filesz);
	if (!dyn_data) {
		free(phdrs);
		fclose(f);
		return B_NO_MEMORY;
	}

	fseek(f, dynamic_phdr->p_offset, SEEK_SET);
	if (fread(dyn_data, 1, dynamic_phdr->p_filesz, f) != dynamic_phdr->p_filesz) {
		printf("[DynamicLinker]   ERROR: Cannot read DYNAMIC segment\n");
		free(dyn_data);
		free(phdrs);
		fclose(f);
		return B_ERROR;
	}

	// Parse DYNAMIC segment
	DynamicInfo lib_dyn;
	status_t parse_status = ParseDynamic(dyn_data, dynamic_phdr->p_filesz, lib_dyn, lib_base);
	
	free(dyn_data);
	free(phdrs);
	fclose(f);

	if (parse_status != B_OK) {
		printf("[DynamicLinker]   ERROR: Failed to parse DYNAMIC segment\n");
		return parse_status;
	}

	// Load symbol table from section headers if available
	printf("[DynamicLinker]   Loading symbols from %s...\n", lib_name);
	printf("[DynamicLinker]   symtab_vaddr=0x%08x, strtab_vaddr=0x%08x, strtab_size=%u\n",
		lib_dyn.symtab_vaddr, lib_dyn.strtab_vaddr, lib_dyn.strtab_size);
	
	// Register library first
	LoadedLibrary lib;
	lib.path = lib_path;
	lib.baseAddress = lib_base;
	lib.size = lib_size;
	lib.symtab = NULL;
	lib.symcount = 0;
	lib.strtab = NULL;
	lib.strtab_size = 0;

	// Try to load symbols from .dynsym section
	if (lib_dyn.symtab_vaddr && lib_dyn.strtab_vaddr && lib_dyn.strtab_size > 0) {
		printf("[DynamicLinker]     Symbol table: 0x%08x, String table: 0x%08x (%u bytes)\n",
			lib_dyn.symtab_vaddr, lib_dyn.strtab_vaddr, lib_dyn.strtab_size);
		
		// Register symbols from DYNAMIC info
		uint32_t symtab_entries = lib_dyn.symtab_size > 0 ? lib_dyn.symtab_size / sizeof(Elf32_Sym) : 256;
		printf("[DynamicLinker]     Processing %u symbol table entries\n", symtab_entries);
		
		uint32_t loaded_count = 0;
		for (uint32_t i = 1; i < symtab_entries; i++) {  // Skip STN_UNDEF (0)
			Elf32_Sym sym;
			status_t sym_status = fAddressSpace.Read(lib_dyn.symtab_vaddr + i * sizeof(Elf32_Sym),
				&sym, sizeof(Elf32_Sym));
			
			if (sym_status != B_OK) {
				printf("[DynamicLinker]     Read failed at entry %u, stopping\n", i);
				break;
			}
			if (sym.st_name == 0 || sym.st_name >= lib_dyn.strtab_size) continue;
			
			// Read symbol name from string table
			char sym_name[256];
			status_t name_status = fAddressSpace.ReadString(lib_dyn.strtab_vaddr + sym.st_name,
				sym_name, sizeof(sym_name));
			
			if (name_status != B_OK || sym_name[0] == '\0') continue;
			
			// Store symbol in resolver
			uint32_t sym_address = lib_base + sym.st_value;
			uint8_t sym_binding = ELF32_ST_BIND(sym.st_info);
			uint8_t sym_type = ELF32_ST_TYPE(sym.st_info);
			
			// Only store global symbols
			if (sym_binding == STB_GLOBAL || sym_binding == STB_WEAK) {
				ResolvedSymbol resolved;
				resolved.name = sym_name;
				resolved.address = sym_address;
				resolved.size = sym.st_size;
				resolved.binding = sym_binding;
				resolved.type = sym_type;
				resolved.resolved = true;
				
				fResolvedSymbols[sym_name] = resolved;
				loaded_count++;
				
				if (sym_type == STT_FUNC && loaded_count <= 10) {  // Log first few functions
					printf("[DynamicLinker]       [FUNC] %s @ 0x%08x\n", sym_name, sym_address);
				}
			}
		}
		
		printf("[DynamicLinker]   Loaded %u symbols from %s\n", loaded_count, lib_name);
	}

	fLoadedLibraries[lib_path] = lib;
	fNextLoadAddress += (lib_size + 0x1000) & ~0xFFF;  // Align to page

	printf("[DynamicLinker]   LOADED: %s @ 0x%08x (size=0x%x)\n", lib_name, lib_base, lib_size);

	// Apply relocations for the library (CRITICAL FIX)
	printf("[DynamicLinker] Applying relocations for %s...\n", lib_name);
	status_t reloc_status = ApplyRelocations(lib_dyn, lib_base);
	if (reloc_status != B_OK) {
		printf("[DynamicLinker]   WARNING: Relocations for %s had issues\n", lib_name);
	}

	return B_OK;
}

// Load all required libraries
status_t DynamicLinker::LoadRequiredLibraries(const DynamicInfo& dyn_info)
{
	printf("[DynamicLinker] LoadRequiredLibraries: starting\n");

	// Extract DT_NEEDED entries
	std::vector<std::string> needed;
	status_t status = ExtractNeededLibraries(dyn_info, needed);
	if (status != B_OK) {
		printf("[DynamicLinker]   ERROR: Failed to extract needed libraries\n");
		return status;
	}

	if (needed.empty()) {
		printf("[DynamicLinker]   No required libraries\n");
		return B_OK;
	}

	printf("[DynamicLinker]   Loading %zu required libraries\n", needed.size());

	// Load each library
	int loaded = 0;
	int failed = 0;

	for (const auto& lib_name : needed) {
		// Find library file
		std::string lib_path = FindLibraryFile(lib_name.c_str());
		if (lib_path.empty()) {
			printf("[DynamicLinker]   SKIP: %s not found\n", lib_name.c_str());
			failed++;
			continue;
		}

		// Load library
		status_t load_status = LoadLibrary(lib_path.c_str(), lib_name.c_str());
		if (load_status != B_OK) {
			printf("[DynamicLinker]   FAILED: %s (error=%d)\n", lib_name.c_str(), load_status);
			failed++;
		} else {
			loaded++;
		}
	}

	printf("[DynamicLinker] LoadRequiredLibraries: complete (%d loaded, %d failed)\n",
		loaded, failed);

	return (loaded > 0) ? B_OK : B_ERROR;
}

// Load symbol table from main binary for weak symbol resolution
status_t DynamicLinker::LoadMainBinarySymbolTable(FILE* file, const Elf32_Ehdr& ehdr, uint32_t base_address)
{
	printf("[DynamicLinker] LoadMainBinarySymbolTable: loading symbols for main binary\n");

	// Read section headers
	Elf32_Shdr* shdrs = (Elf32_Shdr*)malloc(ehdr.e_shentsize * ehdr.e_shnum);
	if (!shdrs) return B_NO_MEMORY;

	fseek(file, ehdr.e_shoff, SEEK_SET);
	if (fread(shdrs, ehdr.e_shentsize, ehdr.e_shnum, file) != (size_t)ehdr.e_shnum) {
		printf("[DynamicLinker] Failed to read section headers\n");
		free(shdrs);
		return B_ERROR;
	}

	// Read string table for section names
	Elf32_Shdr& shstrtab_hdr = shdrs[ehdr.e_shstrndx];
	char* shstrtab = (char*)malloc(shstrtab_hdr.sh_size);
	if (!shstrtab) {
		free(shdrs);
		return B_NO_MEMORY;
	}

	fseek(file, shstrtab_hdr.sh_offset, SEEK_SET);
	if (fread(shstrtab, 1, shstrtab_hdr.sh_size, file) != shstrtab_hdr.sh_size) {
		printf("[DynamicLinker] Failed to read section name string table\n");
		free(shstrtab);
		free(shdrs);
		return B_ERROR;
	}

	// Find .dynsym section (dynamic symbol table)
	Elf32_Shdr* dynsym_hdr = nullptr;
	Elf32_Shdr* dynstr_hdr = nullptr;

	for (int i = 0; i < ehdr.e_shnum; i++) {
		const char* sec_name = &shstrtab[shdrs[i].sh_name];
		if (strcmp(sec_name, ".dynsym") == 0) {
			dynsym_hdr = &shdrs[i];
			printf("[DynamicLinker] Found .dynsym at offset 0x%08x, size 0x%08x\n",
				dynsym_hdr->sh_offset, dynsym_hdr->sh_size);
		}
		if (strcmp(sec_name, ".dynstr") == 0) {
			dynstr_hdr = &shdrs[i];
			printf("[DynamicLinker] Found .dynstr at offset 0x%08x, size 0x%08x\n",
				dynstr_hdr->sh_offset, dynstr_hdr->sh_size);
		}
	}

	// Load symbol table
	if (dynsym_hdr) {
		fMainSymcount = dynsym_hdr->sh_size / sizeof(Elf32_Sym);
		fMainSymtab = (Elf32_Sym*)malloc(dynsym_hdr->sh_size);
		if (!fMainSymtab) {
			free(shstrtab);
			free(shdrs);
			return B_NO_MEMORY;
		}

		fseek(file, dynsym_hdr->sh_offset, SEEK_SET);
		if (fread(fMainSymtab, 1, dynsym_hdr->sh_size, file) != dynsym_hdr->sh_size) {
			printf("[DynamicLinker] Failed to read main binary symbol table\n");
			free(fMainSymtab);
			fMainSymtab = nullptr;
			free(shstrtab);
			free(shdrs);
			return B_ERROR;
		}
		printf("[DynamicLinker] Loaded %u symbols from main binary\n", fMainSymcount);
	}

	// Load string table
	if (dynstr_hdr) {
		fMainStrtab_size = dynstr_hdr->sh_size;
		fMainStrtab = (char*)malloc(dynstr_hdr->sh_size);
		if (!fMainStrtab) {
			free(fMainSymtab);
			fMainSymtab = nullptr;
			free(shstrtab);
			free(shdrs);
			return B_NO_MEMORY;
		}

		fseek(file, dynstr_hdr->sh_offset, SEEK_SET);
		if (fread(fMainStrtab, 1, dynstr_hdr->sh_size, file) != dynstr_hdr->sh_size) {
			printf("[DynamicLinker] Failed to read main binary string table\n");
			free(fMainStrtab);
			fMainStrtab = nullptr;
			free(fMainSymtab);
			fMainSymtab = nullptr;
			free(shstrtab);
			free(shdrs);
			return B_ERROR;
		}
		printf("[DynamicLinker] Loaded main binary string table (%u bytes)\n", fMainStrtab_size);
	}

	fMainBase = base_address;

	// Register stub functions for missing symbols
	RegisterStubSymbols();

	free(shstrtab);
	free(shdrs);
	return B_OK;
}

// Register stub symbols for functions that may not be in libroot
void DynamicLinker::RegisterStubSymbols()
{
	printf("[DynamicLinker] RegisterStubSymbols: registering stubs for missing functions\n");

	// Use a fixed address in BSS area that should be mapped
	// These are typically weak symbols or missing GNU extensions
	static uint32_t stub_address = 0xbffc0000;  // Safe high stack area (below 0xbffd0000)
	
	// GNU libc and coreutils extensions
	const char* stub_names[] = {
		// GNU coreutils and error handling
		"quote_quoting_options",      // GNU coreutils quoting
		"close_stdout",               // coreutils stdio shutdown
		"version_etc_copyright",      // version copyright string
		"error_message_count",        // error tracking
		"error_print_progname",       // error printing
		"program_name",               // argv[0]
		"exit_failure",               // exit code
		"thrd_exit",                  // thread exit
		"Version",                    // version string
		"error_one_per_line",         // error one-per-line flag
		
		// GNU libc memory allocation wrappers
		"xmalloc",                    // allocate or exit
		"xcalloc",                    // calloc or exit
		"xrealloc",                   // realloc or exit
		"xcharalloc",                 // char array allocation
		"xmemdup",                    // duplicate with alloc
		"x2nrealloc",                 // reallocate with doubling
		"xireallocarray",             // int reallocarray
		"xreallocarray",              // reallocarray
		"ximalloc",                   // int malloc
		"xicalloc",                   // int calloc
		
		// GNU libc error functions
		"error",                      // formatted error output
		"xalloc_die",                 // memory exhaustion handler
		
		// GNU quoting functions (quotearg)
		"quotearg_alloc_mem",         // quotearg with allocation
		"quotearg_n_custom_mem",      // custom quoting
		"quotearg_n_custom",          // custom quoting (string)
		"quotearg_n_mem",             // quotearg memory version
		"quotearg_n",                 // quotearg for arg n
		"quotearg_char_mem",          // quotearg char delim
		"quotearg_char",              // quotearg char delim (string)
		"quotearg_colon",             // quote with colon
		"quotearg_n_style",           // quotearg with style
		"quotearg_n_style_mem",       // quotearg style memory
		"quote_n",                    // quote arg n
		"quote_n_mem",                // quote memory version
		
		// GNU libc version/program functions
		"set_program_name",           // set argv[0]
		"getprogname",                // get program name
		"version_etc",                // print version info
		"version_etc_arn",            // version with author array
		"version_etc_va",             // version with va_list
		"usage",                      // print usage message
		
		// GNU libc locale/encoding functions
		"locale_charset",             // get locale charset
		"hard_locale",                // check if locale is hard
		"setlocale_null_r",           // reentrant setlocale null
		"rpl_nl_langinfo",            // replacement nl_langinfo
		
		// rpl_* replacement functions
		"rpl_malloc",                 // replacement malloc
		"rpl_calloc",                 // replacement calloc
		"rpl_realloc",                // replacement realloc
		"rpl_free",                   // replacement free
		"rpl_mbrtowc",                // replacement mbrtowc
		"rpl_fclose",                 // replacement fclose
		"rpl_fflush",                 // replacement fflush
		"rpl_fseeko",                 // replacement fseeko
		"rpl_vfprintf",               // replacement vfprintf
		
		// GNU quoting option functions
		"set_char_quoting",           // set char quoting
		"set_custom_quoting",         // set custom quoting
		
		// GNU printf functions
		"printf_parse",               // parse printf format
		"printf_fetchargs",           // fetch printf args
		"vasnprintf",                 // va_list snprintf
		
		// GNU stream functions
		"fseterr",                    // set stream error
		"close_stream",               // close stream with error check
		
		// GNU filesystem functions
		"globfree",                   // free glob result
		
		// Less common GNU functions from libc
		"gl_get_setlocale_null_lock", // locale null lock
	};
	
	int stub_count = sizeof(stub_names) / sizeof(stub_names[0]);
	
	for (int i = 0; i < stub_count; i++) {
		// Just create a resolved symbol pointing to this address
		// These are typically weak symbols that may or may not be used
		ResolvedSymbol sym;
		sym.name = stub_names[i];
		sym.address = stub_address;
		sym.size = 0;
		sym.binding = STB_WEAK;      // These are weak symbols
		sym.type = STT_NOTYPE;
		sym.resolved = false;        // Mark as unresolved but available
		
		fResolvedSymbols[stub_names[i]] = sym;
		stub_address += 16;  // Each stub gets 16 bytes
		printf("[DynamicLinker]   Stub: %s @ 0x%08x\n", stub_names[i], sym.address);
	}
}
