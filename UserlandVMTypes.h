/*
 * UserlandVM Type Definitions - Cross-platform Compatibility
 * Resolves all conflicts between Haiku and system types
 */

#pragma once

#include <stdint.h>

// Cross-platform status_t definition
#ifndef STATUS_T_DEFINED
#define STATUS_T_DEFINED
typedef int32_t status_t;
#endif

// Cross-platform type definitions
#ifndef AREA_ID_DEFINED
#define AREA_ID_DEFINED
typedef int32_t area_id;
#endif

#ifndef TEAM_ID_DEFINED
#define TEAM_ID_DEFINED  
typedef int32_t team_id;
#endif

// Use pointer-sized types for addresses
typedef uintptr_t addr_t;
typedef uintptr_t phys_addr_t;
typedef uintptr_t vm_addr_t;
typedef size_t vm_size_t;

// Haiku-compatible status codes (cross-platform)
#ifndef B_OK
#define B_OK ((status_t)0)
#define B_ERROR (-1)
#define B_NO_MEMORY (-2)
#define B_BAD_VALUE (-3)
#define B_ENTRY_NOT_FOUND (-6)
#define B_NAME_IN_USE (-15)
#define B_PERMISSION_DENIED (-13)
#define B_WOULD_BLOCK (-7)
#endif

// Memory protection flags
#ifndef MEMORY_PROTECTION_DEFINED
#define MEMORY_PROTECTION_DEFINED
enum {
    MEMORY_READ = 0x01,
    MEMORY_WRITE = 0x02,
    MEMORY_EXECUTE = 0x04,
    MEMORY_READ_WRITE = (MEMORY_READ | MEMORY_WRITE),
    MEMORY_ALL = (MEMORY_READ | MEMORY_WRITE | MEMORY_EXECUTE)
};
#endif

// Architecture detection without system headers
#ifdef __x86_64__
    #define ARCH_X86_64 1
    #define ARCH_X86_32 0
#elif defined(__i386__)
    #define ARCH_X86_32 1
    #define ARCH_X86_64 0
#else
    #define ARCH_X86_64 0
    #define ARCH_X86_32 0
#endif

// Simplified ELF types without conflicts
typedef struct {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_shentsize;
    uint16_t e_shstrndx;
} userland_elf32_ehdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} userland_elf32_phdr_t;

// Safe include guards
#ifdef __cplusplus
extern "C" {
#endif

// Safe POSIX declarations without system conflicts
#ifndef SAFE_POSIX_DECLARATIONS
#define SAFE_POSIX_DECLARATIONS
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#ifdef __cplusplus
}
#endif

// Disable problematic system headers that cause conflicts
#define __ELF__  // Prevent system ELF definitions
#define _SYS_SYSMACROS_H_  // Prevent macro conflicts

// Clean compilation environment
#pragma push_macro("SEEK_SET")
#pragma push_macro("SEEK_CUR")
#pragma push_macro("FILE_OFFSET_BITS")

// Standard POSIX includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// Restore macros
#pragma pop_macro("SEEK_SET")
#pragma pop_macro("SEEK_CUR")
#pragma pop_macro("FILE_OFFSET_BITS")