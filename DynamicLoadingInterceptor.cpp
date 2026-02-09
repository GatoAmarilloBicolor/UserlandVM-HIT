// Dynamic Loading Interceptor
// Intercepts dlopen, dlsym, and other dynamic loading syscalls

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

// Forward declarations to linker
void linker_init();
uint32_t linker_load_library(const char* libname);
uint32_t linker_resolve_symbol(const char* symbol_name);
uint32_t linker_get_library_base(const char* libname);

// Syscall numbers for dynamic loading
#define SYSCALL_DLOPEN   0x3000
#define SYSCALL_DLSYM    0x3001
#define SYSCALL_DLCLOSE  0x3002
#define SYSCALL_DLERROR  0x3003

struct DynloadContext {
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp;
};

extern "C" {

// Initialize dynamic loading system
void dynload_init() {
    printf("[DYNLOAD] Initializing dynamic loading system\n");
    linker_init();
    printf("[DYNLOAD] ✓ Dynamic loading ready\n");
}

// Handle dlopen syscall (0x3000)
// ebx = library name (guest address)
// ecx = flags
// Returns handle (library base address)
uint32_t handle_dlopen(const char* libname, int flags, uint8_t* memory) {
    if (!libname || libname[0] == '\0') {
        printf("[DLOPEN] ERROR: Invalid library name\n");
        return 0;
    }
    
    printf("[DLOPEN] dlopen('%s', 0x%x)\n", libname, flags);
    
    uint32_t handle = linker_load_library(libname);
    
    if (handle == 0) {
        printf("[DLOPEN] ERROR: Failed to load library\n");
        return 0;
    }
    
    printf("[DLOPEN] ✓ Library loaded with handle 0x%08x\n", handle);
    return handle;
}

// Handle dlsym syscall (0x3001)
// ebx = library handle
// ecx = symbol name (guest address)
// Returns symbol address
uint32_t handle_dlsym(uint32_t handle, const char* symbol_name, uint8_t* memory) {
    if (!symbol_name || symbol_name[0] == '\0') {
        printf("[DLSYM] ERROR: Invalid symbol name\n");
        return 0;
    }
    
    printf("[DLSYM] dlsym(0x%08x, '%s')\n", handle, symbol_name);
    
    uint32_t symbol_addr = linker_resolve_symbol(symbol_name);
    
    if (symbol_addr == 0) {
        printf("[DLSYM] WARNING: Symbol not found, returning NULL\n");
        return 0;
    }
    
    printf("[DLSYM] ✓ Symbol resolved to 0x%08x\n", symbol_addr);
    return symbol_addr;
}

// Handle dlclose syscall (0x3002)
int handle_dlclose(uint32_t handle) {
    printf("[DLCLOSE] dlclose(0x%08x)\n", handle);
    // Just return success - no actual unloading needed
    return 0;
}

// Handle dlerror syscall (0x3003)
const char* handle_dlerror() {
    return "No errors";
}

// Main dynamic loading syscall handler
int handle_dynamicload_syscall(int syscall_num, DynloadContext* ctx, uint8_t* memory) {
    printf("[DYNLOAD] Intercepted syscall: 0x%04x\n", syscall_num);
    
    switch (syscall_num) {
        case SYSCALL_DLOPEN: {
            // ebx = library name address
            // ecx = flags
            const char* libname = (const char*)(memory + ctx->ebx);
            uint32_t flags = ctx->ecx;
            uint32_t result = handle_dlopen(libname, flags, memory);
            ctx->eax = result;
            return 0;
        }
        
        case SYSCALL_DLSYM: {
            // ebx = handle
            // ecx = symbol name address
            uint32_t handle = ctx->ebx;
            const char* symbol_name = (const char*)(memory + ctx->ecx);
            uint32_t result = handle_dlsym(handle, symbol_name, memory);
            ctx->eax = result;
            return 0;
        }
        
        case SYSCALL_DLCLOSE: {
            // ebx = handle
            uint32_t handle = ctx->ebx;
            int result = handle_dlclose(handle);
            ctx->eax = result;
            return 0;
        }
        
        case SYSCALL_DLERROR: {
            // Return pointer to error string
            const char* error_str = handle_dlerror();
            ctx->eax = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(error_str) & 0xFFFFFFFF);
            return 0;
        }
        
        default:
            printf("[DYNLOAD] Unhandled dynamic load syscall: 0x%04x\n", syscall_num);
            ctx->eax = -1;
            return -1;
    }
}

// Wrapper for program initialization
// Maps library initialization functions
void initialize_program_libraries() {
    printf("[DYNLOAD] Initializing program libraries\n");
    
    // Load required libraries
    linker_load_library("libc.so.6");
    linker_load_library("libbe.so.1");
    linker_load_library("libcrypto.so");
    linker_load_library("libz.so");
    linker_load_library("libwebkit.so");
    
    printf("[DYNLOAD] ✓ All libraries initialized\n");
}

}  // extern "C"
