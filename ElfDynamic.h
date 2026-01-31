/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _ELF_DYNAMIC_H_
#define _ELF_DYNAMIC_H_

#include <SupportDefs.h>
#include <elf.h>

// Dynamic tag constants (some may be missing from host elf.h)
#ifndef DT_INIT_ARRAY
#define DT_INIT_ARRAY   25  // Array with addresses of init functions
#endif

#ifndef DT_FINI_ARRAY
#define DT_FINI_ARRAY   26  // Array with addresses of fini functions
#endif

#ifndef DT_INIT_ARRAYSZ
#define DT_INIT_ARRAYSZ 27  // Size of init functions array
#endif

#ifndef DT_FINI_ARRAYSZ
#define DT_FINI_ARRAYSZ 28  // Size of fini functions array
#endif

#ifndef DT_RUNPATH
#define DT_RUNPATH      29  // Library search path (supersedes DT_RPATH)
#endif

#ifndef DT_FLAGS
#define DT_FLAGS        30  // Flags for object being loaded
#endif

#ifndef DT_ENCODING
#define DT_ENCODING     32  // Start of encoded range
#endif

#ifndef DT_PREINIT_ARRAY
#define DT_PREINIT_ARRAY    32  // Array with addresses of preinit functions
#endif

#ifndef DT_PREINIT_ARRAYSZ
#define DT_PREINIT_ARRAYSZ  33  // Size of preinit functions array
#endif

// DT_FLAGS values
#ifndef DF_ORIGIN
#define DF_ORIGIN       0x0001  // Object uses DF_ORIGIN
#endif

#ifndef DF_SYMBOLIC
#define DF_SYMBOLIC     0x0002  // Symbol resolutions starts here
#endif

#ifndef DF_TEXTREL
#define DF_TEXTREL      0x0004  // Object contains text relocations
#endif

#ifndef DF_BIND_NOW
#define DF_BIND_NOW     0x0008  // No lazy binding for this object
#endif

#ifndef DF_STATIC_TLS
#define DF_STATIC_TLS   0x0010  // Static TLS model negotiated
#endif

// Relocation types for x86 (if not in elf.h)
#ifndef R_386_NONE
#define R_386_NONE              0   // No relocation
#endif

#ifndef R_386_32
#define R_386_32                1   // S + A
#endif

#ifndef R_386_PC32
#define R_386_PC32              2   // S + A - P
#endif

#ifndef R_386_GOT32
#define R_386_GOT32             3   // G + A - P
#endif

#ifndef R_386_PLT32
#define R_386_PLT32             4   // L + A - P
#endif

#ifndef R_386_COPY
#define R_386_COPY              5   // None
#endif

#ifndef R_386_GLOB_DAT
#define R_386_GLOB_DAT          6   // S
#endif

#ifndef R_386_JMP_SLOT
#define R_386_JMP_SLOT          7   // S
#endif

#ifndef R_386_RELATIVE
#define R_386_RELATIVE          8   // B + A
#endif

#ifndef R_386_GOTOFF
#define R_386_GOTOFF            9   // S + A - GOT
#endif

#ifndef R_386_GOTPC
#define R_386_GOTPC             10  // GOT + A - P
#endif

// Symbol binding macros (if not defined)
#ifndef ELF32_ST_BIND
#define ELF32_ST_BIND(info)     ((info) >> 4)
#endif

#ifndef ELF32_ST_TYPE
#define ELF32_ST_TYPE(info)     ((info) & 0x0f)
#endif

#ifndef ELF32_ST_INFO
#define ELF32_ST_INFO(bind, type) (((bind)<<4) + ((type)&0x0f))
#endif

// Symbol types (if not defined)
#ifndef STT_NOTYPE
#define STT_NOTYPE  0
#endif

#ifndef STT_OBJECT
#define STT_OBJECT  1
#endif

#ifndef STT_FUNC
#define STT_FUNC    2
#endif

#ifndef STT_SECTION
#define STT_SECTION 3
#endif

#ifndef STT_FILE
#define STT_FILE    4
#endif

// Symbol binding (if not defined)
#ifndef STB_LOCAL
#define STB_LOCAL  0
#endif

#ifndef STB_GLOBAL
#define STB_GLOBAL 1
#endif

#ifndef STB_WEAK
#define STB_WEAK   2
#endif

// Relocation macros (if not defined)
#ifndef ELF32_R_SYM
#define ELF32_R_SYM(info)       ((info) >> 8)
#endif

#ifndef ELF32_R_TYPE
#define ELF32_R_TYPE(info)      ((info) & 0xff)
#endif

#ifndef ELF32_R_INFO
#define ELF32_R_INFO(sym, type) (((sym)<<8) + ((type)&0xff))
#endif

// Structure to hold parsed dynamic information
struct DynamicInfo {
    const Elf32_Dyn* dynamic;       // DYNAMIC segment (pointer to host memory)
    uint32_t symtab_vaddr;          // Symbol table virtual address (guest)
    uint32_t strtab_vaddr;          // String table virtual address (guest)
    uint32_t rel_vaddr;             // Relocations virtual address (guest)
    uint32_t rela_vaddr;            // Relocations with addend virtual address (guest)
    uint32_t jmprel_vaddr;          // PLT relocations virtual address (guest)
    // const Elf32_Hash* hash;      // Hash table (not in all systems)

    uint32_t symtab_size;           // Number of symbols (actual size in bytes)
    uint32_t strtab_size;           // String table size
    uint32_t rel_size;              // Relocations size
    uint32_t rela_size;             // Relocations with addend size
    uint32_t jmprel_size;           // PLT relocations size
    uint32_t rel_entry_size;        // Size of one relocation
    uint32_t rela_entry_size;       // Size of one relocation with addend
    uint32_t init_array_size;       // Init array size
    uint32_t fini_array_size;       // Fini array size

    uint32_t pltgot;                // PLT/GOT address
    uint32_t init;                  // Init function address
    uint32_t fini;                  // Fini function address
    uint32_t flags;                 // DT_FLAGS value

    uint32_t init_array_vaddr;      // Init functions array virtual address (guest)
    uint32_t fini_array_vaddr;      // Fini functions array virtual address (guest)

    bool has_rel;
    bool has_rela;
    bool bind_now;
};

#endif // _ELF_DYNAMIC_H_
