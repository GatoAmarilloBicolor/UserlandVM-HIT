#include "ExecutionBootstrap.h"
#include "CommpageManager.h"
#include "DirectAddressSpace.h"
#include "DynamicLinker.h"
#include "TLSSetup.h"
#include "VirtualCpuX86Native.h"
#include "X86_32GuestContext.h"
#include "OptimizedX86Executor.h"
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
  ctx.entryPoint = (uint32)(unsigned long long)image->GetEntry();
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

  // Setup environment (Commpage, TLS)
  if (!SetupX86Environment(ctx, argv, env)) {
    fprintf(stderr, "[X86] Failed to setup environment\n");
    return 1;
  }

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
  DirectAddressSpace addressSpace;
  
  // Create X86_32GuestContext for interpreter
  X86_32GuestContext guestContext(addressSpace);
  
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
  regs.eip = ctx.entryPoint;
  regs.eflags = 0x202; // IF and reserved bits
  
  printf("[X86] Guest context initialized\n");
  fflush(stdout);

  // Create syscall dispatcher
  Haiku32SyscallDispatcher syscallDispatcher(&addressSpace);
  
  // Run the interpreter
  printf("[X86] Starting x86-32 interpreter\n");
  fflush(stdout);
  
  // Execute using the interpreter
  OptimizedX86Executor executor(addressSpace, syscallDispatcher);
  uint32 exitCode = 0;
  executor.Execute(guestContext, exitCode);

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
                                             char **env) {
  (void)ctx;
  (void)argv;
  (void)env;
  printf("[X86] Setting up execution environment\n");

  // Create a temporary AddressSpace that maps to the host 32-bit window
  DirectAddressSpace space;

  // Setup commpage
  uint32_t commpageAddr = 0;
  if (CommpageManager::Setupx86Commpage(space, commpageAddr) == B_OK) {
    printf("[X86] Commpage initialized at 0x%08x\n", commpageAddr);
  }

  // Setup thread local storage (TLS)
  printf("[X86] Initializing TLS\n");
  if (TLSSetup::Initialize(space, 1) != B_OK) {
    printf("[X86] WARNING: TLS setup failed\n");
  }

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
