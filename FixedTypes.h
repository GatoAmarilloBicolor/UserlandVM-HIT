/*
 * UserlandVM Fixed Type Definitions
 * Cross-platform compatibility without conflicts
 */

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

// Core type definitions without system conflicts
typedef int32_t status_t;
typedef int32_t area_id;
typedef int32_t team_id;
typedef uintptr_t addr_t;
typedef uintptr_t phys_addr_t;
typedef uintptr_t vm_addr_t;
typedef size_t vm_size_t;

// Haiku-compatible status codes
#define B_OK ((status_t)0)
#define B_ERROR (-1)
#define B_NO_MEMORY (-2)
#define B_BAD_VALUE (-3)
#define B_ENTRY_NOT_FOUND (-6)
#define B_NAME_IN_USE (-15)
#define B_NOT_SUPPORTED (-10)
#define B_FILE_ERROR (-227)

// Memory protection flags
enum {
    MEMORY_READ = 0x01,
    MEMORY_WRITE = 0x02,
    MEMORY_EXECUTE = 0x04,
    MEMORY_READ_WRITE = (MEMORY_READ | MEMORY_WRITE),
    MEMORY_ALL = (MEMORY_READ | MEMORY_WRITE | MEMORY_EXECUTE)
};

// Cross-platform ELF types without system conflicts
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

// Architecture constants
#define EM_386 3
#define EM_X86_64 62
#define ELFCLASS32 1
#define ELFCLASS64 2

// ELF types
#define ET_EXEC 2
#define ET_DYN 3
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_NULL 0

// Relocation types
#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2
#define R_386_GOT32 3
#define R_386_PLT32 4
#define R_386_COPY 5
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8

// Dynamic section tags
#define DT_NULL 0
#define DT_NEEDED 1
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_REL 17
#define DT_RELA 7
#define DT_JMPREL 23
#define DT_PLTRELSZ 2

// Symbol info
#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2

// Forward declarations
class GuestContext;
class AddressSpace;
class SyscallDispatcher;

// Safe POSIX wrappers
extern "C" {
#include <sys/stat.h>
#include <unistd.h>
}

inline bool SafeFileExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

inline bool SafeDirectoryExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

inline void* SafeMalloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) memset(ptr, 0, size);
    return ptr;
}

inline void SafeFree(void* ptr) {
    if (ptr) free(ptr);
}

// Debug macros
#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

// Error macros
#define ERROR_PRINT(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define WARN_PRINT(fmt, ...)  printf("[WARN] " fmt "\n", ##__VA_ARGS__)

// Safe string operations
inline std::string SafeString(const char* str) {
    return std::string(str ? str : "");
}

// Architecture detection
inline bool IsX86_64() {
    return sizeof(void*) == 8;
}

inline bool IsX86_32() {
    return sizeof(void*) == 4 && !IsX86_64();
}

// Compatibility shims for Haiku types
#define B_ANY_ADDRESS 0
#define B_NO_LOCK 0
#define B_READ_AREA MEMORY_READ
#define B_WRITE_AREA MEMORY_WRITE
#define B_READ_WRITE MEMORY_READ_WRITE