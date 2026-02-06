/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under terms of MIT License.
 * 
 * DynamicLinker.cpp - Simplified dynamic linking for UserlandVM-HIT
 */

#include "DynamicLinker.h"
#include "HybridSymbolResolver.h"
#include <dlfcn.h>
#include <link.h>
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
        if (info.handle) {
            dlclose(info.handle);
        }
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

    printf("[DYNAMIC] Loading library: %s\n", path);

    // Try to load with system dynamic loader first
    void* handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        char* error = dlerror();
        printf("[DYNAMIC] Failed to load %s: %s\n", path, error ? error : "Unknown error");
        return nullptr;
    }

    // Store library info
    LibraryInfo info;
    info.handle = handle;
    info.loaded = true;
    info.elf_image = nullptr; // Not used when using system loader
    info.reference_count = 1;
    info.base_address = nullptr;
    info.size = 0;
    
    fLibraries[lib_name] = info;
    printf("[DYNAMIC] Successfully loaded %s\n", lib_name.c_str());
    return nullptr; // Image not available when using system loader
}

bool DynamicLinker::FindSymbol(const char* name, void** address, size_t* size) {
    if (!name || !address) return false;

    // First try hybrid symbol resolution
    static HybridSymbolResolver hybridResolver;
    if (hybridResolver.ResolveSymbol(name, address, size)) {
        printf("[DYNAMIC] Symbol '%s' resolved via HYBRID resolver\n", name);
        return true;
    }

    // Fallback to original method
    printf("[DYNAMIC] Symbol '%s' not found via HYBRID, trying original method\n", name);
    
    // Search in all loaded libraries
    for (const auto& [lib_name, info] : fLibraries) {
        if (!info.loaded || !info.handle) continue;

        void* sym_addr = dlsym(info.handle, name);
        if (sym_addr) {
            printf("[DYNAMIC] Found symbol '%s' in %s at %p\n", name, lib_name.c_str(), sym_addr);
            *address = sym_addr;
            if (size) *size = 0; // Size not available through dlsym
            return true;
        }
    }

    printf("[DYNAMIC] Symbol '%s' not found in any loaded library\n", name);
    return false;
}

ElfImage* DynamicLinker::GetLibrary(const char* name) {
    if (!name) return nullptr;
    
    std::string lib_name = GetLibraryName(name);
    auto it = fLibraries.find(lib_name);
    if (it != fLibraries.end() && it->second.loaded) {
        return it->second.elf_image;
    }
    return nullptr;
}

void DynamicLinker::SetSearchPath(const char* path) {
    if (path) {
        fSearchPaths.insert(fSearchPaths.begin(), path);
        printf("[DYNAMIC] Added search path: %s\n", path);
    }
}

void DynamicLinker::AddLibrary(const char* name, ElfImage* image) {
    if (name && image) {
        std::string lib_name = name;
        
        LibraryInfo info;
        info.handle = nullptr; // Not used for ELF images
        info.elf_image = image;
        info.loaded = true;
        info.reference_count = 1;
        info.base_address = nullptr;
        info.size = 0;
        
        fLibraries[lib_name] = info;
        printf("[DYNAMIC] Added ELF library: %s\n", name);
    }
}

bool DynamicLinker::LoadDynamicDependencies(const char* program_path) {
    printf("[DYNAMIC] Loading dynamic dependencies for: %s\n", program_path);

    // Load critical Haiku libraries first
    if (!LoadCriticalLibraries()) {
        printf("[DYNAMIC] Failed to load critical libraries\n");
        return false;
    }

    printf("[DYNAMIC] Dependency loading complete\n");
    return true;
}

std::vector<std::string> DynamicLinker::GetDynamicDependencies(const char* program_path) {
    std::vector<std::string> dependencies;
    
    // For now, return common Haiku dependencies
    // In a real implementation, we would parse ELF dynamic section
    dependencies.push_back("sysroot/haiku32/system/libroot.so");
    dependencies.push_back("sysroot/haiku32/system/libbe.so");
    dependencies.push_back("sysroot/haiku32/system/libbsd.so");
    
    return dependencies;
}

bool DynamicLinker::LoadCriticalLibraries() {
    printf("[DYNAMIC] Loading critical Haiku libraries\n");
    
    // Essential Haiku libraries
    const char* critical_libs[] = {
        "sysroot/haiku32/system/libroot.so",
        "sysroot/haiku32/system/libbe.so"
    };
    
    bool success = true;
    for (const char* lib_path : critical_libs) {
        if (!LoadLibrary(lib_path)) {
            printf("[DYNAMIC] Warning: Failed to load critical library: %s\n", lib_path);
            // Continue anyway for now
        }
    }
    
    return success;
}

std::string DynamicLinker::ResolveLibraryPath(const char* name) {
    if (!name) return "";

    // Check if it's an absolute path
    if (name[0] == '/') {
        return name;
    }

    // Try each search path
    for (const auto& search_path : fSearchPaths) {
        std::string full_path = search_path + "/" + name;
        
        // Check if file exists
        FILE* file = fopen(full_path.c_str(), "rb");
        if (file) {
            fclose(file);
            return full_path;
        }
        
        // Try with .so extension
        std::string so_path = full_path + ".so";
        file = fopen(so_path.c_str(), "rb");
        if (file) {
            fclose(file);
            return so_path;
        }
    }

    return name; // Not found
}

bool DynamicLinker::IsLibraryLoaded(const char* name) const {
    if (!name) return false;
    
    std::string lib_name = GetLibraryName(name);
    auto it = fLibraries.find(lib_name);
    return (it != fLibraries.end() && it->second.loaded);
}

std::vector<std::string> DynamicLinker::GetLoadedLibraries() const {
    std::vector<std::string> libraries;
    
    for (const auto& [name, info] : fLibraries) {
        if (info.loaded) {
            libraries.push_back(name);
        }
    }
    
    return libraries;
}

// Private helper methods

ElfImage* DynamicLinker::FindInLibraries(const char* name) {
    return GetLibrary(name);
}

std::string DynamicLinker::ResolveLibPath(const char* name) {
    return ResolveLibraryPath(name);
}

std::string DynamicLinker::GetLibraryName(const char* path) const {
    if (!path) return "";
    
    const char* name = strrchr(path, '/');
    if (name) {
        name++; // Skip '/'
    } else {
        name = path;
    }
    
    return name;
}

ElfImage* DynamicLinker::LoadLibraryELF(const char* path) {
    printf("[DYNAMIC] Fallback: Loading with custom ELF loader\n");
    
    // Use existing ELF loader as fallback
    return ElfImage::Load(path);
}

bool DynamicLinker::FindSymbolInELF(const char* name, void** address, size_t* size) {
    // Search in ELF images that were loaded with our custom loader
    for (const auto& [lib_name, info] : fLibraries) {
        if (info.elf_image) {
            if (info.elf_image->FindSymbol(name, address, size)) {
                printf("[DYNAMIC] Found ELF symbol '%s' in %s\n", name, lib_name.c_str());
                return true;
            }
        }
    }
    return false;
}

size_t DynamicLinker::GetSymbolSize(void* handle, const char* name) {
    // Try to get symbol size (simplified - not available through dladdr)
    return 0;
}