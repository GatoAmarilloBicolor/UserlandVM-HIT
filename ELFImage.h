#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "SupportDefs.h"

// Only define ELF structures if system headers haven't already defined them
#ifndef _ELF_H
// Simplified ELF structures for cross-platform compatibility
#define EI_NIDENT 16

#ifndef Elf32_Ehdr
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;
#endif

#ifndef Elf32_Phdr
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;
#endif

#ifndef Elf32_Shdr
typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} Elf32_Shdr;
#endif

#ifndef Elf32_Sym
typedef struct {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
} Elf32_Sym;
#endif

#ifndef Elf32_Dyn
typedef struct {
    uint32_t d_tag;
    uint32_t d_val;
} Elf32_Dyn;
#endif

#ifndef Elf32_Rel
typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
} Elf32_Rel;
#endif

#ifndef Elf32_Rela
typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
    int32_t r_addend;
} Elf32_Rela;
#endif

#endif // _ELF_H

// ELF constants
#define ELFMAG "\177ELF"
#define EI_CLASS 4
#define ELFCLASS32 1
#define EI_DATA 5
#define ELFDATA2LSB 1

#define ET_EXEC 2
#define ET_DYN 3

#define EM_386 3

#define EV_CURRENT 1

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3

#define PF_R 0x4
#define PF_W 0x2
#define PF_X 0x1

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_DYNAMIC 6
#define SHT_REL 9

#define STB_GLOBAL 1
#define STT_FUNC 2

#define DT_NULL 0
#define DT_NEEDED 1
#define DT_STRTAB 5
#define DT_SYMTAB 6
#define DT_REL 17
#define DT_RELA 7
#define DT_JMPREL 23
#define DT_PLTRELSZ 2

#define R_386_JUMP_SLOT 7
#define R_386_GLOB_DAT 6
#define R_386_RELATIVE 8
#define R_386_32 2

class ElfImage {
public:
    ElfImage();
    ~ElfImage();
    
    static ElfImage* Load(const char* path);
    
    bool IsDynamic() const;
    const char* GetArchString() const;
    uint32_t GetEntry() const;
    
    void* GetBaseAddress() const;
    size_t GetSize() const;
    
    const Elf32_Ehdr& GetHeader() const;
    const Elf32_Phdr* GetProgramHeaders() const;
    const Elf32_Shdr* GetSectionHeaders() const;
    
    const char* GetStringTable() const;
    const char* GetDynamicStringTable() const;
    const Elf32_Sym* GetSymbolTable() const;
    const Elf32_Dyn* GetDynamicSection() const;
    
    uint32_t GetSymbolCount() const;
    const char* GetSymbolName(uint32_t index) const;
    
    // PT_INTERP support
    bool HasInterpreter() const;
    const char* GetInterpreterPath() const;
    
private:
    FILE* fFile;
    Elf32_Ehdr fHeader;
    Elf32_Phdr* fProgramHeaders;
    Elf32_Shdr* fSectionHeaders;
    char* fStringTable;
    char* fDynamicStringTable;
    Elf32_Sym* fSymbolTable;
    Elf32_Dyn* fDynamicSection;
    char* fInterpreterPath;
    
    uint32_t fSymbolCount;
    bool fIsLoaded;
    
    bool ReadHeaders();
    bool LoadStringTable();
    bool LoadSymbolTable();
    bool LoadDynamicSection();
    
    void Cleanup();
};