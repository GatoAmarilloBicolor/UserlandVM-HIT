/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under terms of MIT License.
 * 
 * DynamicLinker.cpp - Simplified dynamic linking for UserlandVM-HIT
 */

#include "DynamicLinker.h"
#include <cstring>
#include <algorithm>
#include <vector>
#include <map>
#include <cstdio>

DynamicLinker::DynamicLinker() {
    // Initialize standard Haiku search paths for our sysroot
    fSearchPaths = {
        "sysroot/haiku32/lib",
        "sysroot/haiku32/system/lib",
        "sysroot/haiku32/boot/system/lib"
    };
    printf("[DYNAMIC] DynamicLinker initialized with %zu search paths\n", fSearchPaths.size());
}

DynamicLinker::~DynamicLinker() {
    // Cleanup all loaded libraries
    for (auto& [name, info] : fLibraries) {
        if (info.elf_image) {
            delete info.elf_image;
        }
    }
    fLibraries.clear();
    printf("[DYNAMIC] DynamicLinker destroyed\n");
}

ElfImage* DynamicLinker::LoadLibrary(const char* path) {
    if (!path) return nullptr;

    // Check if already loaded
    std::string lib_name = GetLibraryName(path);
    auto it = fLibraries.find(lib_name);
    if (it != fLibraries.end() && it->second.loaded) {
        printf("[DYNAMIC] Library %s already loaded\n", lib_name.c_str());
        return it->second.elf_image;
    }

    // Load the ELF file using our ELFImage class
    ElfImage* image = ElfImage::Load(path);
    if (!image) {
        printf("[DYNAMIC] Failed to load library: %s\n", path);
        return nullptr;
    }

    // Store library info
    LibraryInfo info;
    info.elf_image = image;
    info.handle = nullptr;  // We don't use dlopen for our ELF loading
    info.base_address = nullptr;  // Will be set during relocation
    info.size = 0;  // Will be calculated
    info.loaded = true;
    info.reference_count = 1;
    
    fLibraries[lib_name] = info;
    printf("[DYNAMIC] Successfully loaded library: %s\n", path);
    
    return image;
}

bool DynamicLinker::FindSymbol(const char* name, void** address, size_t* size) {
    if (!name || !address) return false;
    
    // Search in all loaded libraries
    for (auto& [lib_name, info] : fLibraries) {
        if (!info.elf_image || !info.loaded) continue;
        
        // Search in symbol table
        const Elf32_Sym* symbols = info.elf_image->GetSymbolTable();
        const char* str_table = info.elf_image->GetStringTable();
        uint32_t symbol_count = info.elf_image->GetSymbolCount();
        
        for (uint32_t i = 0; i < symbol_count; i++) {
            const char* symbol_name = str_table + symbols[i].st_name;
            
            if (strcmp(symbol_name, name) == 0) {
                *address = (void*)(uintptr_t)symbols[i].st_value;
                if (size) *size = symbols[i].st_size;
                
                printf("[DYNAMIC] Found symbol %s in library %s at address %p\n", 
                       name, lib_name.c_str(), *address);
                return true;
            }
        }
    }
    
    printf("[DYNAMIC] Symbol not found: %s\n", name);
    return false;
}

ElfImage* DynamicLinker::GetLibrary(const char* name) {
    if (!name) return nullptr;
    
    auto it = fLibraries.find(name);
    if (it != fLibraries.end() && it->second.loaded) {
        return it->second.elf_image;
    }
    
    return nullptr;
}

void DynamicLinker::SetSearchPath(const char* path) {
    fSearchPaths.clear();
    fSearchPaths.push_back(std::string(path));
    printf("[DYNAMIC] Set search path to: %s\n", path);
}

void DynamicLinker::AddLibrary(const char* name, ElfImage* image) {
    if (!name || !image) return;
    
    LibraryInfo info;
    info.elf_image = image;
    info.handle = nullptr;
    info.base_address = nullptr;
    info.size = 0;
    info.loaded = true;
    info.reference_count = 1;
    
    fLibraries[std::string(name)] = info;
    printf("[DYNAMIC] Added library: %s\n", name);
}

bool DynamicLinker::LoadDynamicDependencies(const char* program_path) {
    if (!program_path) return false;
    
    printf("[DYNAMIC] Loading dependencies for: %s\n", program_path);
    
    // Load program ELF first
    ElfImage* program = ElfImage::Load(program_path);
    if (!program) {
        printf("[DYNAMIC] Failed to load program: %s\n", program_path);
        return false;
    }
    
    // Get dynamic section
    const Elf32_Dyn* dynamic = program->GetDynamicSection();
    if (!dynamic) {
        printf("[DYNAMIC] No dynamic section found in program\n");
        return false;
    }
    
    // Load required libraries (DT_NEEDED entries)
    const char* str_table = program->GetDynamicStringTable();
    if (!str_table) {
        printf("[DYNAMIC] No dynamic string table found\n");
        return false;
    }
    
    for (uint32_t i = 0; dynamic[i].d_tag != DT_NULL; i++) {
        if (dynamic[i].d_tag == DT_NEEDED) {
            const char* lib_name = str_table + dynamic[i].d_val;
            
            // Try to find library in search paths
            bool loaded = false;
            for (const std::string& search_path : fSearchPaths) {
                std::string full_path = search_path + "/" + std::string(lib_name);
                
                if (LoadLibrary(full_path.c_str())) {
                    loaded = true;
                    break;
                }
            }
            
            if (!loaded) {
                printf("[DYNAMIC] Failed to load required library: %s\n", lib_name);
                return false;
            }
        }
    }
    
    printf("[DYNAMIC] Successfully loaded all dependencies\n");
    return true;
}

std::vector<std::string> DynamicLinker::GetDynamicDependencies(const char* program_path) {
    std::vector<std::string> dependencies;
    
    ElfImage* program = ElfImage::Load(program_path);
    if (!program) {
        return dependencies;
    }
    
    const Elf32_Dyn* dynamic = program->GetDynamicSection();
    const char* str_table = program->GetDynamicStringTable();
    
    if (dynamic && str_table) {
        for (uint32_t i = 0; dynamic[i].d_tag != DT_NULL; i++) {
            if (dynamic[i].d_tag == DT_NEEDED) {
                dependencies.push_back(std::string(str_table + dynamic[i].d_val));
            }
        }
    }
    
    delete program;
    return dependencies;
}

bool DynamicLinker::LoadCriticalLibraries() {
    // Load essential Haiku libraries
    const char* critical_libs[] = {
        "libroot.so",
        "libbe.so", 
        "libbsd.so",
        "libnetwork.so",
        "libmedia.so",
        nullptr
    };
    
    for (int i = 0; critical_libs[i]; i++) {
        bool loaded = false;
        
        for (const std::string& search_path : fSearchPaths) {
            std::string full_path = search_path + "/" + critical_libs[i];
            
            if (LoadLibrary(full_path.c_str())) {
                loaded = true;
                break;
            }
        }
        
        if (!loaded) {
            printf("[DYNAMIC] Warning: Failed to load critical library: %s\n", critical_libs[i]);
        }
    }
    
    return true;
}

std::string DynamicLinker::ResolveLibraryPath(const char* name) {
    if (!name) return "";
    
    // Try all search paths
    for (const std::string& search_path : fSearchPaths) {
        std::string full_path = search_path + "/" + std::string(name);
        FILE* test = fopen(full_path.c_str(), "rb");
        if (test) {
            fclose(test);
            return full_path;
        }
    }
    
    return "";
}

bool DynamicLinker::IsLibraryLoaded(const char* name) const {
    if (!name) return false;
    
    auto it = fLibraries.find(std::string(name));
    return (it != fLibraries.end() && it->second.loaded);
}

std::vector<std::string> DynamicLinker::GetLoadedLibraries() const {
    std::vector<std::string> loaded;
    
    for (const auto& [name, info] : fLibraries) {
        if (info.loaded) {
            loaded.push_back(name);
        }
    }
    
    return loaded;
}

ElfImage* DynamicLinker::FindInLibraries(const char* name) {
    if (!name) return nullptr;
    
    for (auto& [lib_name, info] : fLibraries) {
        if (!info.elf_image || !info.loaded) continue;
        
        const Elf32_Sym* symbols = info.elf_image->GetSymbolTable();
        const char* str_table = info.elf_image->GetStringTable();
        uint32_t symbol_count = info.elf_image->GetSymbolCount();
        
        for (uint32_t i = 0; i < symbol_count; i++) {
            const char* symbol_name = str_table + symbols[i].st_name;
            
            if (strcmp(symbol_name, name) == 0) {
                return info.elf_image;
            }
        }
    }
    
    return nullptr;
}

std::string DynamicLinker::ResolveLibPath(const char* name) {
    return ResolveLibraryPath(name);
}

std::string DynamicLinker::GetLibraryName(const char* path) const {
    if (!path) return "";
    
    std::string full_path(path);
    
    // Find the last '/' character
    size_t last_slash = full_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        return full_path.substr(last_slash + 1);
    }
    
    return full_path;
}

ElfImage* DynamicLinker::LoadLibraryELF(const char* path) {
    return ElfImage::Load(path);
}

bool DynamicLinker::FindSymbolInELF(const char* name, void** address, size_t* size) {
    return FindSymbol(name, address, size);
}

size_t DynamicLinker::GetSymbolSize(void* handle, const char* name) {
    // For simplicity, return 0 for now
    return 0;
}