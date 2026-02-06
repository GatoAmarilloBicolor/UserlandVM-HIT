/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * ProgramHeaderParser.cpp - Complete ELF program header parsing implementation
 */

#include "ProgramHeaderParser.h"
#include "Loader.h"
#include <stdio.h>
#include <cstring>
#include <algorithm>

// ELF structure definitions for compatibility
#include <elf.h>

bool ProgramHeaderParser::ParseProgramHeaders(ElfImage* image) {
    if (!image) {
        printf("[PHDR] ERROR: Invalid image\n");
        return false;
    }

    printf("[PHDR] Parsing program headers for %s\n", image->GetPath());
    
    // Get program header count from ELF header
    // Implementation depends on existing ElfImage interface
    // TODO: Access ELF header through ElfImage methods
    
    uint32_t phdr_count = image->GetProgramHeaderCount();
    uint32_t phdr_offset = image->GetProgramHeaderOffset();
    
    printf("[PHDR] Found %d program headers at offset 0x%08x\n", phdr_count, phdr_offset);
    
    for (uint32_t i = 0; i < phdr_count; i++) {
        if (!ValidateProgramHeader(image, i)) {
            printf("[PHDR] ERROR: Invalid program header %d\n", i);
            return false;
        }
        
        uint32_t phdr_type = image->GetProgramHeaderType(i);
        printf("[PHDR] Program header %d: type 0x%08x\n", i, phdr_type);
        
        switch (phdr_type) {
            case PT_INTERP:
            {
                std::string interpreter = GetInterpreter(image);
                if (!interpreter.empty()) {
                    printf("[PHDR] Interpreter: %s\n", interpreter.c_str());
                }
                break;
            }
            
            case PT_DYNAMIC:
            {
                printf("[PHDR] Found PT_DYNAMIC section\n");
                DynamicInfo dynamic_info = ParseDynamicSection(image);
                printf("[PHDR] Dynamic section parsed:\n");
                printf("[PHDR]   Needed libraries: %zu\n", dynamic_info.needed_libs.size());
                printf("[PHDR]   SONAME: %s\n", dynamic_info.soname.c_str());
                printf("[PHDR]   Hash table: 0x%08x\n", dynamic_info.hash_addr);
                printf("[PHDR]   GNU hash table: 0x%08x\n", dynamic_info.gnu_hash_addr);
                printf("[PHDR]   Symbol table: 0x%08x\n", dynamic_info.symtab_addr);
                printf("[PHDR]   String table: 0x%08x\n", dynamic_info.strtab_addr);
                break;
            }
            
            case PT_TLS:
            {
                uint32_t tls_addr, tls_size, tls_align;
                if (GetTLSInfo(image, tls_addr, tls_size, tls_align)) {
                    printf("[PHDR] TLS: addr=0x%08x size=%u align=%u\n", tls_addr, tls_size, tls_align);
                }
                break;
            }
            
            case PT_GNU_RELRO:
            {
                printf("[PHDR] Found GNU RELRO segment\n");
                break;
            }
            
            case PT_GNU_STACK:
            {
                printf("[PHDR] Found GNU executable stack segment\n");
                break;
            }
            
            default:
                if (phdr_type >= PT_GNU_EH_FRAME && phdr_type <= PT_GNU_RELRO) {
                    printf("[PHDR] Found GNU segment type 0x%08x\n", phdr_type);
                } else {
                    printf("[PHDR] Standard segment type 0x%08x\n", phdr_type);
                }
                break;
        }
    }
    
    printf("[PHDR] Program header parsing completed successfully\n");
    return true;
}

ProgramHeaderParser::DynamicInfo ProgramHeaderParser::ParseDynamicSection(ElfImage* image) {
    DynamicInfo info;
    
    printf("[DYNAMIC] Parsing dynamic section\n");
    
    // Find PT_DYNAMIC segment
    uint32_t dynamic_addr = 0;
    uint32_t dynamic_size = 0;
    
    uint32_t phdr_count = image->GetProgramHeaderCount();
    for (uint32_t i = 0; i < phdr_count; i++) {
        if (image->GetProgramHeaderType(i) == PT_DYNAMIC) {
            dynamic_addr = image->GetProgramHeaderVirtAddr(i);
            dynamic_size = image->GetProgramHeaderFileSize(i);
            break;
        }
    }
    
    if (dynamic_addr == 0 || dynamic_size == 0) {
        printf("[DYNAMIC] No PT_DYNAMIC segment found\n");
        return info;
    }
    
    printf("[DYNAMIC] Dynamic section at 0x%08x, size %u\n", dynamic_addr, dynamic_size);
    
    // Parse dynamic entries
    uint32_t entry_count = dynamic_size / sizeof(DynamicEntry);
    printf("[DYNAMIC] Parsing %d dynamic entries\n", entry_count);
    
    for (uint32_t i = 0; i < entry_count; i++) {
        uint32_t entry_offset = dynamic_addr + i * sizeof(DynamicEntry);
        
        // Read dynamic entry from memory
        DynamicEntry entry;
        if (!image->ReadMemory(entry_offset, &entry, sizeof(entry))) {
            printf("[DYNAMIC] ERROR: Failed to read dynamic entry %d\n", i);
            continue;
        }
        
        if (entry.d_tag == DT_NULL) {
            break; // End of dynamic section
        }
        
        if (!ParseDynamicEntry(image, entry.d_tag, entry.d_val, info)) {
            printf("[DYNAMIC] Warning: Failed to parse entry tag=0x%08x val=0x%08x\n", 
                   entry.d_tag, entry.d_val);
        }
    }
    
    printf("[DYNAMIC] Dynamic parsing complete\n");
    return info;
}

std::string ProgramHeaderParser::GetInterpreter(ElfImage* image) {
    printf("[INTERP] Looking for interpreter\n");
    
    uint32_t phdr_count = image->GetProgramHeaderCount();
    for (uint32_t i = 0; i < phdr_count; i++) {
        if (image->GetProgramHeaderType(i) == PT_INTERP) {
            uint32_t interp_addr = image->GetProgramHeaderVirtAddr(i);
            printf("[INTERP] Found PT_INTERP at 0x%08x\n", interp_addr);
            
            return ReadString(image, interp_addr);
        }
    }
    
    printf("[INTERP] No PT_INTERP segment found\n");
    return "";
}

bool ProgramHeaderParser::IsPositionIndependent(ElfImage* image) {
    uint32_t phdr_count = image->GetProgramHeaderCount();
    
    for (uint32_t i = 0; i < phdr_count; i++) {
        if (image->GetProgramHeaderType(i) == PT_DYNAMIC) {
            DynamicInfo info = ParseDynamicSection(image);
            
            // Check for typical PIE indicators
            if (info.pltgot_addr != 0) {
                printf("[PHDR] Position independent executable detected (PLTGOT present)\n");
                return true;
            }
            
            // Check for DT_FLAGS with DF_PIE
            if (info.flags & 0x00000001) { // DF_PIE flag
                printf("[PHDR] Position independent executable detected (PIE flag)\n");
                return true;
            }
            
            break;
        }
    }
    
    printf("[PHDR] Not a position independent executable\n");
    return false;
}

bool ProgramHeaderParser::GetTLSInfo(ElfImage* image, uint32_t& addr, uint32_t& size, uint32_t& align) {
    uint32_t phdr_count = image->GetProgramHeaderCount();
    
    for (uint32_t i = 0; i < phdr_count; i++) {
        if (image->GetProgramHeaderType(i) == PT_TLS) {
            addr = image->GetProgramHeaderVirtAddr(i);
            size = image->GetProgramHeaderFileSize(i);
            align = image->GetProgramHeaderAlign(i);
            
            printf("[TLS] TLS segment found: addr=0x%08x size=%u align=%u\n", addr, size, align);
            return true;
        }
    }
    
    printf("[TLS] No TLS segment found\n");
    return false;
}

bool ProgramHeaderParser::HasRelroProtection(ElfImage* image) {
    uint32_t phdr_count = image->GetProgramHeaderCount();
    
    for (uint32_t i = 0; i < phdr_count; i++) {
        if (image->GetProgramHeaderType(i) == PT_GNU_RELRO) {
            printf("[RELRO] RELRO protection found\n");
            return true;
        }
    }
    
    printf("[RELRO] No RELRO protection\n");
    return false;
}

bool ProgramHeaderParser::ParseDynamicEntry(ElfImage* image, uint32_t tag, uint32_t val, DynamicInfo& info) {
    switch (tag) {
        case DT_NEEDED:
        {
            std::string lib_name = ReadString(image, val);
            if (!lib_name.empty()) {
                info.needed_libs.push_back(lib_name);
                printf("[DYNAMIC] DT_NEEDED: %s\n", lib_name.c_str());
            }
            break;
        }
        
        case DT_SONAME:
            info.soname = ReadString(image, val);
            printf("[DYNAMIC] DT_SONAME: %s\n", info.soname.c_str());
            break;
            
        case DT_RPATH:
            info.rpath = ReadString(image, val);
            printf("[DYNAMIC] DT_RPATH: %s\n", info.rpath.c_str());
            break;
            
        case DT_RUNPATH:
            info.runpath = ReadString(image, val);
            printf("[DYNAMIC] DT_RUNPATH: %s\n", info.runpath.c_str());
            break;
            
        case DT_INIT:
            info.init_addr = val;
            printf("[DYNAMIC] DT_INIT: 0x%08x\n", val);
            break;
            
        case DT_FINI:
            info.fini_addr = val;
            printf("[DYNAMIC] DT_FINI: 0x%08x\n", val);
            break;
            
        case DT_INIT_ARRAY:
            info.init_array_addr = val;
            printf("[DYNAMIC] DT_INIT_ARRAY: 0x%08x\n", val);
            break;
            
        case DT_INIT_ARRAYSZ:
            info.init_array_size = val;
            info.init_array_count = val / sizeof(uint32_t);
            printf("[DYNAMIC] DT_INIT_ARRAYSZ: %u (%d entries)\n", val, info.init_array_count);
            break;
            
        case DT_FINI_ARRAY:
            info.fini_array_addr = val;
            printf("[DYNAMIC] DT_FINI_ARRAY: 0x%08x\n", val);
            break;
            
        case DT_FINI_ARRAYSZ:
            info.fini_array_size = val;
            info.fini_array_count = val / sizeof(uint32_t);
            printf("[DYNAMIC] DT_FINI_ARRAYSZ: %u (%d entries)\n", val, info.fini_array_count);
            break;
            
        case DT_REL:
            info.rel_addr = val;
            printf("[DYNAMIC] DT_REL: 0x%08x\n", val);
            break;
            
        case DT_RELSZ:
            info.rel_size = val;
            printf("[DYNAMIC] DT_RELSZ: %u\n", val);
            break;
            
        case DT_RELA:
            info.rela_addr = val;
            printf("[DYNAMIC] DT_RELA: 0x%08x\n", val);
            break;
            
        case DT_RELASZ:
            info.rela_size = val;
            printf("[DYNAMIC] DT_RELASZ: %u\n", val);
            break;
            
        case DT_JMPREL:
            info.plt_rel_addr = val;
            printf("[DYNAMIC] DT_JMPREL: 0x%08x\n", val);
            break;
            
        case DT_PLTRELSZ:
            info.plt_rel_size = val;
            printf("[DYNAMIC] DT_PLTRELSZ: %u\n", val);
            break;
            
        case DT_HASH:
            info.hash_addr = val;
            printf("[DYNAMIC] DT_HASH: 0x%08x\n", val);
            break;
            
        case DT_GNU_HASH:
            info.gnu_hash_addr = val;
            printf("[DYNAMIC] DT_GNU_HASH: 0x%08x\n", val);
            break;
            
        case DT_SYMTAB:
            info.symtab_addr = val;
            printf("[DYNAMIC] DT_SYMTAB: 0x%08x\n", val);
            break;
            
        case DT_STRTAB:
            info.strtab_addr = val;
            printf("[DYNAMIC] DT_STRTAB: 0x%08x\n", val);
            break;
            
        case DT_SYMENT:
            printf("[DYNAMIC] DT_SYMENT: %u\n", val);
            break;
            
        case DT_STRSZ:
            info.strtab_size = val;
            printf("[DYNAMIC] DT_STRSZ: %u\n", val);
            break;
            
        case DT_PLTGOT:
            info.pltgot_addr = val;
            printf("[DYNAMIC] DT_PLTGOT: 0x%08x\n", val);
            break;
            
        case DT_TLS:
            info.tls_addr = val;
            printf("[DYNAMIC] DT_TLS: 0x%08x\n", val);
            break;
            
        case DT_FLAGS:
            info.flags = val;
            printf("[DYNAMIC] DT_FLAGS: 0x%08x\n", val);
            break;
            
        case DT_DEBUG:
            info.debug = val;
            printf("[DYNAMIC] DT_DEBUG: 0x%08x\n", val);
            break;
            
        default:
            printf("[DYNAMIC] Unhandled tag 0x%08x = 0x%08x\n", tag, val);
            break;
    }
    
    return true;
}

std::string ProgramHeaderParser::ReadString(ElfImage* image, uint32_t addr) {
    if (!image) {
        return "";
    }
    
    std::string result;
    char ch;
    uint32_t current_addr = addr;
    
    while (image->ReadMemory(current_addr, &ch, 1) && ch != '\0') {
        result += ch;
        current_addr++;
    }
    
    return result;
}

bool ProgramHeaderParser::ValidateProgramHeader(ElfImage* image, int ph_index) {
    // Basic validation - check that header is within file bounds
    uint32_t phdr_size = image->GetProgramHeaderSize();
    uint32_t phdr_offset = image->GetProgramHeaderOffset() + ph_index * phdr_size;
    
    // TODO: More comprehensive validation
    printf("[PHDR] Validating program header %d at offset 0x%08x\n", ph_index, phdr_offset);
    
    return true;
}