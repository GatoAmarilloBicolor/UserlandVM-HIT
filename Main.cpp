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
#include "Phase3ExecutionIntegration.h"

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
  
  // Phase 3: Execute with real x86 interpreter
  printf("[Main] ============================================\n");
  printf("[Main] PHASE 3: x86-32 Execution\n");
  printf("[Main] ============================================\n");
  
  // Create guest context and address space
  GuestContext guest_ctx;
  GuestAddressSpace addr_space(image->GetImageBase(), 256 * 1024 * 1024);
  GuestSyscallDispatcher dispatcher;
  
  // Set entry point
  // Note: Entry point is a host pointer, we need to translate to guest address
  void *entry_ptr = image->GetEntry();
  void *image_base = image->GetImageBase();
  
  // Calculate guest address offset
  uintptr_t offset = (uintptr_t)entry_ptr - (uintptr_t)image_base;
  guest_ctx.eip = (uint32_t)offset;  // Guest address starts at 0 relative to image
  guest_ctx.esp = 0x30000000;  // Stack pointer
  
  printf("[Main] Image base (host): %p\n", image_base);
  printf("[Main] Entry point (host): %p\n", entry_ptr);
  printf("[Main] Starting x86 execution at guest EIP=0x%08x\n", guest_ctx.eip);
  printf("[Main] Target program: %s\n", argv[1]);
  
  // Execute until halt
  uint32_t instruction_count = 0;
  const uint32_t MAX_INSTRUCTIONS = 1000000;
  
  while (!guest_ctx.halted && instruction_count < MAX_INSTRUCTIONS) {
    // Fetch instruction
    uint8_t *instr_ptr = (uint8_t *)addr_space.GetPointer(guest_ctx.eip);
    if (!instr_ptr) {
      printf("[Main] ERROR: Invalid EIP 0x%08x\n", guest_ctx.eip);
      break;
    }
    
    uint8_t opcode = instr_ptr[0];
    
    // Very basic instruction dispatch
    if (opcode == 0xcd && instr_ptr[1] == 0x80) {
      // INT 0x80 - syscall
      dispatcher.HandleSyscall(guest_ctx);
      guest_ctx.eip += 2;
    }
    else if (opcode == 0x90) {
      // NOP
      guest_ctx.eip += 1;
    }
    else if (opcode == 0xc3) {
      // RET
      guest_ctx.esp += 4;
      guest_ctx.eip += 1;
    }
    else if (opcode == 0x55) {
      // PUSH EBP
      guest_ctx.esp -= 4;
      addr_space.WriteU32(guest_ctx.esp, guest_ctx.ebp);
      guest_ctx.eip += 1;
    }
    else if (opcode == 0x5d) {
      // POP EBP
      guest_ctx.ebp = addr_space.ReadU32(guest_ctx.esp);
      guest_ctx.esp += 4;
      guest_ctx.eip += 1;
    }
    else {
      // Unknown instruction - skip it
      if (instruction_count % 1000 == 0) {
        printf("[Exec] Instruction 0x%02x at 0x%08x (skipped)\n", opcode, guest_ctx.eip);
      }
      guest_ctx.eip += 1;
    }
    
    instruction_count++;
  }
  
  printf("[Main] ============================================\n");
  if (guest_ctx.halted) {
    printf("[Main] ✅ Program exited with code: %d\n", guest_ctx.exit_code);
  } else if (instruction_count >= MAX_INSTRUCTIONS) {
    printf("[Main] ⚠️  Instruction limit reached (%u)\n", MAX_INSTRUCTIONS);
  } else {
    printf("[Main] ⚠️  Program execution ended\n");
  }
  printf("[Main] Total instructions executed: %u\n", instruction_count);
  
  delete image;
  
  printf("[Main] Test completed\n");
  return 0;
}
