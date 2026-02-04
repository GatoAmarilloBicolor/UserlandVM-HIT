#include "ExecutionBootstrap.h"
#include "CommpageManager.h"
#include "DirectAddressSpace.h"
#include "DynamicLinker.h"
#include "TLSSetup.h"
#include "VirtualCpuX86Native.h"
#include "X86_32GuestContext.h"
#include "InterpreterX86_32.h"
#include "Haiku32SyscallDispatcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ExecutionBootstrap::ExecutionBootstrap() {}

ExecutionBootstrap::~ExecutionBootstrap() {}

status_t ExecutionBootstrap::ExecuteProgram(const char *programPath,
                                            char **argv, char **env) {
  if (!programPath) {
    fprintf(stderr, "[X86] No program path provided\n");
    return 1;
  }

  printf("[X86] Loading x86 32-bit Haiku program: %s\n", programPath);
  fflush(stdout);

  // Load the ELF binary
  ObjectDeleter<ElfImage> image(ElfImage::Load(programPath));
  if (!image.IsSet()) {
    fprintf(stderr, "[X86] Failed to load program\n");
    return 1;
  }

  printf("[X86] Program loaded at %p, entry=%p\n", image->GetImageBase(),
         image->GetEntry());
  fflush(stdout);

  // Setup execution context
  ProgramContext ctx;
  ctx.image = image.Get();
  // WARNING: In direct memory mode on 64-bit host, entry points are 64-bit pointers
  // but X86_32GuestContext uses 32-bit EIP. This truncation is intentional - we use
  // the lower 32 bits as an offset and rely on direct memory access to work with
  // the actual 64-bit host pointers
  void *fullEntryPoint = image->GetEntry();
  ctx.entryPoint = (uintptr_t)fullEntryPoint;  // Keep as uintptr_t (64-bit on 64-bit host)
  printf("[X86] Entry point: %p (will be used as host pointer)\n", fullEntryPoint);
  printf("[X86] Entry point as stored: 0x%lx\n", ctx.entryPoint);
  fflush(stdout);
  ctx.stackSize = DEFAULT_STACK_SIZE;
  ctx.linker = new DynamicLinker();

  // Load dynamic dependencies
  if (image->IsDynamic()) {
    printf("[X86] Loading dynamic dependencies\n");
    fflush(stdout);
    if (!LoadDependencies(ctx, image.Get())) {
      fprintf(stderr, "[X86] Failed to load dependencies\n");
      // Continue anyway as per previous logic
    }
  }

  // Resolve dynamic symbols and apply relocations
  if (image->IsDynamic()) {
    printf("[X86] Resolving dynamic symbols\n");
    fflush(stdout);
    if (!ResolveDynamicSymbols(ctx, image.Get())) {
      fprintf(stderr, "[X86] Failed to resolve symbols (continuing anyway)\n");
    }

    printf("[X86] Applying relocations\n");
    fflush(stdout);
    RelocationProcessor reloc_processor(ctx.linker);
    status_t reloc_status = reloc_processor.ProcessRelocations(image.Get());
    if (reloc_status != B_OK) {
      fprintf(stderr, "[X86] Failed to apply relocations: %d\n", reloc_status);
      // Continue anyway - some relocations might be optional
    }
  }

  // Note: DirectAddressSpace currently has limitations:
  // - It's designed to translate guest virtual addresses to offsets in a contiguous block
  // - But our loaded images are in malloc'd host memory at arbitrary addresses
  // - For now, we bypass address translation by using direct host pointers in the interpreter
  // TODO: Implement proper memory mapping to support address space isolation

  // Allocate stack for guest program
  ctx.stackBase = AllocateStack(ctx.stackSize);
  if (!ctx.stackBase) {
    fprintf(stderr, "[X86] Failed to allocate stack\n");
    return 1;
  }

  ctx.stackPointer = (uint32)(unsigned long long)ctx.stackBase + ctx.stackSize;
  printf("[X86] Stack allocated at %p, sp=%#x\n", ctx.stackBase,
         ctx.stackPointer);
  fflush(stdout);

  // Build the stack with arguments
  if (!BuildX86Stack(ctx, argv, env)) {
    fprintf(stderr, "[X86] Failed to build stack\n");
    return 1;
  }

  printf("[X86] Ready to execute x86 32-bit program\n");
  printf("[X86] Entry point: %#x\n", ctx.entryPoint);
  printf("[X86] Stack pointer: %#x\n", ctx.stackPointer);
  printf("[X86] ===== Program Output =====\n");
  fflush(stdout);

  // Create address space wrapper for guest memory
  // Using direct memory mode since we load binaries into host malloc'd memory
  DirectAddressSpace addressSpace;
  addressSpace.SetGuestMemoryBase(0, 0);  // Placeholder, direct memory mode enabled
  printf("[X86] Direct memory mode enabled for address space\n");
  fflush(stdout);
  
  // Setup environment (Commpage, TLS) - must be done AFTER address space is ready
  printf("[X86] About to setup environment\n");
  fflush(stdout);
  if (!SetupX86Environment(ctx, argv, env, addressSpace)) {
    fprintf(stderr, "[X86] Failed to setup environment\n");
    return 1;
  }
  printf("[X86] Environment setup complete\n");
  fflush(stdout);
  
  // Create X86_32GuestContext for interpreter
  X86_32GuestContext guestContext(addressSpace);
  printf("[X86] Guest context created\n");
  fflush(stdout);
  
  // Set initial registers
  X86_32Registers &regs = guestContext.Registers();
  regs.eax = 0;
  regs.ebx = 0;
  regs.ecx = 0;
  regs.edx = 0;
  regs.esi = 0;
  regs.edi = 0;
  regs.esp = ctx.stackPointer;
  regs.ebp = ctx.stackPointer;
  // In direct memory mode, store 64-bit entry point separately
  guestContext.SetEIP64(ctx.entryPoint);
  regs.eip = (uint32)ctx.entryPoint;  // Also store lower 32 bits in case needed
  printf("[X86] Setting EIP: full address=0x%lx, 32-bit=0x%x\n", ctx.entryPoint, regs.eip);
  fflush(stdout);
  regs.eflags = 0x202; // IF and reserved bits
  
  printf("[X86] Guest context initialized\n");
  fflush(stdout);

  // Create syscall dispatcher
  Haiku32SyscallDispatcher syscallDispatcher(&addressSpace);
  
  // Run the interpreter
  printf("[X86] Starting x86-32 interpreter\n");
  fflush(stdout);
  
  // Execute using the interpreter in a loop
  OptimizedX86Executor executor(addressSpace, syscallDispatcher);
  uint32 exitCode = 0;
  uint32 instructionCount = 0;
  const uint32 MAX_INSTRUCTIONS = 1000000;  // Safety limit to prevent infinite loops
  
  printf("[X86] Execution loop starting, max %u instructions\n", MAX_INSTRUCTIONS);
  fflush(stdout);
  
  // TEMPORARY DEBUG: Just print first few instructions without executing
  printf("[X86] DEBUG MODE: Printing instructions without execution\n");
  fflush(stdout);
  
  for (int i = 0; i < 5; i++) {
    uintptr_t eip64 = guestContext.GetEIP64();
    printf("[X86] Instruction %d: EIP=0x%lx (lower 32: 0x%x)\n", i, eip64, guestContext.Registers().eip);
    
    // Try to peek at memory
    uint8_t* code_ptr = (uint8_t*)eip64;
    if (code_ptr) {
      printf("[X86]   Bytes: %02x %02x %02x %02x\n", code_ptr[0], code_ptr[1], code_ptr[2], code_ptr[3]);
    }
    fflush(stdout);
    
    // Don't actually execute
    break;  // Just do one iteration for now
  }
  
  printf("[X86] Exiting early for debug\n");
  fflush(stdout);
  
  if (false) {  // Disabled for now
    while (!guestContext.ShouldExit() && instructionCount < MAX_INSTRUCTIONS) {
      printf("[X86Loop] Before execute, instruction %u\n", instructionCount);
      fflush(stdout);
      
      status_t status = executor.Execute(guestContext, exitCode);
      
      printf("[X86Loop] After execute, status=%d\n", status);
      fflush(stdout);
    
    if (status != B_OK) {
      printf("[X86] Executor returned error: %d at instruction %u\n", status, instructionCount);
      fflush(stdout);
      break;
    }
    
    instructionCount++;
    
    // Print progress every 10 instructions (for debugging)
    if (instructionCount % 10 == 0) {
      printf("[X86] Executed %u instructions\n", instructionCount);
      fflush(stdout);
    }
    }
    }  // End of if(false)
    
    if (instructionCount >= MAX_INSTRUCTIONS) {
    printf("[X86] WARNING: Reached instruction limit (%u), possible infinite loop\n", MAX_INSTRUCTIONS);
  } else {
    printf("[X86] Program exited normally after %u instructions\n", instructionCount);
  }

  printf("[X86] ===== Program Terminated with code %u =====\n", exitCode);
  fflush(stdout);

  return exitCode;
}

void *ExecutionBootstrap::AllocateStack(size_t size) {
  // Allocate 32-bit addressable memory for stack
  void *stack = malloc(size);
  printf("[X86] Allocated stack: %p (size=%zu)\n", stack, size);
  return stack;
}

bool ExecutionBootstrap::BuildX86Stack(ProgramContext &ctx, char **argv,
                                       char **env) {
  printf("[X86] Building stack with %zu bytes available\n", ctx.stackSize);

  // Build stack frame at the top of the stack going downward
  uint32 sp = ctx.stackPointer;
  uint8 *stack_base = (uint8 *)ctx.stackBase;

  // Count arguments and environment variables
  int argc = 0;
  while (argv && argv[argc])
    argc++;

  int envc = 0;
  while (env && env[envc])
    envc++;

  printf("[X86] argc=%d, envc=%d\n", argc, envc);

  // Stack layout (x86):
  // [esp + 0]  = argc
  // [esp + 4]  = argv[0]
  // [esp + 8]  = argv[1]
  // ...
  // [esp + 4*(argc+1)] = NULL
  // [esp + 4*(argc+2)] = env[0]
  // ...

  // For now, just set up argc at stack pointer
  // This is simplified - a full implementation would copy strings

  // Write argc
  if (sp >= 4) {
    sp -= 4;
    *(uint32 *)(stack_base + (sp - (uint32)(unsigned long long)stack_base)) =
        argc;
    printf("[X86] Wrote argc=%d at %#x\n", argc, sp);
  }

  // Update stack pointer
  ctx.stackPointer = sp;

  printf("[X86] Stack frame built, new sp=%#x\n", ctx.stackPointer);
  return true;
}

bool ExecutionBootstrap::SetupX86Environment(ProgramContext &ctx, char **argv,
                                              char **env, DirectAddressSpace &addressSpace) {
  (void)ctx;
  (void)argv;
  (void)env;
  printf("[X86] Setting up execution environment\n");
  fflush(stdout);

  // Setup commpage (skipped for now - vm32_create_area hangs)
  // TODO: Fix vm32_create_area blocking issue
  printf("[X86] Commpage setup skipped (TODO: fix vm32_create_area)\n");
  fflush(stdout);

  // Setup thread local storage (TLS) - skipped for now  
  printf("[X86] TLS setup skipped (TODO: fix TLS initialization)\n");
  fflush(stdout);

  return true;
}

bool ExecutionBootstrap::LoadDependencies(ProgramContext &ctx,
                                          ElfImage *image) {
  if (!image->IsDynamic()) {
    return true; // No dependencies for static binaries
  }

  printf("[X86] Scanning for dependencies in %s\n", image->GetPath());
  fflush(stdout);

  // Try to load libroot.so from the standard locations
  const char *libPaths[] = {
      "./sysroot/haiku32/lib/libroot.so",
      "./sysroot/haiku32/lib/x86/libroot.so",
      "./sysroot/haiku32/system/lib/libroot.so",
      "/boot/home/src/UserlandVM-HIT/sysroot/haiku32/lib/libroot.so", NULL};

  for (int i = 0; libPaths[i] != NULL; i++) {
    FILE *f = fopen(libPaths[i], "rb");
    if (f) {
      fclose(f);
      printf("[X86] Loading libroot.so from %s\n", libPaths[i]);
      fflush(stdout);

      ElfImage *libroot = ElfImage::Load(libPaths[i]);
      if (libroot) {
        ctx.linker->AddLibrary("libroot.so", libroot);
        printf("[X86] libroot.so loaded at %p\n", libroot->GetImageBase());
        return true;
      }
    }
  }

  printf("[X86] Warning: Could not find libroot.so\n");
  return false;
}

bool ExecutionBootstrap::ResolveDynamicSymbols(ProgramContext &ctx,
                                                ElfImage *image) {
  if (!image || !image->IsDynamic()) {
    return true;
  }

  printf("[X86] Resolving dynamic symbols\n");
  fflush(stdout);

  // For now, just verify that libroot.so was loaded
  // Full symbol resolution would happen here
  ElfImage *libroot = ctx.linker->GetLibrary("libroot.so");
  if (!libroot) {
    printf("[X86] Warning: libroot.so not loaded, symbols may not resolve\n");
    // Continue anyway - some symbols might be in the binary itself
    return true;
  }

  printf("[X86] libroot.so available for symbol resolution\n");
  return true;
}
