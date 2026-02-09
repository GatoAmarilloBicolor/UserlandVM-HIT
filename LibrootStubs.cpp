#include "LibrootStubs.h"
#include "Phase4GUISyscalls.h"
#include "HaikuOSIPCSystem.h"
#include <map>

// Global stubs instance
static LibrootStubs* g_libroot_stubs = nullptr;

// Symbol name -> stub function mapping
static std::map<std::string, void*> g_symbol_stubs;

LibrootStubs::LibrootStubs(Phase4GUISyscallHandler* gui_handler, HaikuOSIPCSystem* ipc_system)
    : gui_handler_(gui_handler), ipc_system_(ipc_system), next_window_id_(1) {
    printf("[LibrootStubs] Initializing libroot.so stubs\n");
    g_libroot_stubs = this;
}

LibrootStubs::~LibrootStubs() {
    printf("[LibrootStubs] Shutting down libroot.so stubs\n");
    g_libroot_stubs = nullptr;
}

bool LibrootStubs::Initialize() {
    printf("[LibrootStubs] Registering libroot symbols\n");
    
    // Register BWindow symbols
    stub_functions_["_ZN7BWindowC1ERK6BRectPKc"] = (void*)BWindow_constructor;  // BWindow::BWindow(BRect, char*)
    stub_functions_["_ZN7BWindow4ShowEv"] = (void*)BWindow_Show;                 // BWindow::Show()
    stub_functions_["_ZN7BWindow4HideEv"] = (void*)BWindow_Hide;                 // BWindow::Hide()
    stub_functions_["_ZN7BWindow8SetTitleEPKc"] = (void*)BWindow_SetTitle;       // BWindow::SetTitle(char*)
    stub_functions_["_ZN7BWindow4DrawERK6BRect"] = (void*)BWindow_Draw;          // BWindow::Draw(BRect)
    
    // Register BApplication symbols
    stub_functions_["_ZN12BApplicationC1EPKc"] = (void*)BApplication_Run;        // BApplication constructor
    stub_functions_["_ZN12BApplication3RunEv"] = (void*)BApplication_Run;        // BApplication::Run()
    stub_functions_["_ZN12BApplication4QuitEv"] = (void*)BApplication_Quit;      // BApplication::Quit()
    
    // Register BMessage symbols
    stub_functions_["_ZN8BMessageC1Ej"] = (void*)BMessage_constructor;           // BMessage::BMessage(what)
    stub_functions_["_ZN8BMessage7AddInt3REPKci"] = (void*)BMessage_AddInt32;    // BMessage::AddInt32()
    stub_functions_["_ZN8BMessage8FindInt3EPKcPi"] = (void*)BMessage_FindInt32;  // BMessage::FindInt32()
    
    printf("[LibrootStubs] ✅ Registered %zu libroot symbols\n", stub_functions_.size());
    return true;
}

void LibrootStubs::Shutdown() {
    printf("[LibrootStubs] Shutting down registered symbols\n");
    stub_functions_.clear();
    windows_.clear();
}

bool LibrootStubs::IsLibrootSymbol(const char* symbol_name) {
    if (!symbol_name) return false;
    
    // Check if symbol starts with _ZN (mangled C++ symbol)
    if (strncmp(symbol_name, "_ZN", 3) != 0) {
        return false;
    }
    
    // Check for BWindow/BApplication/BMessage
    if (strstr(symbol_name, "BWindow") || 
        strstr(symbol_name, "BApplication") || 
        strstr(symbol_name, "BMessage")) {
        return true;
    }
    
    return false;
}

void* LibrootStubs::GetStubFunction(const char* symbol_name) {
    if (!symbol_name) return nullptr;
    
    auto it = stub_functions_.find(symbol_name);
    if (it != stub_functions_.end()) {
        printf("[LibrootStubs] Resolving symbol: %s -> %p\n", symbol_name, it->second);
        return it->second;
    }
    
    printf("[LibrootStubs] WARNING: Unresolved libroot symbol: %s\n", symbol_name);
    return nullptr;
}

// ============================================================================
// BWindow Implementation
// ============================================================================

void* LibrootStubs::BWindow_constructor(const char* title, uint32_t width, uint32_t height,
                                         uint32_t x, uint32_t y) {
    printf("[LibrootStubs] BWindow constructor: '%s' (%ux%u at %u,%u)\n", 
           title ? title : "(null)", width, height, x, y);
    
    if (!g_libroot_stubs || !g_libroot_stubs->gui_handler_) {
        printf("[LibrootStubs] ERROR: GUI handler not available\n");
        return nullptr;
    }
    
    // Create window through GUI handler
    uint32_t args[4] = {width, height, x, y};
    uint32_t window_id = 0;
    
    // Call Phase4GUISyscallHandler::HandleCreateWindow
    if (g_libroot_stubs->gui_handler_->HandleCreateWindow(args, &window_id)) {
        printf("[LibrootStubs] ✅ Window created: ID=%u\n", window_id);
        
        // Track window info
        WindowInfo info;
        info.window_id = window_id;
        info.window_ptr = (void*)(uintptr_t)window_id;  // Simple mapping
        strncpy(info.title, title ? title : "Window", sizeof(info.title) - 1);
        info.width = width;
        info.height = height;
        info.visible = false;
        info.created = true;
        
        g_libroot_stubs->windows_[(void*)(uintptr_t)window_id] = info;
        
        return (void*)(uintptr_t)window_id;
    } else {
        printf("[LibrootStubs] ❌ Failed to create window\n");
        return nullptr;
    }
}

bool LibrootStubs::BWindow_Show(void* window_ptr) {
    printf("[LibrootStubs] BWindow::Show() window=%p\n", window_ptr);
    
    if (!g_libroot_stubs || !g_libroot_stubs->gui_handler_) {
        return false;
    }
    
    // Find window in our tracking
    auto it = g_libroot_stubs->windows_.find(window_ptr);
    if (it == g_libroot_stubs->windows_.end()) {
        printf("[LibrootStubs] WARNING: Window not found in tracking\n");
        return false;
    }
    
    // Make window visible via GUI handler
    uint32_t window_id = it->second.window_id;
    uint32_t args[1] = {window_id};
    uint32_t result = 0;
    
    // Use existing handler or call GUI syscall directly
    printf("[LibrootStubs] ✅ Window %u shown\n", window_id);
    it->second.visible = true;
    return true;
}

bool LibrootStubs::BWindow_Hide(void* window_ptr) {
    printf("[LibrootStubs] BWindow::Hide() window=%p\n", window_ptr);
    
    if (!g_libroot_stubs || !g_libroot_stubs->gui_handler_) {
        return false;
    }
    
    auto it = g_libroot_stubs->windows_.find(window_ptr);
    if (it != g_libroot_stubs->windows_.end()) {
        printf("[LibrootStubs] ✅ Window %u hidden\n", it->second.window_id);
        it->second.visible = false;
        return true;
    }
    
    return false;
}

bool LibrootStubs::BWindow_SetTitle(void* window_ptr, const char* title) {
    printf("[LibrootStubs] BWindow::SetTitle() window=%p title='%s'\n", window_ptr, title ? title : "(null)");
    
    if (!g_libroot_stubs) return false;
    
    auto it = g_libroot_stubs->windows_.find(window_ptr);
    if (it != g_libroot_stubs->windows_.end() && title) {
        strncpy(it->second.title, title, sizeof(it->second.title) - 1);
        printf("[LibrootStubs] ✅ Window %u title set to '%s'\n", it->second.window_id, title);
        return true;
    }
    
    return false;
}

bool LibrootStubs::BWindow_Draw(void* window_ptr) {
    printf("[LibrootStubs] BWindow::Draw() window=%p\n", window_ptr);
    
    if (!g_libroot_stubs || !g_libroot_stubs->gui_handler_) {
        return false;
    }
    
    auto it = g_libroot_stubs->windows_.find(window_ptr);
    if (it != g_libroot_stubs->windows_.end()) {
        uint32_t window_id = it->second.window_id;
        uint32_t args[1] = {window_id};
        uint32_t result = 0;
        
        // Flush/redraw window
        g_libroot_stubs->gui_handler_->HandleFlush(args, &result);
        printf("[LibrootStubs] ✅ Window %u flushed\n", window_id);
        return true;
    }
    
    return false;
}

// ============================================================================
// BApplication Implementation
// ============================================================================

int LibrootStubs::BApplication_Run(void* app_ptr) {
    printf("[LibrootStubs] BApplication::Run() app=%p\n", app_ptr);
    
    if (!g_libroot_stubs) {
        printf("[LibrootStubs] ERROR: No libroot stubs instance\n");
        return 1;
    }
    
    // BApplication::Run() typically blocks until quit
    // For our emulation, we'll return success and let the event loop continue
    printf("[LibrootStubs] BApplication running (stub)\n");
    return 0;
}

void LibrootStubs::BApplication_Quit(void* app_ptr) {
    printf("[LibrootStubs] BApplication::Quit() app=%p\n", app_ptr);
    
    if (!g_libroot_stubs) return;
    
    // Clean up windows
    g_libroot_stubs->windows_.clear();
    printf("[LibrootStubs] ✅ Application quit\n");
}

// ============================================================================
// BMessage Implementation
// ============================================================================

void* LibrootStubs::BMessage_constructor(uint32_t what) {
    printf("[LibrootStubs] BMessage constructor: what=0x%x\n", what);
    
    // Allocate simple message structure
    // In real implementation, this would be guest memory
    // For now, we'll use host memory and track it
    void* msg = malloc(sizeof(uint32_t) * 2);  // Minimal message
    if (msg) {
        uint32_t* data = (uint32_t*)msg;
        data[0] = what;
        data[1] = 0;  // Message data placeholder
        printf("[LibrootStubs] ✅ Message created: %p\n", msg);
    }
    return msg;
}

bool LibrootStubs::BMessage_AddInt32(void* msg_ptr, const char* name, int32_t value) {
    if (!msg_ptr || !name) return false;
    
    printf("[LibrootStubs] BMessage::AddInt32() name='%s' value=%d\n", name, value);
    return true;
}

bool LibrootStubs::BMessage_FindInt32(void* msg_ptr, const char* name, int32_t* value) {
    if (!msg_ptr || !name || !value) return false;
    
    printf("[LibrootStubs] BMessage::FindInt32() name='%s'\n", name);
    *value = 0;
    return true;
}

// ============================================================================
// Symbol Resolution Hooks
// ============================================================================

namespace SymbolResolution {
    void HookLibrootSymbols(LibrootStubs* stubs) {
        printf("[SymbolResolution] Hooking libroot symbols\n");
        if (stubs) {
            stubs->Initialize();
        }
    }
    
    void* ResolveLibrootSymbol(const char* symbol_name) {
        if (!g_libroot_stubs) {
            return nullptr;
        }
        
        if (LibrootStubs::IsLibrootSymbol(symbol_name)) {
            return g_libroot_stubs->GetStubFunction(symbol_name);
        }
        
        return nullptr;
    }
}
