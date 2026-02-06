/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * ProgramHeaderParser.h - Complete ELF program header parsing for maximum compatibility
 */

#ifndef PROGRAM_HEADER_PARSER_H
#define PROGRAM_HEADER_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

// Forward declarations
class ElfImage;

class ProgramHeaderParser {
public:
    // Dynamic section information extracted from PT_DYNAMIC
    struct DynamicInfo {
        // Dependencies (DT_NEEDED)
        std::vector<std::string> needed_libs;
        
        // Library identification
        std::string soname;
        std::string rpath;
        std::string runpath;
        
        // Initialization and finalization
        uint32_t init_addr;
        uint32_t fini_addr;
        uint32_t init_array_addr;
        uint32_t fini_array_addr;
        uint32_t init_array_size;
        uint32_t fini_array_size;
        
        // Relocation information
        uint32_t rel_addr;
        uint32_t rel_size;
        uint32_t rela_addr;
        uint32_t rela_size;
        uint32_t plt_rel_addr;
        uint32_t plt_rel_size;
        uint32_t plt_rela_addr;
        uint32_t plt_rela_size;
        
        // Hash tables for symbol lookup
        uint32_t hash_addr;
        uint32_t gnu_hash_addr;
        
        // Symbol information
        uint32_t strtab_addr;
        uint32_t strtab_size;
        uint32_t symtab_addr;
        uint32_t symtab_size;
        
        // ELF identity information
        uint32_t flags;
        uint32_t debug;
        
        // Thread Local Storage
        uint32_t tls_addr;
        uint32_t tls_size;
        uint32_t tls_align;
        
        // Position Independent Code
        uint32_t pltgot_addr;
        
        // Constructor/destructor counts
        uint32_t init_array_count;
        uint32_t fini_array_count;
        
        DynamicInfo() : init_addr(0), fini_addr(0), init_array_addr(0), fini_array_addr(0),
                       init_array_size(0), fini_array_size(0), rel_addr(0), rel_size(0),
                       rela_addr(0), rela_size(0), plt_rel_addr(0), plt_rel_size(0),
                       plt_rela_addr(0), plt_rela_size(0), hash_addr(0), gnu_hash_addr(0),
                       strtab_addr(0), strtab_size(0), symtab_addr(0), symtab_size(0),
                       flags(0), debug(0), tls_addr(0), tls_size(0), tls_align(0),
                       pltgot_addr(0), init_array_count(0), fini_array_count(0) {}
    };

    // Program header types
    enum ProgramHeaderType {
        PT_NULL = 0,
        PT_LOAD = 1,
        PT_DYNAMIC = 2,
        PT_INTERP = 3,
        PT_NOTE = 4,
        PT_SHLIB = 5,
        PT_PHDR = 6,
        PT_TLS = 7,
        PT_GNU_EH_FRAME = 0x6474e550,
        PT_GNU_STACK = 0x6474e551,
        PT_GNU_RELRO = 0x6474e552
    };

    // Dynamic entry types
    enum DynamicType {
        DT_NULL = 0,
        DT_NEEDED = 1,
        DT_PLTRELSZ = 2,
        DT_PLTGOT = 3,
        DT_HASH = 4,
        DT_STRTAB = 5,
        DT_SYMTAB = 6,
        DT_RELA = 7,
        DT_RELASZ = 8,
        DT_RELAENT = 9,
        DT_STRSZ = 10,
        DT_SYMENT = 11,
        DT_INIT = 12,
        DT_FINI = 13,
        DT_SONAME = 14,
        DT_RPATH = 15,
        DT_SYMBOLIC = 16,
        DT_REL = 17,
        DT_RELSZ = 18,
        DT_RELENT = 19,
        DT_PLTREL = 20,
        DT_DEBUG = 21,
        DT_TEXTREL = 22,
        DT_JMPREL = 23,
        DT_BIND_NOW = 24,
        DT_INIT_ARRAY = 25,
        DT_FINI_ARRAY = 26,
        DT_INIT_ARRAYSZ = 27,
        DT_FINI_ARRAYSZ = 28,
        DT_RUNPATH = 29,
        DT_FLAGS = 30,
        DT_GNU_HASH = 0x6ffffef5,
        DT_RELACOUNT = 0x6ffffef9,
        DT_RELCOUNT = 0x6ffffefa,
        DT_FLAGS_1 = 0x6ffffffb,
        DT_VERSYM = 0x6ffffff0,
        DT_VERNEEDED = 0x6ffffffe,
        DT_VERDEF = 0x6ffffffc,
        DT_VERDEFNUM = 0x6ffffffd
    };

    // Parse all program headers from ELF image
    static bool ParseProgramHeaders(ElfImage* image);
    
    // Parse dynamic section specifically
    static DynamicInfo ParseDynamicSection(ElfImage* image);
    
    // Extract interpreter path from PT_INTERP
    static std::string GetInterpreter(ElfImage* image);
    
    // Check if image is position independent
    static bool IsPositionIndependent(ElfImage* image);
    
    // Get TLS information
    static bool GetTLSInfo(ElfImage* image, uint32_t& addr, uint32_t& size, uint32_t& align);
    
    // Check for RELRO protection
    static bool HasRelroProtection(ElfImage* image);

private:
    // Helper methods
    static bool ParseDynamicEntry(ElfImage* image, uint32_t tag, uint32_t val, DynamicInfo& info);
    static std::string ReadString(ElfImage* image, uint32_t addr);
    static bool ValidateProgramHeader(ElfImage* image, int ph_index);
    
    // For compatibility with existing code
    struct DynamicEntry {
        uint32_t d_tag;
        uint32_t d_val;
    };
};

#endif // PROGRAM_HEADER_PARSER_H