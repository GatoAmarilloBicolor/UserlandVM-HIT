/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under terms of MIT License.
 * 
 * SimpleRelocationProcessor.h - Basic relocations for dynamic binaries
 */

#pragma once

#include <stdint.h>

// Basic ELF types for relocation processing
#define ELF32_R_SYM(info)  ((info) >> 8)
#define ELF32_R_TYPE(info) ((info) & 0xff)
#define R_386_JUMP_SLOT  7
#define R_386_GLOB_DAT   6
#define R_386_RELATIVE   8
#define R_386_32        2

// ELF relocation structure
struct Elf32_Rel {
    uint32_t r_offset;
    uint32_t r_info;
};

// Forward declarations
class DynamicLinker;

class SimpleRelocationProcessor {
public:
    SimpleRelocationProcessor();
    ~SimpleRelocationProcessor();

    // Process relocations for a dynamic binary
    bool ProcessRelocations(uint8_t* programBase, uint32_t programSize,
                          Elf32_Rel* relocations, uint32_t relCount,
                          DynamicLinker* linker);

private:
    // Specific relocation handlers
    bool ProcessJumpSlot(uint8_t* relocAddr, uint32_t symbolIndex, DynamicLinker* linker);
    bool ProcessGlobalData(uint8_t* relocAddr, uint32_t symbolIndex, DynamicLinker* linker);
    void ProcessRelative(uint8_t* relocAddr, uint8_t* programBase);
    bool ProcessAbsolute32(uint8_t* relocAddr, uint32_t symbolIndex, DynamicLinker* linker);

    // Helper to get symbol name from index (simplified)
    const char* GetSymbolName(uint32_t symbolIndex);
};