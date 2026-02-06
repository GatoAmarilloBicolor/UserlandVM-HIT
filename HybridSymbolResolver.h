/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under terms of MIT License.
 * 
 * HybridSymbolResolver.h - Hybrid symbol resolution using host, stubs, and ELF symbols
 */

#pragma once

#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <string>

// Forward declarations
// TODO: Fix when advanced symbol resolver is available
// class SymbolResolver;
class Haiku32SyscallDispatcher;
class StubFunctions;

enum SymbolType {
    SYM_TYPE_SYSCALL,    // Use Haiku32SyscallDispatcher
    SYM_TYPE_LIBC_HOST, // Direct host libc function
    SYM_TYPE_STUB,       // Use StubFunctions implementation
    SYM_TYPE_ELF_RESOLVE // Use advanced ELF symbol resolver
};

// Symbol mapping entry
struct SymbolMapping {
    const char* haiku_name;
    SymbolType type;
    void* implementation;
    const char* description;
};

class HybridSymbolResolver {
public:
    HybridSymbolResolver();
    ~HybridSymbolResolver();

    // Main symbol resolution method
    bool ResolveSymbol(const char* name, void** address, size_t* size);

    // Initialize with existing components
    void SetSyscallDispatcher(Haiku32SyscallDispatcher* dispatcher);
    // void SetAdvancedResolver(SymbolResolver* resolver); // TODO: Fix when SymbolResolver available

    // Statistics
    struct ResolutionStats {
        uint64_t total_requests;
        uint64_t syscall_resolutions;
        uint64_t libc_host_resolutions;
        uint64_t stub_resolutions;
        uint64_t elf_resolutions;
        uint64_t failed_resolutions;
        
        void Reset() {
            total_requests = syscall_resolutions = libc_host_resolutions = 
            stub_resolutions = elf_resolutions = failed_resolutions = 0;
        }
    };

    ResolutionStats GetStats() const { return fStats; }
    void PrintStats() const;

private:
    // Core resolution methods
    bool ResolveSyscallSymbol(const char* name, void** address);
    bool ResolveLibcHostSymbol(const char* name, void** address);
    bool ResolveStubSymbol(const char* name, void** address);
    bool ResolveElfSymbol(const char* name, void** address);

    // Symbol mapping table
    static const SymbolMapping fSymbolMap[];
    static const size_t fSymbolMapSize;

    // Component references
    Haiku32SyscallDispatcher* fSyscallDispatcher;
    // SymbolResolver* fAdvancedResolver; // TODO: Fix when SymbolResolver available

    // Statistics
    mutable ResolutionStats fStats;

    // Helper methods
    void* GetSyscallWrapper(const char* name);
    bool IsCommonLibcSymbol(const char* name);
    bool IsHaikuSpecificSymbol(const char* name);
};