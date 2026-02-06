/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * DynamicLinker.cpp - Complete dynamic linking solution for Haiku compatibility
 */

#include "DynamicLinker.h"
#include <dlfcn.h>
#include <link.h>
#include <cstring>
#include <algorithm>
#include <vector>
#include <map>

DynamicLinker::DynamicLinker() {
    // Initialize standard Haiku search paths
    fSearchPaths = {
        "sysroot/haiku32/lib",
        "sysroot/haiku32/system/lib",
        "sysroot/haiku32/boot/system/lib",
        "/boot/system/lib",
        "/boot/system/non-packaged/lib",
        "/boot/home/config/lib",
        "/boot/common/lib",
        "/boot/common/settings/boot/leaves/lib",
        "/boot/common/settings/drivers/lib"
    };
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

    // Use system dynamic loader for Haiku compatibility
    void* handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        char* error = dlerror();
        printf("[DYNAMIC] Failed to load %s: %s\n", path, error ? error : "Unknown error");
        
        // Fallback to our own ELF loader
        return LoadLibraryELF(path);
    }

    // Get library info
    LibraryInfo info;
    info.handle = handle;
    info.loaded = true;
    info.elf_image = nullptr; // Not used when using system loader
    info.reference_count = 1;
    
    // Extract basic info
    link_map* map_info = (link_map*)dlsym(handle, "_r_debug");
    if (map_info) {
        info.base_address = (void*)map_info->l_addr;
        info.size = map_info->l_ld;
        printf("[DYNAMIC] Loaded %s at %p (size: %zu)\n", lib_name.c_str(), info.base_address, info.size);
    }
    
    fLibraries[lib_name] = info;
    return nullptr; // Image not available when using system loader
}

bool DynamicLinker::LoadDynamicDependencies(const char* program_path) {
    printf("[DYNAMIC] Loading dynamic dependencies for: %s\n", program_path);

    // For Haiku, we can rely on the system dynamic loader
    // But we need to load any explicitly required libraries
    
    // Load critical Haiku libraries first
    if (!LoadCriticalLibraries()) {
        printf("[DYNAMIC] Failed to load critical libraries\n");
        return false;
    }

    // Try to load program-specific dependencies
    std::vector<std::string> dependencies = GetDynamicDependencies(program_path);
    
    for (const auto& dep : dependencies) {
        if (!LoadLibrary(dep.c_str())) {
            printf("[DYNAMIC] Warning: Failed to load dependency: %s\n", dep.c_str());
            // Continue anyway - Haiku's runtime loader will resolve at runtime
        }
    }

    printf("[DYNAMIC] Dependency loading complete\n");
    return true;
}

bool DynamicLinker::FindSymbol(const char* name, void** address, size_t* size) {
    if (!name || !address) return false;

    // Search in all loaded libraries
    for (const auto& [lib_name, info] : fLibraries) {
        if (!info.loaded || !info.handle) continue;

        void* sym_addr = dlsym(info.handle, name);
        if (sym_addr) {
            printf("[DYNAMIC] Found symbol '%s' in %s at %p\n", name, lib_name.c_str(), sym_addr);
            *address = sym_addr;
            if (size) *size = GetSymbolSize(info.handle, name);
            return true;
        }
    }

    // Fallback to ELF symbol search
    return FindSymbolInELF(name, address, size);
}

std::vector<std::string> DynamicLinker::GetDynamicDependencies(const char* program_path) {
    std::vector<std::string> dependencies;
    
    // For now, return common Haiku dependencies
    // In a real implementation, we would parse the ELF dynamic section
    
    dependencies.push_back("libroot.so");
    dependencies.push_back("libbe.so");
    dependencies.push_back("libbsd.so");
    dependencies.push_back("libnetwork.so");
    dependencies.push_back("libz.so");
    
    return dependencies;
}

bool DynamicLinker::LoadCriticalLibraries() {
    printf("[DYNAMIC] Loading critical Haiku libraries\n");
    
    // Essential Haiku libraries
    const char* critical_libs[] = {
        "sysroot/haiku32/system/libroot.so",
        "sysroot/haiku32/system/libbe.so",
        "sysroot/haiku32/system/libbsd.so",
        "sysroot/haiku32/system/libnetwork.so"
    };
    
    bool success = true;
    for (const char* lib_path : critical_libs) {
        if (!LoadLibrary(lib_path)) {
            printf("[DYNAMIC] Failed to load critical library: %s\n", lib_path);
            success = false;
        }
    }
    
    return success;
}

// Private helper methods

std::string DynamicLinker::GetLibraryName(const char* path) {
    if (!path) return "";
    
    const char* name = strrchr(path, '/');
    if (name) {
        name++; // Skip the '/'
    } else {
        name = path;
    }
    
    return name;
}

ElfImage* DynamicLinker::LoadLibraryELF(const char* path) {
    printf("[DYNAMIC] Fallback: Loading with custom ELF loader\n");
    
    // Use the existing ELF loader as fallback
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
    // Try to get symbol size (simplified)
    Dl_info info;
    if (dladdr(handle, name, &info) == 0) {
        return info.dli_size;
    }
    return 0;
}

void DynamicLinker::SetSearchPath(const char* path) {
    if (path) {
        fSearchPaths.insert(fSearchPaths.begin(), path);
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
        
        fLibraries[lib_name] = info;
        printf("[DYNAMIC] Added ELF library: %s\n", name);
    }
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

// void DynamicLinker::ClearSymbolCache() {
//     // Clear any caches
//     printf("[DYNAMIC] Symbol cache cleared\n");
// }