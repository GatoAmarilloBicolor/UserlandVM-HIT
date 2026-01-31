/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "LibraryLoader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

status_t LibraryLoader::LoadLibrary(const char* lib_path, SymbolResolver& symbol_resolver,
	uint32_t load_base)
{
	if (!lib_path) {
		printf("[LibraryLoader] Error: lib_path is NULL\n");
		return B_BAD_VALUE;
	}

	FILE* f = fopen(lib_path, "rb");
	if (!f) {
		printf("[LibraryLoader] Error: Cannot open library file: %s\n", lib_path);
		return B_ERROR;
	}

	printf("[LibraryLoader] Loading library: %s (base=0x%08x)\n", lib_path, load_base);

	// Read and verify ELF header
	unsigned char ident[16];
	if (fread(ident, 1, 16, f) != 16) {
		fclose(f);
		printf("[LibraryLoader] Error: Cannot read ELF header\n");
		return B_ERROR;
	}

	// Verify ELF magic
	if (ident[0] != 0x7f || ident[1] != 'E' || ident[2] != 'L' || ident[3] != 'F') {
		fclose(f);
		printf("[LibraryLoader] Error: Not an ELF file\n");
		return B_BAD_VALUE;
	}

	// Read full ELF header
	Elf32_Ehdr header;
	fseek(f, 0, SEEK_SET);
	if (fread(&header, sizeof(Elf32_Ehdr), 1, f) != 1) {
		fclose(f);
		printf("[LibraryLoader] Error: Cannot read ELF header\n");
		return B_ERROR;
	}

	// Get section header table info
	uint32_t shoff = header.e_shoff;
	uint32_t shnum = header.e_shnum;
	uint32_t shentsize = header.e_shentsize;

	printf("[LibraryLoader] Library has %u sections\n", shnum);

	// Read section headers to find .dynsym and .dynstr
	Elf32_Shdr* sections = (Elf32_Shdr*)malloc(shnum * shentsize);
	if (!sections) {
		fclose(f);
		return B_NO_MEMORY;
	}

	fseek(f, shoff, SEEK_SET);
	if (fread(sections, shentsize, shnum, f) != shnum) {
		free(sections);
		fclose(f);
		printf("[LibraryLoader] Error: Cannot read section headers\n");
		return B_ERROR;
	}

	// Find .dynsym (section type SHT_DYNSYM = 11)
	Elf32_Shdr* dynsym_shdr = NULL;
	Elf32_Shdr* dynstr_shdr = NULL;

	for (uint32_t i = 0; i < shnum; i++) {
		if (sections[i].sh_type == SHT_DYNSYM) {
			dynsym_shdr = &sections[i];
			printf("[LibraryLoader] Found .dynsym at offset 0x%08x, size=%u, entsize=%u\n",
				sections[i].sh_offset, sections[i].sh_size, sections[i].sh_entsize);
		}
		if (sections[i].sh_type == SHT_STRTAB) {
			// Assume the first SHT_STRTAB after dynsym is dynstr
			// Or check section name (but that requires string table lookup)
			if (dynsym_shdr && !dynstr_shdr) {
				dynstr_shdr = &sections[i];
				printf("[LibraryLoader] Found .dynstr at offset 0x%08x, size=%u\n",
					sections[i].sh_offset, sections[i].sh_size);
			}
		}
	}

	if (!dynsym_shdr || !dynstr_shdr) {
		printf("[LibraryLoader] Warning: Could not find .dynsym or .dynstr sections\n");
		free(sections);
		fclose(f);
		return B_ERROR;  // Can't continue without symbol table
	}

	// Read .dynstr (string table)
	uint8_t* dynstr = (uint8_t*)malloc(dynstr_shdr->sh_size);
	if (!dynstr) {
		free(sections);
		fclose(f);
		return B_NO_MEMORY;
	}

	fseek(f, dynstr_shdr->sh_offset, SEEK_SET);
	if (fread(dynstr, 1, dynstr_shdr->sh_size, f) != dynstr_shdr->sh_size) {
		free(dynstr);
		free(sections);
		fclose(f);
		printf("[LibraryLoader] Error: Cannot read string table\n");
		return B_ERROR;
	}

	// Read .dynsym (symbol table)
	uint8_t* dynsym_data = (uint8_t*)malloc(dynsym_shdr->sh_size);
	if (!dynsym_data) {
		free(dynstr);
		free(sections);
		fclose(f);
		return B_NO_MEMORY;
	}

	fseek(f, dynsym_shdr->sh_offset, SEEK_SET);
	if (fread(dynsym_data, 1, dynsym_shdr->sh_size, f) != dynsym_shdr->sh_size) {
		free(dynsym_data);
		free(dynstr);
		free(sections);
		fclose(f);
		printf("[LibraryLoader] Error: Cannot read symbol table\n");
		return B_ERROR;
	}

	// Extract symbols and register with SymbolResolver
	Elf32_Sym* symtab = (Elf32_Sym*)dynsym_data;
	uint32_t symcount = dynsym_shdr->sh_size / sizeof(Elf32_Sym);
	
	Library lib;
	lib.path = lib_path;
	lib.baseAddress = load_base;
	lib.size = 0x1000000;  // Assume 16MB for library size
	lib.soname = "libroot.so";  // Default soname

	printf("[LibraryLoader] Extracting %u symbols from library\n", symcount);

	// Extract all symbols
	uint32_t exported_symbols = 0;
	for (uint32_t i = 0; i < symcount; i++) {
		Elf32_Sym& sym = symtab[i];

		// Skip unnamed symbols
		if (sym.st_name == 0 || sym.st_name >= dynstr_shdr->sh_size) continue;

		const char* sym_name = (const char*)&dynstr[sym.st_name];

		// Only include global and weak symbols
		uint8_t binding = ELF32_ST_BIND(sym.st_info);
		if (binding != STB_GLOBAL && binding != STB_WEAK) continue;

		// Calculate absolute address
		uint32_t sym_addr = load_base + sym.st_value;

		Symbol library_symbol;
		library_symbol.name = sym_name;
		library_symbol.address = sym_addr;
		library_symbol.size = sym.st_size;
		library_symbol.binding = sym.st_info;  // Store full info byte
		library_symbol.type = sym.st_info;
		library_symbol.shndx = sym.st_shndx;

		lib.symbols.push_back(library_symbol);
		exported_symbols++;

		if (exported_symbols <= 10) {  // Print first 10 for debugging
			printf("[LibraryLoader]   Symbol: %s @ 0x%08x (size=%u)\n",
				sym_name, sym_addr, sym.st_size);
		}
	}

	printf("[LibraryLoader] Extracted %u exported symbols\n", exported_symbols);

	// Register library with symbol resolver
	status_t status = symbol_resolver.RegisterLibrary(lib);

	free(dynsym_data);
	free(dynstr);
	free(sections);
	fclose(f);

	return status;
}

status_t LibraryLoader::LoadLibroot(const char* sysroot_path, SymbolResolver& symbol_resolver)
{
	if (!sysroot_path) {
		printf("[LibraryLoader] Error: sysroot_path is NULL\n");
		return B_BAD_VALUE;
	}

	// Try to find libroot.so in the sysroot
	char libroot_path[512];
	snprintf(libroot_path, sizeof(libroot_path), "%s/lib/libroot.so", sysroot_path);

	printf("[LibraryLoader] Attempting to load libroot from: %s\n", libroot_path);

	// Load the library
	return LoadLibrary(libroot_path, symbol_resolver, 0x50000000);  // Use 0x50000000 as base for libroot
}
