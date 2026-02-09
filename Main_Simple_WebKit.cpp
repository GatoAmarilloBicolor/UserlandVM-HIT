// UserlandVM-HIT WebPositive Integration (Simplified)
// Avoids header conflicts by using only essential components

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>

// Forward declarations only
class AddressSpace;
class GuestContext;
class InterpreterX86_32;

// Include only the essential wrapper
#include "BeAPIWrapper.h"

// ============================================================================
// LOAD AND EXECUTE WEBPOSITIVE
// ============================================================================

// Simplified external C interface to the real VM components
extern "C" {
    // Loader interface
    void* LoadELFProgram(const char* path);
    uint32_t GetEntryPoint(void* elf_image);
    void* CreateAddressSpace(size_t size_mb);
    void* CreateGuestContext(void* address_space);
    void* CreateInterpreter(void* address_space);
    
    // Execution interface
    int ExecuteProgram(void* interpreter, void* guest_context, uint32_t entry_point, 
                       uint64_t max_instructions);
    
    // Cleanup
    void CleanupProgram(void* elf_image, void* interpreter, void* guest_context, void* address_space);
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

int main(int argc, char* argv[]) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║     UserlandVM-HIT WebPositive Integration                 ║\n");
    printf("║   x86-32 Haiku Emulator with Real GUI Support             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    if (argc < 2) {
        printf("Usage: %s <program_path> [options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  -g, --gui     Enable GUI window creation\n");
        printf("\nExample:\n");
        printf("  %s /path/to/webpositive -g\n", argv[0]);
        return 1;
    }
    
    const char* program_path = argv[1];
    bool enable_gui = false;
    
    // Parse options
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-g" || arg == "--gui") {
            enable_gui = true;
        }
    }
    
    printf("[MAIN] Loading: %s\n", program_path);
    
    // Check if file exists
    if (access(program_path, F_OK) == -1) {
        printf("[MAIN] ERROR: File not found\n");
        return 1;
    }
    
    // Create VM components
    printf("[MAIN] Initializing VM...\n");
    void* address_space = CreateAddressSpace(64);
    void* guest_context = CreateGuestContext(address_space);
    void* interpreter = CreateInterpreter(address_space);
    
    // Load program
    printf("[MAIN] Loading ELF binary...\n");
    void* elf_image = LoadELFProgram(program_path);
    if (!elf_image) {
        printf("[MAIN] ERROR: Failed to load program\n");
        return 1;
    }
    
    uint32_t entry_point = GetEntryPoint(elf_image);
    printf("[MAIN] Entry point: 0x%08x\n", entry_point);
    
    // Create GUI window if requested
    if (enable_gui) {
        printf("[MAIN] Creating Haiku window...\n");
        CreateHaikuWindow("WebPositive - UserlandVM");
        ShowHaikuWindow();
        printf("[MAIN] Window created\n");
    }
    
    // Execute program
    printf("[MAIN] Starting execution (max 50M instructions)...\n");
    int status = ExecuteProgram(interpreter, guest_context, entry_point, 50000000);
    printf("[MAIN] Execution finished with status: %d\n", status);
    
    // Cleanup
    CleanupProgram(elf_image, interpreter, guest_context, address_space);
    
    // Process events if GUI was created
    if (enable_gui) {
        printf("[MAIN] Entering event loop...\n");
        ProcessWindowEvents();
    }
    
    printf("[MAIN] Done\n");
    return 0;
}
