// Dynamic Linker Stub for UserlandVM
// Provides minimal symbol resolution for dynamic libraries

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

// Symbol resolution table
struct Symbol {
    const char* name;
    uint32_t address;
    uint32_t size;
};

// Mock library base addresses (in guest memory)
#define LIBC_BASE 0x10000000
#define LIBBE_BASE 0x20000000
#define LIBCRYPTO_BASE 0x30000000
#define LIBZ_BASE 0x40000000
#define LIBWEBKIT_BASE 0x50000000

// Minimal symbol table for common functions
static Symbol g_symbols[] = {
    // libc symbols
    {"malloc", LIBC_BASE + 0x1000, 0x100},
    {"free", LIBC_BASE + 0x1100, 0x100},
    {"printf", LIBC_BASE + 0x1200, 0x100},
    {"strlen", LIBC_BASE + 0x1300, 0x100},
    {"strcpy", LIBC_BASE + 0x1400, 0x100},
    {"strcmp", LIBC_BASE + 0x1500, 0x100},
    {"memcpy", LIBC_BASE + 0x1600, 0x100},
    {"memset", LIBC_BASE + 0x1700, 0x100},
    {"exit", LIBC_BASE + 0x1800, 0x100},
    
    // libbe symbols - GUI creation
    {"_ZN12BApplicationC1EPKc", LIBBE_BASE + 0x1000, 0x200},  // BApplication::BApplication(const char*)
    {"_ZN7BWindowC1EN5BRectS0_PKcjP8BView", LIBBE_BASE + 0x1200, 0x200},  // BWindow::BWindow(...)
    {"_ZN7BWindow4ShowEv", LIBBE_BASE + 0x1400, 0x100},  // BWindow::Show()
    {"_ZN5BViewC1EN5BRectS0_PKcjj", LIBBE_BASE + 0x1500, 0x200},  // BView::BView(...)
    {"_ZN6BApplication3RunEv", LIBBE_BASE + 0x1700, 0x300},  // BApplication::Run()
    
    // NULL terminator
    {nullptr, 0, 0}
};

class DynamicLinker {
public:
    static uint32_t ResolveSymbol(const char* symbol_name) {
        if (!symbol_name) return 0;
        
        printf("[LINKER] Resolving symbol: %s\n", symbol_name);
        
        for (int i = 0; g_symbols[i].name != nullptr; i++) {
            if (strcmp(g_symbols[i].name, symbol_name) == 0) {
                printf("[LINKER] ✓ Found: %s @ 0x%08x\n", symbol_name, g_symbols[i].address);
                return g_symbols[i].address;
            }
        }
        
        printf("[LINKER] ✗ NOT FOUND: %s (returning stub)\n", symbol_name);
        return 0;
    }
    
    static bool LoadLibrary(const char* libname) {
        printf("[LINKER] Loading library: %s\n", libname);
        
        if (strcmp(libname, "libc.so") == 0) {
            printf("[LINKER] ✓ libc mapped @ 0x%08x\n", LIBC_BASE);
            return true;
        }
        if (strcmp(libname, "libbe.so") == 0 || strcmp(libname, "libbe.so.1") == 0) {
            printf("[LINKER] ✓ libbe mapped @ 0x%08x\n", LIBBE_BASE);
            return true;
        }
        if (strstr(libname, "libcrypto") != nullptr) {
            printf("[LINKER] ✓ libcrypto mapped @ 0x%08x\n", LIBCRYPTO_BASE);
            return true;
        }
        if (strstr(libname, "libz") != nullptr) {
            printf("[LINKER] ✓ libz mapped @ 0x%08x\n", LIBZ_BASE);
            return true;
        }
        if (strstr(libname, "webkit") != nullptr) {
            printf("[LINKER] ✓ libwebkit mapped @ 0x%08x\n", LIBWEBKIT_BASE);
            return true;
        }
        
        printf("[LINKER] ⚠ Library not recognized: %s (assuming present)\n", libname);
        return true;
    }
    
    static void PrintSymbolTable() {
        printf("\n[LINKER] === Symbol Resolution Table ===\n");
        for (int i = 0; g_symbols[i].name != nullptr; i++) {
            printf("[LINKER] %3d: %-40s @ 0x%08x (size: %d)\n", 
                   i, g_symbols[i].name, g_symbols[i].address, g_symbols[i].size);
        }
        printf("[LINKER] ===================================\n\n");
    }
};

// Syscall numbers for GUI operations
#define SYSCALL_CREATE_WINDOW 0x2710
#define SYSCALL_SHOW_WINDOW   0x2711
#define SYSCALL_DRAW_RECT     0x2712
#define SYSCALL_DRAW_TEXT     0x2713

extern "C" {
    uint32_t vm_resolve_symbol(const char* symbol) {
        return DynamicLinker::ResolveSymbol(symbol);
    }
    
    bool vm_load_library(const char* libname) {
        return DynamicLinker::LoadLibrary(libname);
    }
    
    void vm_print_symbols() {
        DynamicLinker::PrintSymbolTable();
    }
}
