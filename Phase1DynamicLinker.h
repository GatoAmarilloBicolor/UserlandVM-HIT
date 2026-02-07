#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <unordered_map>
#include <string>

// Phase 1: Minimal Dynamic Linker for PT_INTERP support
// Recycled from ultra-optimized implementation

class Phase1SymbolResolver {
private:
    struct SymbolInfo {
        uint32_t address;
        const char *name;
    };
    
    std::unordered_map<std::string, SymbolInfo> symbols;
    
public:
    Phase1SymbolResolver() {
        // 11 core Haiku symbols - placeholder addresses (will be resolved from runtime_loader)
        AddSymbol("malloc", 0x10001000);
        AddSymbol("free", 0x10002000);
        AddSymbol("strlen", 0x10003000);
        AddSymbol("strcpy", 0x10004000);
        AddSymbol("memcpy", 0x10005000);
        AddSymbol("memset", 0x10006000);
        AddSymbol("exit", 0x10007000);
        AddSymbol("printf", 0x10008000);
        AddSymbol("__cxa_atexit", 0x10009000);
        AddSymbol("__cxa_finalize", 0x1000a000);
        AddSymbol("_dyld_call_init_routine", 0x1000b000);
    }
    
    void AddSymbol(const char *name, uint32_t addr) {
        symbols[name] = {addr, name};
    }
    
    bool FindSymbol(const char *name, uint32_t *addr) {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            if (addr) *addr = it->second.address;
            return true;
        }
        return false;
    }
    
    void PrintSymbols() {
        printf("[Phase1] Loaded %zu symbols:\n", symbols.size());
        for (const auto &sym : symbols) {
            printf("  %s @ 0x%08x\n", sym.first.c_str(), sym.second.address);
        }
    }
};

class Phase1DynamicLinker {
private:
    Phase1SymbolResolver resolver;
    const char *interpreter_path;
    bool runtime_loader_loaded;
    
public:
    Phase1DynamicLinker() 
        : interpreter_path(nullptr), runtime_loader_loaded(false) {}
    
    ~Phase1DynamicLinker() {}
    
    // Set the interpreter path from PT_INTERP
    void SetInterpreterPath(const char *path) {
        interpreter_path = path;
        printf("[Phase1] Interpreter path: %s\n", path);
    }
    
    // Load and initialize runtime_loader
    bool LoadRuntimeLoader() {
        if (!interpreter_path || !*interpreter_path) {
            printf("[Phase1] ERROR: No interpreter path set\n");
            return false;
        }
        
        printf("[Phase1] Loading runtime_loader: %s\n", interpreter_path);
        
        // TODO: Actually load the runtime_loader ELF file
        // For now, just mark as loaded and use stub symbols
        runtime_loader_loaded = true;
        
        printf("[Phase1] Runtime loader initialized\n");
        resolver.PrintSymbols();
        
        return true;
    }
    
    // Resolve a symbol
    bool FindSymbol(const char *name, uint32_t *addr) {
        return resolver.FindSymbol(name, addr);
    }
    
    // Check if runtime_loader is loaded
    bool IsInitialized() const {
        return runtime_loader_loaded;
    }
    
    const char *GetInterpreterPath() const {
        return interpreter_path;
    }
};
