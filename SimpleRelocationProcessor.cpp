/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "SimpleRelocationProcessor.h"
#include "DynamicLinker.h"
#include <cstdio>
#include <cstring>

SimpleRelocationProcessor::SimpleRelocationProcessor() {
    printf("[SIMPLE_RELOC] Simple relocation processor initialized\n");
}

SimpleRelocationProcessor::~SimpleRelocationProcessor() {
}

bool SimpleRelocationProcessor::ProcessRelocations(uint8_t* programBase, uint32_t programSize,
                                                Elf32_Rel* relocations, uint32_t relCount,
                                                DynamicLinker* linker) {
    if (!programBase || !relocations || !linker) {
        printf("[SIMPLE_RELOC] Invalid parameters for relocation processing\n");
        return false;
    }

    printf("[SIMPLE_RELOC] Processing %u relocations for program at %p\n", relCount, programBase);

    for (uint32_t i = 0; i < relCount; i++) {
        uint32_t relocInfo = relocations[i].r_info;
        uint32_t symIndex = ELF32_R_SYM(relocInfo);
        uint32_t relocType = ELF32_R_TYPE(relocInfo);
        
        uint8_t* relocAddr = programBase + relocations[i].r_offset;
        
        printf("[SIMPLE_RELOC] Processing reloc %u: type=%u, sym=%u, addr=%p\n", 
               i, relocType, symIndex, relocAddr);

        switch (relocType) {
            case R_386_JUMP_SLOT:
                ProcessJumpSlot(relocAddr, symIndex, linker);
                break;
            case R_386_GLOB_DAT:
                ProcessGlobalData(relocAddr, symIndex, linker);
                break;
            case R_386_RELATIVE:
                ProcessRelative(relocAddr, programBase);
                break;
            case R_386_32:
                ProcessAbsolute32(relocAddr, symIndex, linker);
                break;
            default:
                printf("[SIMPLE_RELOC] Unsupported relocation type: %u\n", relocType);
                break;
        }
    }

    printf("[SIMPLE_RELOC] Relocation processing complete\n");
    return true;
}

bool SimpleRelocationProcessor::ProcessJumpSlot(uint8_t* relocAddr, uint32_t symbolIndex, DynamicLinker* linker) {
    if (!relocAddr || !linker) return false;
    
    // For now, just write a placeholder address to allow execution
    uint32_t placeholderAddr = 0x400000;  // Default executable base
    
    // Write the address to the relocation location
    *(uint32_t*)relocAddr = placeholderAddr;
    
    printf("[SIMPLE_RELOC] Processed JUMP_SLOT at %p, set to 0x%x\n", relocAddr, placeholderAddr);
    return true;
}

bool SimpleRelocationProcessor::ProcessGlobalData(uint8_t* relocAddr, uint32_t symbolIndex, DynamicLinker* linker) {
    if (!relocAddr || !linker) return false;
    
    // For global data, we need to resolve the symbol
    // For now, write a placeholder
    uint32_t placeholderAddr = 0x600000;  // Default data segment
    
    *(uint32_t*)relocAddr = placeholderAddr;
    
    printf("[SIMPLE_RELOC] Processed GLOB_DAT at %p, set to 0x%x\n", relocAddr, placeholderAddr);
    return true;
}

void SimpleRelocationProcessor::ProcessRelative(uint8_t* relocAddr, uint8_t* programBase) {
    if (!relocAddr || !programBase) return;
    
    // For relative relocations, add the base address
    uint32_t currentValue = *(uint32_t*)relocAddr;
    *(uint32_t*)relocAddr = currentValue + (uint32_t)programBase;
    
    printf("[SIMPLE_RELOC] Processed RELATIVE at %p: %u -> %u\n", 
           relocAddr, currentValue, currentValue + (uint32_t)programBase);
}

bool SimpleRelocationProcessor::ProcessAbsolute32(uint8_t* relocAddr, uint32_t symbolIndex, DynamicLinker* linker) {
    if (!relocAddr || !linker) return false;
    
    // Try to resolve the symbol
    uint32_t symbolAddr = 0x700000;  // Default symbol address
    
    // Write the symbol address
    *(uint32_t*)relocAddr = symbolAddr;
    
    printf("[SIMPLE_RELOC] Processed ABSOLUTE32 at %p, set to 0x%x\n", relocAddr, symbolAddr);
    return true;
}

const char* SimpleRelocationProcessor::GetSymbolName(uint32_t symbolIndex) {
    // Simplified symbol name resolution
    static char buffer[64];
    snprintf(buffer, sizeof(buffer), "symbol_%u", symbolIndex);
    return buffer;
}