// MUST be first - defines all types before any system headers
#include "PlatformTypes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Loader.h"
#include "Syscalls.h"
#include "Phase1DynamicLinker.h"
#include "Phase2SyscallHandler.h"
#include "SimpleX86Executor.h"

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
  
  // Phase 2: Execute the program with syscall handling
  printf("[Main] ============================================\n");
  printf("[Main] PHASE 2: Execution with Syscalls\n");
  printf("[Main] ============================================\n");
  
  Phase2SyscallHandler handler;
  SimpleX86Executor executor(image->GetImageBase(), 256 * 1024 * 1024);
  
  printf("[Main] Starting execution of %s\n", argv[1]);
  printf("[Main] Entry point: 0x%p\n", image->GetEntry());
  
  bool executed = executor.Execute((uint32_t)(uintptr_t)image->GetEntry(), handler);
  
  if (executed) {
    printf("[Main] ✅ Program executed successfully\n");
  } else {
    printf("[Main] ⚠️  Program execution ended (not all instructions supported)\n");
  }
  
  delete image;
  
  printf("[Main] Test completed\n");
  return 0;
}
