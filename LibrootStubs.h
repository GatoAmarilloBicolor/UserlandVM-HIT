#pragma once

#include "PlatformTypes.h"
#include "Phase4GUISyscalls.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <map>
#include <memory>

// Forward declarations
class HaikuOSIPCSystem;
class Phase4GUISyscallHandler;

/**
 * LibrootStubs.h
 * 
 * Minimal stubs for Haiku libroot.so symbols that WebPositive needs.
 * Maps BWindow/BApplication C++ calls to our Phase4GUISyscallHandler.
 * 
 * This layer bridges:
 *   WebPositive (C++ code using BWindow)
 *     ↓
 *   BWindow/BApplication symbols
 *     ↓
 *   Phase4GUISyscallHandler (our GUI syscall handler)
 */

class LibrootStubs {
public:
    LibrootStubs(Phase4GUISyscallHandler* gui_handler, HaikuOSIPCSystem* ipc_system);
    ~LibrootStubs();
    
    // Initialize stubs (hook into symbol resolver)
    bool Initialize();
    void Shutdown();
    
    // Check if a symbol should be intercepted
    static bool IsLibrootSymbol(const char* symbol_name);
    
    // Get stub function pointer for a symbol
    void* GetStubFunction(const char* symbol_name);
    
    // BWindow stubs
    static void* BWindow_constructor(const char* title, uint32_t width, uint32_t height, 
                                      uint32_t x, uint32_t y);
    static bool BWindow_Show(void* window_ptr);
    static bool BWindow_Hide(void* window_ptr);
    static bool BWindow_SetTitle(void* window_ptr, const char* title);
    static bool BWindow_Draw(void* window_ptr);
    
    // BApplication stubs
    static int BApplication_Run(void* app_ptr);
    static void BApplication_Quit(void* app_ptr);
    
    // BMessage stubs for IPC
    static void* BMessage_constructor(uint32_t what);
    static bool BMessage_AddInt32(void* msg_ptr, const char* name, int32_t value);
    static bool BMessage_FindInt32(void* msg_ptr, const char* name, int32_t* value);
    
private:
    Phase4GUISyscallHandler* gui_handler_;
    HaikuOSIPCSystem* ipc_system_;
    
    // Internal window tracking
    struct WindowInfo {
        int32_t window_id;
        void* window_ptr;  // Pointer in guest memory
        char title[256];
        uint32_t width;
        uint32_t height;
        bool visible;
        bool created;
    };
    
    std::map<void*, WindowInfo> windows_;
    std::map<const char*, void*> stub_functions_;
    
    // Track next window ID
    int32_t next_window_id_;
};

/**
 * Symbol Resolution Hooks
 * 
 * These are called by the dynamic linker when resolving symbols from libroot.so.
 * They redirect Haiku API calls to our stub implementations.
 */
namespace SymbolResolution {
    // Hook into symbol resolver
    void HookLibrootSymbols(LibrootStubs* stubs);
    
    // Resolve symbol from libroot
    void* ResolveLibrootSymbol(const char* symbol_name);
}
