/*
 * ExecutionBootstrap - Integrated with RealDynamicLinker
 * Uses complete dynamic linking for HaikuOS programs
 */

#include "ExecutionBootstrap.h"
#include "DirectAddressSpace.h"
#include "Haiku32SyscallDispatcher.h"
#include "RealDynamicLinker.h"
#include "ELFImage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ExecutionBootstrap::ExecutionBootstrap()
    : fExecutionEngine(nullptr),
      fDynamicLinker(nullptr),
      fGUIBackend(nullptr),
      fSymbolResolver(nullptr),
      fMemoryManager(nullptr),
      fThreadManager(nullptr),
      fProfiler(nullptr)
{
    printf("[EXECUTION_BOOTSTRAP] Initialized with RealDynamicLinker integration\n");
}

ExecutionBootstrap::~ExecutionBootstrap()
{
    printf("[EXECUTION_BOOTSTRAP] Destroyed\n");
}

status_t ExecutionBootstrap::ExecuteProgram(const char *programPath, char **argv, char **env)
{
    printf("[EXECUTION_BOOTSTRAP] Starting program execution: %s\n", programPath);
    
    if (!programPath) {
        fprintf(stderr, "[EXECUTION_BOOTSTRAP] ERROR: Null program path\n");
        return B_BAD_VALUE;
    }
    
    // Load ELF binary to determine if it's dynamic
    ElfImage* image = ElfImage::Load(programPath);
    if (!image) {
        fprintf(stderr, "[EXECUTION_BOOTSTRAP] ERROR: Failed to load ELF binary: %s\n", programPath);
        return B_ERROR;
    }
    
    const char* arch = image->GetArchString();
    printf("[EXECUTION_BOOTSTRAP] Detected architecture: %s\n", arch ? arch : "unknown");
    
    bool isDynamic = image->IsDynamic();
    printf("[EXECUTION_BOOTSTRAP] Binary type: %s\n", isDynamic ? "DYNAMIC" : "STATIC");
    
    // Create guest memory space (2GB for x86-32) using abstraction
    HaikuMemoryAbstraction& memAbstraction = HaikuMemoryAbstraction::GetInstance();
    void* guestMemory;
    status_t memResult = memAbstraction.AllocateSimple(&guestMemory, 0x80000000);  // 2GB
    if (memResult != HAIKU_OK) {
        fprintf(stderr, "[EXECUTION_BOOTSTRAP] ERROR: Failed to allocate guest memory\n");
        delete image;
        return B_NO_MEMORY;
    }
    
    printf("[EXECUTION_BOOTSTRAP] Allocated 2GB guest memory at %p\n", guestMemory);
    
    // Setup address space for direct memory access
    DirectAddressSpace addressSpace;
    addressSpace.SetGuestMemoryBase((addr_t)guestMemory, 0x80000000);
    
    // Create syscall dispatcher
    Haiku32SyscallDispatcher syscallDispatcher;
    
    // Setup guest context
    X86_32GuestContext context(addressSpace);
    
    // Load main executable into guest memory
    printf("[EXECUTION_BOOTSTRAP] Loading program into guest memory...\n");
    fflush(stdout);
    
    // Read entire program file
    FILE* programFile = fopen(programPath, "rb");
    if (!programFile) {
        fprintf(stderr, "[EXECUTION_BOOTSTRAP] ERROR: Failed to open program file\n");
        delete image;
        free(guestMemory);
        return B_ERROR;
    }
    
    // Get file size
    fseek(programFile, 0, SEEK_END);
    size_t fileSize = ftell(programFile);
    fseek(programFile, 0, SEEK_SET);
    
    // Load program at standard x86-32 executable base
    uint32_t programBase = 0x08048000;  // Standard executable base
    
    // Copy program to guest memory
    if (programBase + fileSize > 0x80000000) {
        fprintf(stderr, "[EXECUTION_BOOTSTRAP] ERROR: Program too large for guest memory\n");
        fclose(programFile);
        delete image;
        free(guestMemory);
        return B_NO_MEMORY;
    }
    
    uint8_t* guestPtr = (uint8_t*)guestMemory + programBase;
    if (fread(guestPtr, fileSize, 1, programFile) != 1) {
        fprintf(stderr, "[EXECUTION_BOOTSTRAP] ERROR: Failed to read program\n");
        fclose(programFile);
        delete image;
        free(guestMemory);
        return B_ERROR;
    }
    fclose(programFile);
    
    printf("[EXECUTION_BOOTSTRAP] Program loaded at 0x%08x (size: %zu bytes)\n", 
           programBase, fileSize);
    
    // NEW: Apply dynamic linking for dynamic executables
    if (isDynamic) {
        printf("[EXECUTION_BOOTSTRAP] Applying DYNAMIC LINKING...\n");
        fflush(stdout);
        
        // Create real dynamic linker
        RealDynamicLinker dynamicLinker;
        
        // Perform complete dynamic linking process
        status_t linkResult = dynamicLinker.LinkExecutable(programPath, guestMemory);
        if (linkResult != B_OK) {
            fprintf(stderr, "[EXECUTION_BOOTSTRAP] ERROR: Dynamic linking failed: %d\n", linkResult);
            delete image;
            free(guestMemory);
            return linkResult;
        }
        
        printf("[EXECUTION_BOOTSTRAP] ✓ Dynamic linking completed successfully!\n");
    } else {
        printf("[EXECUTION_BOOTSTRAP] ✓ Static executable, no linking needed\n");
    }
    
    // Setup execution context
    const Elf32_Ehdr& ehdr = image->GetHeader();
    uint32_t entryPoint = ehdr.e_entry;
    
    auto& regs = context.Registers();
    regs.eip = entryPoint;      // Entry point from ELF header
    regs.esp = 0x70000000 + 0x01000000 - 4;  // Stack at top of allocated space
    regs.ebp = regs.esp;
    regs.eax = 0;
    regs.ebx = (uint32_t)argv;
    regs.ecx = 0;
    regs.edx = 0;
    regs.esi = 0;
    regs.edi = 0;
    regs.eflags = 0x202;  // Standard flags
    
    printf("[EXECUTION_BOOTSTRAP] Entry point: 0x%08x\n", regs.eip);
    printf("[EXECUTION_BOOTSTRAP] Stack pointer: 0x%08x\n", regs.esp);
    printf("[EXECUTION_BOOTSTRAP] Starting execution...\n");
    fflush(stdout);
    
    // Create interpreter and run
    InterpreterX86_32 interpreter(addressSpace, syscallDispatcher);
    
    status_t result = interpreter.Run(context);
    
    printf("[EXECUTION_BOOTSTRAP] Execution completed with status: %d\n", result);
    printf("[EXECUTION_BOOTSTRAP] Final EIP: 0x%08x\n", context.Registers().eip);
    
    // Cleanup
    delete image;
    free(guestMemory);
    
    return result;
}