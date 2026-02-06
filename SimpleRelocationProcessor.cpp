/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under terms of MIT License.
 * 
 * SimpleRelocationProcessor.cpp - Basic relocations for dynamic binaries
 */

#include "SimpleRelocationProcessor.h"
#include <cstdio>
#include <cstring>

SimpleRelocationProcessor::SimpleRelocationProcessor() {
    printf("[RELOC] SimpleRelocationProcessor initialized\n");
}

SimpleRelocationProcessor::~SimpleRelocationProcessor() {
    printf("[RELOC] SimpleRelocationProcessor destroyed\n");
}

bool SimpleRelocationProcessor::ProcessRelocations(uint8_t* programBase, uint32_t programSize, 
                                               Elf32_Rel* relocations, uint32_t relCount,
                                               DynamicLinker* linker) {
    if (!programBase || !relocations || !linker) {
        printf("[RELOC] Invalid parameters for relocation processing\n");
        return false;
    }

    printf("[RELOC] Processing %u relocations\n", relCount);

    for (uint32_t i = 0; i < relCount; i++) {
        Elf32_Rel* rel = &relocations[i];
        uint32_t relocType = ELF32_R_TYPE(rel->r_info);
        uint32_t symbolIndex = ELF32_R_SYM(rel->r_info);
        uint32_t relocOffset = rel->r_offset;

        printf("[RELOC] Processing relocation %u: type=%u, symbol=%u, offset=0x%08x\n", 
               i, relocType, symbolIndex, relocOffset);

        // Skip if offset is outside program bounds
        if (relocOffset >= programSize) {
            printf("[RELOC] Warning: Relocation offset 0x%08x outside program bounds\n", relocOffset);
            continue;
        }

        uint8_t* relocAddr = programBase + relocOffset;

        switch (relocType) {
            case R_386_JUMP_SLOT:
                // PLT entry - resolve symbol and patch address
                if (!ProcessJumpSlot(relocAddr, symbolIndex, linker)) {
                    printf("[RELOC] Failed to process JUMP_SLOT relocation\n");
                    return false;
                }
                break;

            case R_386_GLOB_DAT:
                // Global data reference
                if (!ProcessGlobalData(relocAddr, symbolIndex, linker)) {
                    printf("[RELOC] Failed to process GLOB_DAT relocation\n");
                    return false;
                }
                break;

            case R_386_RELATIVE:
                // Relative relocation - adjust based on program base
                ProcessRelative(relocAddr, programBase);
                break;

            case R_386_32:
                // 32-bit absolute relocation
                if (!ProcessAbsolute32(relocAddr, symbolIndex, linker)) {
                    printf("[RELOC] Failed to process 32-bit absolute relocation\n");
                    return false;
                }
                break;

            default:
                printf("[RELOC] Unsupported relocation type: %u\n", relocType);
                // Continue anyway for now
                break;
        }
    }

    printf("[RELOC] Relocation processing completed\n");
    return true;
}

bool SimpleRelocationProcessor::ProcessJumpSlot(uint8_t* relocAddr, uint32_t symbolIndex, DynamicLinker* linker) {
    // For now, just stub the jump slot to point to a stub function
    printf("[RELOC] Processing JUMP_SLOT for symbol index %u\n", symbolIndex);
    
    // Get symbol name (simplified - we don't have access to symbol table here)
    const char* symbolName = GetSymbolName(symbolIndex);
    if (!symbolName) {
        printf("[RELOC] Unknown symbol index %u, using stub\n", symbolIndex);
        // Write a simple stub that returns
        uint32_t stubAddr = 0xDEADBEEF; // Placeholder
        memcpy(relocAddr, &stubAddr, sizeof(uint32_t));
        return true;
    }

    // Try to resolve symbol through dynamic linker
    void* symbolAddr = nullptr;
    if (linker->FindSymbol(symbolName, &symbolAddr, nullptr)) {
        printf("[RELOC] Resolved symbol '%s' to %p\n", symbolName, symbolAddr);
        memcpy(relocAddr, &symbolAddr, sizeof(uint32_t));
        return true;
    }

    printf("[RELOC] Symbol '%s' not found, using stub\n", symbolName);
    // Write a stub address
    uint32_t stubAddr = 0xCAFEF00D; // Different placeholder
    memcpy(relocAddr, &stubAddr, sizeof(uint32_t));
    return true;
}

bool SimpleRelocationProcessor::ProcessGlobalData(uint8_t* relocAddr, uint32_t symbolIndex, DynamicLinker* linker) {
    printf("[RELOC] Processing GLOB_DAT for symbol index %u\n", symbolIndex);
    
    const char* symbolName = GetSymbolName(symbolIndex);
    if (!symbolName) {
        printf("[RELOC] Unknown symbol index %u\n", symbolIndex);
        return false;
    }

    void* symbolAddr = nullptr;
    if (linker->FindSymbol(symbolName, &symbolAddr, nullptr)) {
        printf("[RELOC] Resolved global data '%s' to %p\n", symbolName, symbolAddr);
        memcpy(relocAddr, &symbolAddr, sizeof(uint32_t));
        return true;
    }

    printf("[RELOC] Global data symbol '%s' not found\n", symbolName);
    return false;
}

void SimpleRelocationProcessor::ProcessRelative(uint8_t* relocAddr, uint8_t* programBase) {
    printf("[RELOC] Processing RELATIVE relocation\n");
    
    // Add program base to the existing value
    uint32_t existingValue;
    memcpy(&existingValue, relocAddr, sizeof(uint32_t));
    
    uint32_t newValue = existingValue + (uint32_t)(uintptr_t)programBase;
    memcpy(relocAddr, &newValue, sizeof(uint32_t));
    
    printf("[RELOC] RELATIVE: 0x%08x -> 0x%08x (base=%p)\n", 
           existingValue, newValue, programBase);
}

bool SimpleRelocationProcessor::ProcessAbsolute32(uint8_t* relocAddr, uint32_t symbolIndex, DynamicLinker* linker) {
    printf("[RELOC] Processing 32-bit absolute relocation for symbol index %u\n", symbolIndex);
    
    const char* symbolName = GetSymbolName(symbolIndex);
    if (!symbolName) {
        printf("[RELOC] Unknown symbol index %u\n", symbolIndex);
        return false;
    }

    void* symbolAddr = nullptr;
    if (linker->FindSymbol(symbolName, &symbolAddr, nullptr)) {
        printf("[RELOC] Resolved absolute symbol '%s' to %p\n", symbolName, symbolAddr);
        memcpy(relocAddr, &symbolAddr, sizeof(uint32_t));
        return true;
    }

    printf("[RELOC] Absolute symbol '%s' not found\n", symbolName);
    return false;
}

const char* SimpleRelocationProcessor::GetSymbolName(uint32_t symbolIndex) {
    // Simplified symbol name resolution
    // In a real implementation, this would look up in the symbol table
    switch (symbolIndex) {
        case 0: return "write";
        case 1: return "read";
        case 2: return "open";
        case 3: return "close";
        case 4: return "printf";
        case 5: return "malloc";
        case 6: return "free";
        case 7: return "exit";
        case 8: return "strcpy";
        case 9: return "strlen";
        default: 
            // Generate a fake name for unknown symbols
            static char fakeName[32];
            snprintf(fakeName, sizeof(fakeName), "unknown_symbol_%u", symbolIndex);
            return fakeName;
    }
}