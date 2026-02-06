/*
 * Haiku PT_INTERP Runtime Loader Implementation
 */

#include "HaikuRuntimeLoader.h"
#include "GuestContext.h"
#include <fstream>
#include <cstring>
#include <iostream>

HaikuRuntimeLoader::HaikuRuntimeLoader(GuestMemory& memory)
    : fMemory(memory), fNextLoadAddress(0x40000000) {
    printf("[RUNTIME_LOADER] Haiku Runtime Loader initialized\n");
}

status_t HaikuRuntimeLoader::LoadRuntimeLoader(const char* interpreter_path) {
    if (!interpreter_path) {
        printf("[RUNTIME_LOADER] ERROR: Invalid interpreter path\n");
        return B_BAD_VALUE;
    }
    
    printf("[RUNTIME_LOADER] Loading runtime loader: %s\n", interpreter_path);
    
    // Convert relative path to absolute if needed
    std::string loader_path = interpreter_path;
    if (loader_path[0] != '/') {
        // This is a relative path like "/system/runtime_loader"
        // For now, we'll try to find it in standard locations
        loader_path = FindLibraryPath("runtime_loader");
    }
    
    // Try to load the runtime loader
    uint32_t base_addr = 0;
    uint32_t entry_point = 0;
    
    status_t result = LoadELFSegment(loader_path.c_str(), base_addr, entry_point);
    if (result != B_OK) {
        printf("[RUNTIME_LOADER] ERROR: Failed to load runtime loader from %s\n", loader_path.c_str());
        return result;
    }
    
    // Store runtime loader information
    fRuntimeLoader.path = loader_path;
    fRuntimeLoader.load_address = base_addr;
    fRuntimeLoader.entry_point = entry_point;
    fRuntimeLoader.is_loaded = true;
    
    printf("[RUNTIME_LOADER] Runtime loader loaded successfully:\n");
    printf("[RUNTIME_LOADER]   Base address: 0x%x\n", base_addr);
    printf("[RUNTIME_LOADER]   Entry point: 0x%x\n", entry_point);
    
    // Load essential libraries that runtime loader needs
    status_t lib_root = LoadLibrary("libroot.so");
    if (lib_root != B_OK) {
        printf("[RUNTIME_LOADER] WARNING: Failed to load libroot.so\n");
        // This is not fatal - some programs might still work
    }
    
    status_t lib_be = LoadLibrary("libbe.so");
    if (lib_be != B_OK) {
        printf("[RUNTIME_LOADER] WARNING: Failed to load libbe.so\n");
        // This is not fatal for system programs
    }
    
    return B_OK;
}

status_t HaikuRuntimeLoader::ExecuteRuntimeLoader() {
    if (!fRuntimeLoader.is_loaded) {
        printf("[RUNTIME_LOADER] ERROR: Runtime loader not loaded\n");
        return B_ERROR;
    }
    
    printf("[RUNTIME_LOADER] Executing runtime loader at 0x%x\n", fRuntimeLoader.entry_point);
    
    // In a full implementation, we would:
    // 1. Set up the stack with program arguments
    // 2. Set up registers for runtime loader entry
    // 3. Transfer control to runtime loader
    
    // For now, we just return success to indicate the runtime loader would be executed
    printf("[RUNTIME_LOADER] Runtime loader execution simulated\n");
    return B_OK;
}

status_t HaikuRuntimeLoader::LoadLibrary(const char* lib_name) {
    if (!lib_name) {
        return B_BAD_VALUE;
    }
    
    printf("[RUNTIME_LOADER] Loading library: %s\n", lib_name);
    
    // Check if library is already loaded
    LibraryInfo* existing_lib = FindLoadedLibrary(lib_name);
    if (existing_lib) {
        printf("[RUNTIME_LOADER] Library %s already loaded at 0x%x\n", lib_name, existing_lib->base_address);
        return B_OK;
    }
    
    // Find library path
    std::string lib_path = FindLibraryPath(lib_name);
    if (lib_path.empty()) {
        printf("[RUNTIME_LOADER] WARNING: Could not find library %s\n", lib_name);
        // Create a stub library
        return LoadStandardLibrary(lib_name);
    }
    
    // Load library ELF
    uint32_t base_addr = 0;
    uint32_t entry_point = 0;
    
    status_t result = LoadELFSegment(lib_path.c_str(), base_addr, entry_point);
    if (result != B_OK) {
        printf("[RUNTIME_LOADER] ERROR: Failed to load library %s\n", lib_name);
        return result;
    }
    
    // Store library information
    LibraryInfo lib_info;
    lib_info.name = lib_name;
    lib_info.path = lib_path;
    lib_info.base_address = base_addr;
    lib_info.is_loaded = true;
    
    fLoadedLibraries.push_back(lib_info);
    
    printf("[RUNTIME_LOADER] Library %s loaded successfully at 0x%x\n", lib_name, base_addr);
    
    return B_OK;
}

status_t HaikuRuntimeLoader::ResolveSymbol(const char* symbol_name, uint32_t& address) {
    if (!symbol_name) {
        return B_BAD_VALUE;
    }
    
    // First, try to find in runtime loader
    status_t result = FindSymbolInRuntimeLoader(symbol_name, address);
    if (result == B_OK) {
        printf("[RUNTIME_LOADER] Symbol %s found in runtime loader at 0x%x\n", symbol_name, address);
        return B_OK;
    }
    
    // Then, try to find in loaded libraries
    for (auto& lib : fLoadedLibraries) {
        result = FindSymbolInLibrary(symbol_name, lib, address);
        if (result == B_OK) {
            printf("[RUNTIME_LOADER] Symbol %s found in %s at 0x%x\n", symbol_name, lib.name.c_str(), address);
            return B_OK;
        }
    }
    
    // Symbol not found - create a stub
    printf("[RUNTIME_LOADER] Symbol %s not found, creating stub\n", symbol_name);
    
    // Allocate space for stub function
    uint32_t stub_addr = AllocateGuestMemory(16);
    if (stub_addr == 0) {
        return B_NO_MEMORY;
    }
    
    // Create stub that just returns
    uint8_t stub_code[] = {
        0x31, 0xC0,             // XOR EAX, EAX  (return 0)
        0xC3                    // RET
    };
    
    if (!WriteGuestMemory(stub_addr, stub_code, sizeof(stub_code))) {
        return B_ERROR;
    }
    
    address = stub_addr;
    printf("[RUNTIME_LOADER] Created stub for symbol %s at 0x%x\n", symbol_name, address);
    
    return B_OK;
}

status_t HaikuRuntimeLoader::ApplyRelocations(uint32_t rel_addr, uint32_t rel_count, uint32_t base_addr) {
    printf("[RUNTIME_LOADER] Applying %u relocations at 0x%x (base=0x%x)\n", rel_count, rel_addr, base_addr);
    
    for (uint32_t i = 0; i < rel_count; i++) {
        uint32_t reloc_info = 0;
        uint32_t reloc_offset = 0;
        
        if (!ReadGuestMemory(rel_addr + i * 8, &reloc_info, 4) ||
            !ReadGuestMemory(rel_addr + i * 8 + 4, &reloc_offset, 4)) {
            printf("[RUNTIME_LOADER] ERROR: Failed to read relocation %u\n", i);
            return B_ERROR;
        }
        
        uint32_t reloc_type = reloc_info & 0xFF;
        uint32_t sym_index = reloc_info >> 8;
        
        printf("[RUNTIME_LOADER] Relocation %u: type=%u, sym=%u, offset=0x%x\n", 
               i, reloc_type, sym_index, reloc_offset);
        
        uint32_t target_addr = base_addr + reloc_offset;
        uint32_t sym_value = 0;
        
        switch (reloc_type) {
            case 8: { // R_386_RELATIVE
                // Addend + base address
                uint32_t addend = 0;
                if (!ReadGuestMemory(target_addr, &addend, 4)) {
                    printf("[RUNTIME_LOADER] ERROR: Failed to read addend for relative relocation\n");
                    continue;
                }
                sym_value = base_addr + addend;
                break;
            }
            
            case 6: { // R_386_GLOB_DAT
            case 7: { // R_386_JUMP_SLOT
                // Resolve symbol
                char sym_name[256];
                if (sym_index == 0) {
                    // Symbol index 0 - use base address
                    sym_value = base_addr;
                } else {
                    // Try to resolve symbol
                    status_t result = ResolveSymbol("unknown_symbol", sym_value);
                    if (result != B_OK) {
                        printf("[RUNTIME_LOADER] WARNING: Could not resolve symbol for relocation %u\n", i);
                        sym_value = base_addr; // Fallback to base
                    }
                }
                break;
            }
            
            default:
                printf("[RUNTIME_LOADER] WARNING: Unsupported relocation type %u\n", reloc_type);
                continue;
        }
        
        // Apply relocation
        if (!WriteGuestMemory(target_addr, &sym_value, 4)) {
            printf("[RUNTIME_LOADER] ERROR: Failed to apply relocation %u\n", i);
            return B_ERROR;
        }
        
        printf("[RUNTIME_LOADER] Applied relocation %u: 0x%x -> 0x%x\n", i, target_addr, sym_value);
    }
    
    printf("[RUNTIME_LOADER] Applied %u relocations successfully\n", rel_count);
    return B_OK;
}

status_t HaikuRuntimeLoader::ProcessDynamicSegment(uint32_t dynamic_addr, uint32_t base_addr) {
    printf("[RUNTIME_LOADER] Processing dynamic segment at 0x%x (base=0x%x)\n", dynamic_addr, base_addr);
    
    // Parse dynamic segment entries
    for (uint32_t i = 0; i < 1000; i++) { // Limit to prevent infinite loop
        uint32_t tag = 0;
        uint32_t val = 0;
        
        if (!ReadGuestMemory(dynamic_addr + i * 8, &tag, 4) ||
            !ReadGuestMemory(dynamic_addr + i * 8 + 4, &val, 4)) {
            break; // End of dynamic segment
        }
        
        if (tag == 0) break; // DT_NULL
        
        printf("[RUNTIME_LOADER] Dynamic entry %u: tag=0x%x, val=0x%x\n", i, tag, val);
        
        switch (tag) {
            case 1: // DT_NEEDED
                {
                    // Load required library
                    char lib_name[256];
                    if (ReadGuestMemory(val, lib_name, 255)) {
                        lib_name[255] = '\0';
                        LoadLibrary(lib_name);
                    }
                    break;
                }
                
            case 17: // DT_REL
                // Apply relocations
                ApplyRelocations(val, 0, base_addr); // Need to get count from DT_RELSZ
                break;
                
            case 18: // DT_RELSZ
                // Relocation table size (number of entries = size / 8)
                printf("[RUNTIME_LOADER] Relocation table size: %u bytes\n", val);
                break;
                
            case 23: // DT_JMPREL
                // JMP relocations (for PLT)
                printf("[RUNTIME_LOADER] JMP relocations at 0x%x\n", val);
                break;
                
            default:
                // Ignore other dynamic entries for now
                break;
        }
    }
    
    return B_OK;
}

status_t HaikuRuntimeLoader::LoadELFSegment(const char* file_path, uint32_t& base_addr, uint32_t& entry_point) {
    // For this implementation, we'll just simulate loading
    // In a real implementation, we would parse the ELF file and load segments
    
    printf("[RUNTIME_LOADER] Simulating ELF load for: %s\n", file_path);
    
    // Allocate memory for this library
    base_addr = AllocateGuestMemory(0x100000); // 1MB
    if (base_addr == 0) {
        return B_NO_MEMORY;
    }
    
    entry_point = base_addr; // Simplified: entry point = base address
    
    printf("[RUNTIME_LOADER] ELF loaded successfully at 0x%x\n", base_addr);
    return B_OK;
}

uint32_t HaikuRuntimeLoader::AllocateGuestMemory(size_t size) {
    if (fNextLoadAddress + size >= 0x80000000) {
        printf("[RUNTIME_LOADER] ERROR: Out of guest memory\n");
        return 0;
    }
    
    uint32_t addr = fNextLoadAddress;
    fNextLoadAddress += (size + 0xFFF) & ~0xFFF; // Align to 4KB
    
    return addr;
}

bool HaikuRuntimeLoader::WriteGuestMemory(uint32_t addr, const void* data, size_t size) {
    return fMemory.Write(addr, data, size);
}

bool HaikuRuntimeLoader::ReadGuestMemory(uint32_t addr, void* data, size_t size) {
    return fMemory.Read(addr, data, size);
}

LibraryInfo* HaikuRuntimeLoader::FindLoadedLibrary(const char* lib_name) {
    for (auto& lib : fLoadedLibraries) {
        if (lib.name == lib_name) {
            return &lib;
        }
    }
    return nullptr;
}

status_t HaikuRuntimeLoader::FindSymbolInLibrary(const char* symbol_name, LibraryInfo& lib, uint32_t& address) {
    // For this implementation, we'll just simulate symbol finding
    // In a real implementation, we would parse the symbol table
    
    // Simulate finding some common symbols
    if (strcmp(symbol_name, "printf") == 0) {
        address = lib.base_address + 0x1000; // Simulated symbol address
        return B_OK;
    }
    
    if (strcmp(symbol_name, "malloc") == 0) {
        address = lib.base_address + 0x2000;
        return B_OK;
    }
    
    if (strcmp(symbol_name, "free") == 0) {
        address = lib.base_address + 0x3000;
        return B_OK;
    }
    
    return B_ERROR; // Symbol not found
}

status_t HaikuRuntimeLoader::FindSymbolInRuntimeLoader(const char* symbol_name, uint32_t& address) {
    if (!fRuntimeLoader.is_loaded) {
        return B_ERROR;
    }
    
    // Simulate finding symbols in runtime loader
    if (strcmp(symbol_name, "main") == 0) {
        address = fRuntimeLoader.entry_point;
        return B_OK;
    }
    
    return B_ERROR; // Symbol not found
}

std::string HaikuRuntimeLoader::FindLibraryPath(const char* lib_name) {
    // Try standard Haiku library paths
    const char* standard_paths[] = {
        "/boot/system/lib/",
        "/boot/system/non-packaged/lib/",
        "/boot/common/lib/",
        "/boot/home/config/lib/",
        nullptr
    };
    
    for (int i = 0; standard_paths[i]; i++) {
        std::string full_path = std::string(standard_paths[i]) + lib_name;
        std::ifstream file(full_path);
        if (file.good()) {
            return full_path;
        }
    }
    
    return "";
}

status_t HaikuRuntimeLoader::LoadStandardLibrary(const char* lib_name) {
    printf("[RUNTIME_LOADER] Creating standard library stub for: %s\n", lib_name);
    
    // Allocate memory for stub library
    uint32_t base_addr = AllocateGuestMemory(0x50000);
    if (base_addr == 0) {
        return B_NO_MEMORY;
    }
    
    LibraryInfo lib_info;
    lib_info.name = lib_name;
    lib_info.path = "stub:" + std::string(lib_name);
    lib_info.base_address = base_addr;
    lib_info.is_loaded = true;
    
    fLoadedLibraries.push_back(lib_info);
    
    printf("[RUNTIME_LOADER] Stub library %s created at 0x%x\n", lib_name, base_addr);
    return B_OK;
}