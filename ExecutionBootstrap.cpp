#include "ExecutionBootstrap.h"
#include "VirtualCpuX86Native.h"
#include "X86_32GuestContext.h"
#include "InterpreterX86_32.h"
#include "platform/haiku/system/Haiku32SyscallDispatcher.h"
#include "DirectAddressSpace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <sys/mman.h>

// Global instance for fixes
UserlandVMFixes g_fixes;

ExecutionBootstrap::ExecutionBootstrap()
    : fExecutionEngine(nullptr),
      fDynamicLinker(nullptr),
      fGUIBackend(nullptr),
      fSymbolResolver(nullptr),
      fMemoryManager(nullptr),
      fThreadManager(nullptr),
      fProfiler(nullptr)
{
    printf("[ExecutionBootstrap] Initialized\n");
}

ExecutionBootstrap::~ExecutionBootstrap()
{
    printf("[ExecutionBootstrap] Destroyed\n");
}

status_t ExecutionBootstrap::ExecuteProgram(const char *programPath, char **argv, char **env)
{
    printf("[ExecutionBootstrap] Starting program execution: %s\n", programPath);
    
    // Load ELF binary
    ObjectDeleter<ElfImage> image(ElfImage::Load(programPath));
    if (!image.IsSet()) {
        fprintf(stderr, "[ExecutionBootstrap] Failed to load ELF binary: %s\n", programPath);
        return -1;
    }
    
    const char *arch = image->GetArchString();
    printf("[ExecutionBootstrap] Detected architecture: %s\n", arch ? arch : "unknown");
    fflush(stdout);
    
    // For now, use VirtualCpuX86Test for x86 execution
    if (arch && strcmp(arch, "x86") == 0) {
        printf("[ExecutionBootstrap] Using x86-32 interpreter execution\n");
        fflush(stdout);
        
        // Create address space with large guest memory
        // Allocate 2GB for guest (covers 0x08000000 - ~0xc0000000 on 32-bit)
        void *guestMemory = malloc(0x80000000);  // 2GB
        if (!guestMemory) {
            fprintf(stderr, "[ExecutionBootstrap] Failed to allocate guest memory\n");
            return -1;
        }
        
        DirectAddressSpace addressSpace;
        // Use direct memory access since we allocated it ourselves
        addressSpace.SetGuestMemoryBase((addr_t)guestMemory, 0x80000000);
        
        // Create syscall dispatcher
        Haiku32SyscallDispatcher dispatcher;
        
        // Create x86-32 guest context
        X86_32GuestContext context(addressSpace);
        
        // Load program image into guest memory
        printf("[ExecutionBootstrap] Loading program into guest memory...\n");
        fflush(stdout);
        
        // Read program file
        FILE *f = fopen(programPath, "rb");
        if (!f) {
            fprintf(stderr, "[ExecutionBootstrap] Failed to open program file\n");
            return -1;
        }
        
        // Get file size
        fseek(f, 0, SEEK_END);
        size_t fileSize = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        // Read ELF header
        Elf32_Ehdr ehdr;
        if (fread(&ehdr, sizeof(ehdr), 1, f) != 1) {
            fprintf(stderr, "[ExecutionBootstrap] Failed to read ELF header\n");
            fclose(f);
            return -1;
        }
        
        // Allocate guest memory for program
        uint32_t programBase = 0x08048000;  // Standard x86-32 code base
        uint8_t *programBuffer = new uint8_t[fileSize];
        
        // Rewind and read entire program
        fseek(f, 0, SEEK_SET);
        if (fread(programBuffer, fileSize, 1, f) != 1) {
            fprintf(stderr, "[ExecutionBootstrap] Failed to read program\n");
            fclose(f);
            delete[] programBuffer;
            return -1;
        }
        fclose(f);
        
        // Write program directly to guest memory
        // Since we're using direct memory access, guest addresses are direct pointers
        printf("[ExecutionBootstrap] Writing program to guest memory at 0x%08x (size=0x%lx)\n", 
               programBase, fileSize);
        
        // In direct memory mode, guest address is a direct pointer offset into guest memory
        // The base address 0x08048000 maps to offset (0x08048000 - 0x0) in guest memory
        uint32_t guestOffset = programBase;  // Direct mapping
        if (guestOffset + fileSize > 0x80000000) {
            fprintf(stderr, "[ExecutionBootstrap] Program would exceed guest memory bounds\n");
            delete[] programBuffer;
            return -1;
        }
        
        uint8_t *guestPtr = (uint8_t *)guestMemory + guestOffset;
        memcpy(guestPtr, programBuffer, fileSize);
        printf("[ExecutionBootstrap] Program loaded successfully\n");
        
        delete[] programBuffer;
        
        // Set up stack in guest memory
        // Note: We allocated 2GB (0x80000000) so stack must fit within this
        uint32_t stackBase = 0x70000000;  // Stack base (high in guest memory)
        uint32_t stackSize = 0x10000;     // 64KB stack
        
        // Initialize context registers
        auto& regs = context.Registers();
        regs.eip = ehdr.e_entry;  // Entry point from ELF header
        regs.esp = stackBase + stackSize - 4;  // Stack pointer at top of stack
        regs.ebp = regs.esp;
        regs.eax = 0;
        regs.ebx = 0;
        regs.ecx = 0;
        regs.edx = 0;
        regs.esi = 0;
        regs.edi = 0;
        regs.eflags = 0x202;  // IF and IOPL bits
        
        // Don't set 64-bit EIP - use guest address space instead
        // context.SetEIP64((uintptr_t)image->GetEntry());
        
        printf("[ExecutionBootstrap] Program entry point: 0x%08x (64-bit: 0x%lx)\n", regs.eip, context.GetEIP64());
        printf("[ExecutionBootstrap] Stack pointer: 0x%08x\n", regs.esp);
        
        // Create interpreter
        InterpreterX86_32 interpreter(addressSpace, dispatcher);
        
        // Execute
        printf("[ExecutionBootstrap] Executing program...\n");
        fflush(stdout);
        status_t status = interpreter.Run(context);
        
        printf("[ExecutionBootstrap] Execution completed with status: %d\n", status);
        return status;
    }
    
    fprintf(stderr, "[ExecutionBootstrap] Unsupported architecture: %s\n", arch ? arch : "unknown");
    return -1;
}
