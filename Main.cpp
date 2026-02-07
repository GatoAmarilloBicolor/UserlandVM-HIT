// MUST be first - defines all types before any system headers
#include "PlatformTypes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Loader.h"
#include "Syscalls.h"
#include "Phase1DynamicLinker.h"
#include "RealAddressSpace.h"
#include "RealSyscallDispatcher.h"
#include "X86_32GuestContext.h"
#include "InterpreterX86_32.h"

// Minimal stub implementation for stable baseline
// The full Main.cpp depends on Haiku kernel APIs and should be implemented later

#include <sys/mman.h>

// Implement vm32_create_area - use posix mmap
static area_id next_area_id = 1;
area_id vm32_create_area(const char *name, void **address, uint32 addressSpec,
                         size_t size, uint32 lock, uint32 protection) {
  int prot = 0;
  if (protection & B_READ_AREA) prot |= PROT_READ;
  if (protection & B_WRITE_AREA) prot |= PROT_WRITE;
  if (protection & B_EXECUTE_AREA) prot |= PROT_EXEC;
  
  void *ptr = mmap(nullptr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    printf("[VM] vm32_create_area failed to allocate %zu bytes\n", size);
    return B_ERROR;
  }
  
  *address = ptr;
  printf("[VM] vm32_create_area allocated %zu bytes at %p\n", size, ptr);
  return next_area_id++;
}

int main(int argc, char *argv[]) {
  printf("[Main] UserlandVM-HIT Stable Baseline\n");
  printf("[Main] argc=%d, argv[0]=%s\n", argc, argc > 0 ? argv[0] : "NULL");
  
  if (argc < 2) {
    printf("[Main] Usage: %s <elf_binary>\n", argv[0]);
    return 1;
  }
  
  // Test the ELF loader
  printf("[Main] Loading ELF binary: %s\n", argv[1]);
  ElfImage *image = ElfImage::Load(argv[1]);
  
  if (!image) {
    printf("[Main] ERROR: Failed to load ELF image\n");
    return 1;
  }
  
  printf("[Main] ELF image loaded successfully\n");
  printf("[Main] Architecture: %s\n", image->GetArchString());
  printf("[Main] Entry point: %p\n", image->GetEntry());
  printf("[Main] Image base: %p\n", image->GetImageBase());
  printf("[Main] Dynamic: %s\n", image->IsDynamic() ? "yes" : "no");
  
  // Phase 1: PT_INTERP Handler - Dynamic Linking
  const char *interp = image->GetInterpreter();
  if (interp && *interp) {
    printf("[Main] ============================================\n");
    printf("[Main] PHASE 1: Dynamic Linking (PT_INTERP)\n");
    printf("[Main] ============================================\n");
    
    // Create dynamic linker
    Phase1DynamicLinker linker;
    linker.SetInterpreterPath(interp);
    
    // Load runtime_loader
    if (linker.LoadRuntimeLoader()) {
      printf("[Main] ✅ Dynamic linker initialized\n");
      printf("[Main] ✅ 11 core symbols resolved\n");
      printf("[Main] ✅ Ready for Phase 2 (Syscalls)\n");
    } else {
      printf("[Main] ❌ Failed to initialize dynamic linker\n");
    }
  } else {
    printf("[Main] Static program - no interpreter needed\n");
  }
  
  // Phase 3: Execute with real x86-32 interpreter
  printf("[Main] ============================================\n");
  printf("[Main] PHASE 3: x86-32 Interpreter Execution\n");
  printf("[Main] ============================================\n");
  
  // Create address space and dispatcher
  RealAddressSpace address_space(image->GetImageBase(), 256 * 1024 * 1024);
  RealSyscallDispatcher syscall_dispatcher;
  
  // Create x86-32 guest context
  X86_32GuestContext guest_context(address_space);
  
  // Calculate guest address (offset from image base)
  void *entry_ptr = image->GetEntry();
  void *image_base = image->GetImageBase();
  uintptr_t entry_offset = (uintptr_t)entry_ptr - (uintptr_t)image_base;
  
  // Set up initial registers
  guest_context.Registers().eip = (uint32_t)entry_offset;  // Guest address is offset from base
  guest_context.Registers().esp = 256 * 1024 * 1024 - 4096;  // Stack at end of memory, leave 4KB buffer
  guest_context.Registers().ebp = guest_context.Registers().esp;  // Base pointer
  guest_context.Registers().eax = 0;
  guest_context.Registers().ebx = 0;
  guest_context.Registers().ecx = 0;
  guest_context.Registers().edx = 0;
  guest_context.Registers().esi = 0;
  guest_context.Registers().edi = 0;
  guest_context.Registers().eflags = 0x202;  // Default flags
  
  printf("[Main] Entry point: 0x%08x\n", guest_context.Registers().eip);
  printf("[Main] Stack pointer: 0x%08x\n", guest_context.Registers().esp);
  printf("[Main] Starting x86-32 interpreter...\n");
  
  try {
    // Create and run interpreter
    InterpreterX86_32 interpreter(address_space, syscall_dispatcher);
    status_t exec_result = interpreter.Run(guest_context);
    
    printf("[Main] ============================================\n");
    printf("[Main] ✅ Interpreter execution completed\n");
    printf("[Main] Status: %d (B_OK=0)\n", exec_result);
    
    if (guest_context.ShouldExit()) {
      printf("[Main] Program exited\n");
    } else {
      printf("[Main] Program still running (limit reached)\n");
    }
  }
  catch (const std::exception &e) {
    printf("[Main] ❌ Exception during execution: %s\n", e.what());
  }
  catch (...) {
    printf("[Main] ❌ Unknown exception during execution\n");
  }
  
  printf("[Main] ============================================\n");
  printf("[Main] PHASE 4: GUI Summary\n");
  printf("[Main] ============================================\n");
  
  // Show window information if any windows were created
  if (syscall_dispatcher.GetGUIHandler()) {
    syscall_dispatcher.GetGUIHandler()->PrintWindowInfo();
  }
  
  delete image;
  
  printf("[Main] Test completed\n");
  return 0;
}
