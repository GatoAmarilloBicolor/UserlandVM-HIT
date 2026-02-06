/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under terms of MIT License.
 * 
 * HybridSymbolResolver.cpp - Hybrid symbol resolution implementation
 */

#include "HybridSymbolResolver.h"
#include "platform/haiku/system/Haiku32SyscallDispatcher.h"
#include "StubFunctions.h"
#include "dynamic_loading/SymbolResolver.h"  // Advanced one from dynamic_loading/
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <SupportDefs.h>

// Syscall wrapper functions
extern "C" {
    static uint32_t syscall_write(uint32_t fd, const void* buf, uint32_t size) {
        // This would be called by relocations pointing to write
        printf("[HYBRID] syscall_write wrapper called\n");
        return 0; // Would dispatch to Haiku32SyscallDispatcher
    }
    
    static uint32_t syscall_read(uint32_t fd, void* buf, uint32_t size) {
        printf("[HYBRID] syscall_read wrapper called\n");
        return 0;
    }
    
    static uint32_t syscall_open(const char* path, uint32_t flags, uint32_t mode) {
        printf("[HYBRID] syscall_open wrapper called\n");
        return 0;
    }
    
    static uint32_t syscall_close(uint32_t fd) {
        printf("[HYBRID] syscall_close wrapper called\n");
        return 0;
    }
    
    static void syscall_exit(uint32_t status) {
        printf("[HYBRID] syscall_exit wrapper called with status %u\n");
        // Would terminate execution
    }
}

// Complete symbol mapping table
const SymbolMapping HybridSymbolResolver::fSymbolMap[] = {
    // === SYSTEM CALLS (use Haiku32SyscallDispatcher) ===
    {"write", SYM_TYPE_SYSCALL, (void*)syscall_write, "System call: write to file descriptor"},
    {"read", SYM_TYPE_SYSCALL, (void*)syscall_read, "System call: read from file descriptor"},
    {"open", SYM_TYPE_SYSCALL, (void*)syscall_open, "System call: open file"},
    {"close", SYM_TYPE_SYSCALL, (void*)syscall_close, "System call: close file descriptor"},
    {"exit", SYM_TYPE_SYSCALL, (void*)syscall_exit, "System call: terminate program"},
    {"brk", SYM_TYPE_SYSCALL, nullptr, "System call: change break point"},
    {"mmap", SYM_TYPE_SYSCALL, nullptr, "System call: memory map"},
    {"munmap", SYM_TYPE_SYSCALL, nullptr, "System call: unmap memory"},
    
    // === LIBC HOST FUNCTIONS (direct host calls) ===
    {"malloc", SYM_TYPE_LIBC_HOST, (void*)malloc, "libc: allocate memory"},
    {"free", SYM_TYPE_LIBC_HOST, (void*)free, "libc: free memory"},
    {"calloc", SYM_TYPE_LIBC_HOST, (void*)calloc, "libc: allocate and zero memory"},
    {"realloc", SYM_TYPE_LIBC_HOST, (void*)realloc, "libc: reallocate memory"},
    {"memcpy", SYM_TYPE_LIBC_HOST, (void*)memcpy, "libc: copy memory"},
    {"memset", SYM_TYPE_LIBC_HOST, (void*)memset, "libc: set memory"},
    {"memcmp", SYM_TYPE_LIBC_HOST, (void*)memcmp, "libc: compare memory"},
    {"strcpy", SYM_TYPE_LIBC_HOST, (void*)strcpy, "libc: copy string"},
    {"strlen", SYM_TYPE_LIBC_HOST, (void*)strlen, "libc: string length"},
    {"strcmp", SYM_TYPE_LIBC_HOST, (void*)strcmp, "libc: compare strings"},
    {"printf", SYM_TYPE_LIBC_HOST, (void*)printf, "libc: formatted print"},
    {"fprintf", SYM_TYPE_LIBC_HOST, (void*)fprintf, "libc: formatted print to stream"},
    {"sprintf", SYM_TYPE_LIBC_HOST, (void*)sprintf, "libc: formatted print to string"},
    {"puts", SYM_TYPE_LIBC_HOST, (void*)puts, "libc: print string"},
    {"fgets", SYM_TYPE_LIBC_HOST, (void*)fgets, "libc: read string"},
    {"fopen", SYM_TYPE_LIBC_HOST, (void*)fopen, "libc: open file stream"},
    {"fclose", SYM_TYPE_LIBC_HOST, (void*)fclose, "libc: close file stream"},
    {"fread", SYM_TYPE_LIBC_HOST, (void*)fread, "libc: read from stream"},
    {"fwrite", SYM_TYPE_LIBC_HOST, (void*)fwrite, "libc: write to stream"},
    {"atoi", SYM_TYPE_LIBC_HOST, (void*)atoi, "libc: string to integer"},
    {"strtol", SYM_TYPE_LIBC_HOST, (void*)strtol, "libc: string to long"},
    
    // === GNU COREUTILS STUBS ===
    {"error", SYM_TYPE_STUB, nullptr, "GNU coreutils: error reporting"},
    {"error_at_line", SYM_TYPE_STUB, nullptr, "GNU coreutils: error with line number"},
    {"set_program_name", SYM_TYPE_STUB, nullptr, "GNU coreutils: set program name"},
    {"getprogname", SYM_TYPE_STUB, nullptr, "GNU coreutils: get program name"},
    {"version_etc", SYM_TYPE_STUB, nullptr, "GNU coreutils: version information"},
    {"usage", SYM_TYPE_STUB, nullptr, "GNU coreutils: usage message"},
    {"close_stdout", SYM_TYPE_STUB, nullptr, "GNU coreutils: close stdout"},
    {"quotearg", SYM_TYPE_STUB, nullptr, "GNU coreutils: quote arguments"},
    {"locale_charset", SYM_TYPE_STUB, nullptr, "GNU coreutils: locale charset"},
    
    // === HAIKU SPECIFIC ===
    {"create_thread", SYM_TYPE_STUB, nullptr, "Haiku: create thread"},
    {"kill_thread", SYM_TYPE_STUB, nullptr, "Haiku: kill thread"},
    {"find_directory", SYM_TYPE_STUB, nullptr, "Haiku: find directory"},
    {"get_team_info", SYM_TYPE_STUB, nullptr, "Haiku: get team information"},
    {"write_port", SYM_TYPE_STUB, nullptr, "Haiku: write to port"},
};

const size_t HybridSymbolResolver::fSymbolMapSize = sizeof(HybridSymbolResolver::fSymbolMap) / sizeof(SymbolMapping);

HybridSymbolResolver::HybridSymbolResolver() 
    : fSyscallDispatcher(nullptr), fAdvancedResolver(nullptr) {
    fStats.Reset();
    printf("[HYBRID] HybridSymbolResolver initialized with %zu symbols\n", fSymbolMapSize);
}

HybridSymbolResolver::~HybridSymbolResolver() {
    printf("[HYBRID] HybridSymbolResolver destroyed\n");
    printf("[HYBRID] Final stats: Total=%llu, Syscalls=%llu, LibC=%llu, Stubs=%llu, ELF=%llu, Failed=%llu\n",
           fStats.total_requests, fStats.syscall_resolutions, fStats.libc_host_resolutions,
            fStats.stub_resolutions, fStats.elf_resolutions);
}

void HybridSymbolResolver::SetSyscallDispatcher(Haiku32SyscallDispatcher* dispatcher) {
    fSyscallDispatcher = dispatcher;
    printf("[HYBRID] SyscallDispatcher set\n");
}

void HybridSymbolResolver::SetAdvancedResolver(AdvancedSymbolResolver* resolver) {
    fAdvancedResolver = resolver;
    printf("[HYBRID] Advanced ELF resolver set\n");
}

bool HybridSymbolResolver::ResolveSymbol(const char* name, void** address, size_t* size) {
    if (!name || !address) {
        printf("[HYBRID] Invalid parameters for symbol resolution\n");
        return false;
    }

    fStats.total_requests++;

    printf("[HYBRID] Resolving symbol: '%s'\n", name);

    // Try exact match in symbol map first
    for (size_t i = 0; i < fSymbolMapSize; i++) {
        if (strcmp(fSymbolMap[i].haiku_name, name) == 0) {
            const SymbolMapping& mapping = fSymbolMap[i];
            
            switch (mapping.type) {
                case SYM_TYPE_SYSCALL:
                    if (ResolveSyscallSymbol(name, address)) {
                        fStats.syscall_resolutions++;
                        printf("[HYBRID] ✓ Resolved via SYSCALL: %s\n", mapping.description);
                        if (size) *size = 0;
                        return true;
                    }
                    break;
                    
                case SYM_TYPE_LIBC_HOST:
                    if (ResolveLibcHostSymbol(name, address)) {
                        fStats.libc_host_resolutions++;
                        printf("[HYBRID] ✓ Resolved via LIBC_HOST: %s\n", mapping.description);
                        if (size) *size = 0;
                        return true;
                    }
                    break;
                    
                case SYM_TYPE_STUB:
                    if (ResolveStubSymbol(name, address)) {
                        fStats.stub_resolutions++;
                        printf("[HYBRID] ✓ Resolved via STUB: %s\n", mapping.description);
                        if (size) *size = 0;
                        return true;
                    }
                    break;
                    
                case SYM_TYPE_ELF_RESOLVE:
                    if (ResolveElfSymbol(name, address)) {
                        fStats.elf_resolutions++;
                        printf("[HYBRID] ✓ Resolved via ELF: %s\n", mapping.description);
                        if (size) *size = 0;
                        return true;
                    }
                    break;
            }
            
            // If we have a mapping but couldn't resolve it, return stub address
            if (mapping.implementation) {
                *address = mapping.implementation;
                printf("[HYBRID] ✓ Resolved via MAPPING: %s (static implementation)\n", mapping.description);
                return true;
            }
        }
    }

    // Fallback: try advanced ELF resolver if available
    if (fAdvancedResolver && ResolveElfSymbol(name, address)) {
        fStats.elf_resolutions++;
        printf("[HYBRID] ✓ Resolved via ADVANCED ELF resolver\n");
        return true;
    }

    // Fallback: try host dlsym for unknown symbols
    void* host_symbol = dlsym(RTLD_DEFAULT, name);
    if (host_symbol) {
        *address = host_symbol;
        printf("[HYBRID] ✓ Resolved via HOST FALLBACK: dlsym\n");
        return true;
    }

     // fStats.failed_resolutions++;
    printf("[HYBRID] ✗ Failed to resolve symbol: '%s'\n", name);
    return false;
}

bool HybridSymbolResolver::ResolveSyscallSymbol(const char* name, void** address) {
    // For now, return the wrapper function from the symbol map
    for (size_t i = 0; i < fSymbolMapSize; i++) {
        if (fSymbolMap[i].type == SYM_TYPE_SYSCALL && 
            strcmp(fSymbolMap[i].haiku_name, name) == 0 &&
            fSymbolMap[i].implementation) {
            *address = fSymbolMap[i].implementation;
            return true;
        }
    }
    return false;
}

bool HybridSymbolResolver::ResolveLibcHostSymbol(const char* name, void** address) {
    // Try dlsym from the main program for libc functions
    void* host_symbol = dlsym(RTLD_DEFAULT, name);
    if (host_symbol) {
        *address = host_symbol;
        return true;
    }
    return false;
}

bool HybridSymbolResolver::ResolveStubSymbol(const char* name, void** address) {
    // For now, return a dummy function pointer that does nothing
    // In a real implementation, this would call into StubFunctions
    static uint32_t stub_counter = 0xCAFE0000;
    
    // Create a dummy "function" that just returns
    static uint8_t stub_code[] = {
        0xB8, 0x00, 0x00, 0x00, 0x00, // mov eax, 0
        0xC3                              // ret
    };
    
    *address = (void*)stub_code;  // All stubs point to same dummy
    printf("[HYBRID] Using dummy stub for: %s\n", name);
    return true;
}

bool HybridSymbolResolver::ResolveElfSymbol(const char* name, void** address) {
    if (!fAdvancedResolver) {
        return false;
    }
    
    // Use the advanced ELF symbol resolver
    if (fAdvancedResolver) {
        // TODO: Fix when SymbolResolver is available
        // SymbolResolver::SymbolInfo info;
        // if (fAdvancedResolver->ResolveSymbolWithInfo(name, info) == B_OK) {
        //     *address = (void*)info.address;
        //     return true;
        // }
    }
    
    return false;
}

bool HybridSymbolResolver::IsCommonLibcSymbol(const char* name) {
    const char* libc_symbols[] = {
        "malloc", "free", "calloc", "realloc", "memcpy", "memset",
        "strcmp", "strcpy", "strlen", "printf", "fprintf", "sprintf",
        "fopen", "fclose", "fread", "fwrite", "atoi", "strtol"
    };
    
    for (const char* sym : libc_symbols) {
        if (strcmp(name, sym) == 0) {
            return true;
        }
    }
    return false;
}

bool HybridSymbolResolver::IsHaikuSpecificSymbol(const char* name) {
    const char* haiku_symbols[] = {
        "create_thread", "kill_thread", "find_directory", "get_team_info",
        "write_port", "read_port", "create_port", "delete_port"
    };
    
    for (const char* sym : haiku_symbols) {
        if (strcmp(name, sym) == 0) {
            return true;
        }
    }
    return false;
}

void HybridSymbolResolver::PrintStats() const {
    printf("\n=== HYBRID SYMBOL RESOLVER STATISTICS ===\n");
    printf("Total Requests:     %llu\n", fStats.total_requests);
    printf("Syscall Resolves:  %llu (%.1f%%)\n", 
           fStats.syscall_resolutions, 
           fStats.total_requests ? (fStats.syscall_resolutions * 100.0 / fStats.total_requests) : 0.0);
    printf("LibC Host Resolves:%llu (%.1f%%)\n", 
           fStats.libc_host_resolutions,
           fStats.total_requests ? (fStats.libc_host_resolutions * 100.0 / fStats.total_requests) : 0.0);
    printf("Stub Resolves:     %llu (%.1f%%)\n", 
           fStats.stub_resolutions,
           fStats.total_requests ? (fStats.stub_resolutions * 100.0 / fStats.total_requests) : 0.0);
    printf("ELF Resolves:      %llu (%.1f%%)\n", 
           fStats.elf_resolutions,
           fStats.total_requests ? (fStats.elf_resolutions * 100.0 / fStats.total_requests) : 0.0);
     // printf("Failed Resolves:   %llu (%.1f%%)\n", 
     //        fStats.failed_resolutions,
     //        fStats.total_requests ? (fStats.failed_resolves * 100.0 / fStats.total_requests) : 0.0);
    printf("==========================================\n\n");
}