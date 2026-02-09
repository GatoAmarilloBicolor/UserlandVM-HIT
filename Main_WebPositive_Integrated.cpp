// UserlandVM-HIT WebPositive Integration
// Complete unified system for running WebPositive on 64-bit Haiku
// Compiles with: g++ -std=c++17 -O2 Main_WebPositive_Integrated.cpp Loader.cpp ELFImage.cpp
//   DirectAddressSpace.cpp X86_32GuestContext.cpp InterpreterX86_32.cpp SymbolResolver.cpp
//   DebugOutput.cpp Syscalls.cpp BeAPIWrapper.cpp -o userlandvm_webkit -lbe

#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <thread>
#include <mutex>
#include <queue>

// Include status code unification FIRST to avoid conflicts
#include "UnifiedStatusCodes.h"

// Include VM infrastructure headers
#include "AddressSpace.h"
#include "Loader.h"
#include "ELFImage.h"
#include "DirectAddressSpace.h"
#include "X86_32GuestContext.h"
#include "InterpreterX86_32.h"
#include "GuestContext.h"
#include "SyscallDispatcher.h"
#include "BeAPIWrapper.h"

// ============================================================================
// UNIFIED EXECUTION ENGINE FOR WEBPOSITIVE
// ============================================================================

class WebPositiveVM {
private:
    std::unique_ptr<DirectAddressSpace> address_space;
    std::unique_ptr<X86_32GuestContext> guest_context;
    std::unique_ptr<InterpreterX86_32> interpreter;
    std::unique_ptr<SimpleSyscallDispatcher> syscall_dispatcher;
    
    ElfImage* loaded_image;
    uint32_t entry_point;
    bool is_dynamic;
    bool is_running;
    
    // Guest state
    char program_name[256];
    char working_directory[512];
    pid_t guest_team_id;
    time_t start_time;
    time_t end_time;
    
public:
    WebPositiveVM() 
        : loaded_image(nullptr), entry_point(0), is_dynamic(false), 
          is_running(false), guest_team_id(getpid())
    {
        memset(program_name, 0, sizeof(program_name));
        memset(working_directory, 0, sizeof(working_directory));
        getcwd(working_directory, sizeof(working_directory) - 1);
        
        // Initialize address space (64MB for 32-bit guest)
        address_space = std::make_unique<DirectAddressSpace>(64 * 1024 * 1024);
        
        // Initialize guest context
        guest_context = std::make_unique<X86_32GuestContext>(*address_space);
        
        // Initialize syscall dispatcher
        syscall_dispatcher = std::make_unique<SimpleSyscallDispatcher>();
        
        // Initialize interpreter
        interpreter = std::make_unique<InterpreterX86_32>(*address_space, *syscall_dispatcher);
        
        start_time = time(nullptr);
        
        printf("[USERLANDVM] Initialized WebPositive VM\n");
        printf("[USERLANDVM] Address space: 64MB\n");
        printf("[USERLANDVM] Guest team ID: %d\n", guest_team_id);
    }
    
    ~WebPositiveVM() {
        if (loaded_image) {
            delete loaded_image;
        }
    }
    
    bool LoadProgram(const char* path) {
        printf("\n[USERLANDVM] ============================================\n");
        printf("[USERLANDVM] Loading WebPositive binary\n");
        printf("[USERLANDVM] Path: %s\n", path);
        printf("[USERLANDVM] ============================================\n\n");
        
        strncpy(program_name, path, sizeof(program_name) - 1);
        
        // Load ELF image
        loaded_image = ElfImage::Load(path);
        if (!loaded_image) {
            printf("[USERLANDVM] ERROR: Failed to load ELF image\n");
            return false;
        }
        
        // Get entry point
        entry_point = loaded_image->GetEntry();
        is_dynamic = loaded_image->IsDynamic();
        
        printf("[USERLANDVM] ============================================\n");
        printf("[USERLANDVM] Program loaded successfully\n");
        printf("[USERLANDVM] Entry point: 0x%08x\n", entry_point);
        printf("[USERLANDVM] Program type: %s\n", is_dynamic ? "Dynamic" : "Static");
        printf("[USERLANDVM] Architecture: %s\n", loaded_image->GetArchString());
        printf("[USERLANDVM] ============================================\n\n");
        
        return true;
    }
    
    bool Execute(uint64_t max_instructions = 50000000) {
        if (!loaded_image) {
            printf("[USERLANDVM] ERROR: No program loaded\n");
            return false;
        }
        
        printf("[USERLANDVM] ============================================\n");
        printf("[USERLANDVM] Starting program execution\n");
        printf("[USERLANDVM] Max instructions: %llu\n", max_instructions);
        printf("[USERLANDVM] Entry point: 0x%08x\n", entry_point);
        printf("[USERLANDVM] ============================================\n\n");
        
        is_running = true;
        
        // Initialize guest context registers
        X86_32Registers& regs = guest_context->Registers();
        regs.eip = entry_point;
        regs.esp = 0xbfff8000;  // Near top of stack
        regs.ebp = 0xbfff8000;
        
        printf("[USERLANDVM] Initialized registers:\n");
        printf("[USERLANDVM]   EIP=0x%08x\n", regs.eip);
        printf("[USERLANDVM]   ESP=0x%08x\n", regs.esp);
        printf("[USERLANDVM]   EBP=0x%08x\n", regs.ebp);
        printf("[USERLANDVM]\n");
        
        // Run interpreter
        status_t status = interpreter->Run(*guest_context);
        
        is_running = false;
        end_time = time(nullptr);
        
        printf("\n[USERLANDVM] ============================================\n");
        printf("[USERLANDVM] Program execution completed\n");
        printf("[USERLANDVM] Status: %d\n", status);
        printf("[USERLANDVM] Execution time: %ld seconds\n", end_time - start_time);
        printf("[USERLANDVM] Final EIP: 0x%08x\n", regs.eip);
        printf("[USERLANDVM] ============================================\n\n");
        
        return status == B_OK;
    }
    
    void PrintSummary() const {
        printf("\n[USERLANDVM] ============================================\n");
        printf("[USERLANDVM] EXECUTION SUMMARY\n");
        printf("[USERLANDVM] ============================================\n");
        printf("Program: %s\n", program_name);
        printf("Working Directory: %s\n", working_directory);
        printf("Team ID: %d\n", guest_team_id);
        printf("Start Time: %s", ctime(&start_time));
        printf("End Time: %s", ctime(&end_time));
        printf("Total Time: %ld seconds\n", end_time - start_time);
        printf("[USERLANDVM] ============================================\n\n");
    }
};

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

int main(int argc, char* argv[]) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         UserlandVM-HIT WebPositive Integration              ║\n");
    printf("║   x86-32 Haiku Emulator with Real Window Support            ║\n");
    printf("║   Version: 1.0 (2026-02-09)                                 ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    if (argc < 2) {
        printf("Usage: %s <program_path> [options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  -v, --verbose     Enable verbose output\n");
        printf("  -g, --gui         Enable GUI window creation\n");
        printf("  -i <count>        Max instructions (default: 50000000)\n");
        printf("\nExample:\n");
        printf("  %s /path/to/webpositive -g\n", argv[0]);
        return 1;
    }
    
    const char* program_path = argv[1];
    bool enable_gui = false;
    uint64_t max_instructions = 50000000;
    
    // Parse options
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            // Enable verbose mode (would need to be propagated to interpreter)
            printf("[MAIN] Verbose mode enabled\n");
        } else if (arg == "-g" || arg == "--gui") {
            enable_gui = true;
            printf("[MAIN] GUI mode enabled\n");
        } else if (arg == "-i" && i + 1 < argc) {
            max_instructions = std::stoull(argv[++i]);
            printf("[MAIN] Max instructions: %llu\n", max_instructions);
        }
    }
    
    // Check if file exists
    if (access(program_path, F_OK) == -1) {
        printf("[MAIN] ERROR: File not found: %s\n", program_path);
        return 1;
    }
    
    printf("[MAIN] Target program: %s\n\n", program_path);
    
    // Create and run VM
    WebPositiveVM vm;
    
    // Load the program
    if (!vm.LoadProgram(program_path)) {
        printf("[MAIN] ERROR: Failed to load program\n");
        return 1;
    }
    
    // Create GUI window if requested
    if (enable_gui) {
        printf("[MAIN] ============================================\n");
        printf("[MAIN] Creating Haiku window for guest application\n");
        printf("[MAIN] ============================================\n\n");
        
        CreateHaikuWindow("WebPositive - UserlandVM");
        ShowHaikuWindow();
        
        printf("[MAIN] ✓ Window created and shown on Haiku desktop\n\n");
    }
    
    // Execute the program
    printf("[MAIN] Starting program execution...\n\n");
    
    if (!vm.Execute(max_instructions)) {
        printf("[MAIN] WARNING: Program execution returned non-zero status\n");
    }
    
    // Print summary
    vm.PrintSummary();
    
    // Process window events if GUI was created
    if (enable_gui) {
        printf("[MAIN] Entering event loop...\n");
        printf("[MAIN] Close the window to exit\n\n");
        ProcessWindowEvents();
    }
    
    printf("[MAIN] Exiting UserlandVM\n");
    return 0;
}
