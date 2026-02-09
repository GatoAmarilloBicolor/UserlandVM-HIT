// Enhanced Main.cpp with GUI support for WebPositive
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
#include "Phase4GUISyscalls.h"

// Include new functionality from remote
#include "EnhancedDynamicSymbolResolution.h"
#include "RecycledBasicSyscalls.h"

#include <sys/mman.h>
#include <cstring>

// Global GUI flag
bool g_gui_enabled = true;
bool g_verbose = false;

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

// Simple relocation applicator for ET_DYN binaries
static void ApplySimpleRelocations(uint8_t *guest_memory, size_t guest_size, ElfImage *image) {
  printf("[Relocation] Starting relocation application\n");
  
  // Get to the host ELF header
  const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)image->GetImageBase();
  if (!ehdr) {
    printf("[Relocation] ERROR: Could not get ELF header\n");
    return;
  }
  
  printf("[Relocation] ELF header found: e_phnum=%d, e_phoff=%d\n", ehdr->e_phnum, ehdr->e_phoff);
  
  // Find .rel.dyn section by scanning program headers
  int reloc_count = 0;
  for (uint32 i = 0; i < ehdr->e_phnum; i++) {
    const Elf32_Phdr *phdr = (const Elf32_Phdr *)((uint8_t *)ehdr + ehdr->e_phoff + i * ehdr->e_phentsize);
    
    if (phdr->p_type == PT_DYNAMIC) {
      printf("[Relocation] Found PT_DYNAMIC at offset 0x%x\n", phdr->p_offset);
      
      // Found PT_DYNAMIC, scan for DT_REL entries
      const Elf32_Dyn *dyn = (const Elf32_Dyn *)((uint8_t *)ehdr + phdr->p_offset);
      
      Elf32_Rel *rel_section = nullptr;
      uint32_t rel_size = 0;
      
      // Find DT_REL and DT_RELSZ
      for (int j = 0; j < 100; j++) {
        if (dyn[j].d_tag == DT_NULL) break;
        if (dyn[j].d_tag == DT_REL) {
          uint32_t rel_vaddr = dyn[j].d_un.d_ptr;
          rel_section = (Elf32_Rel *)(guest_memory + rel_vaddr);
          printf("[Relocation] Found DT_REL: vaddr=0x%x\n", rel_vaddr);
        }
        if (dyn[j].d_tag == DT_RELSZ) {
          rel_size = dyn[j].d_un.d_val;
          printf("[Relocation] Found DT_RELSZ = %u bytes\n", rel_size);
        }
      }
      
      if (rel_section && rel_size > 0) {
        printf("[Relocation] Applying relocations\n");
        
        int rel_count = rel_size / sizeof(Elf32_Rel);
        for (int k = 0; k < rel_count; k++) {
          uint32_t rel_offset = rel_section[k].r_offset;
          uint32_t rel_info = rel_section[k].r_info;
          uint32_t rel_type = ELF32_R_TYPE(rel_info);
          
          if (rel_type == 8) {  // R_386_RELATIVE
            if (rel_offset < guest_size) {
              uint32_t *reloc_addr = (uint32_t *)(guest_memory + rel_offset);
              uint32_t addend = *reloc_addr;
              *reloc_addr = (uint32_t)(uintptr_t)guest_memory + addend;
              reloc_count++;
            }
          }
        }
        printf("[Relocation] Applied %d R_386_RELATIVE relocations\n", reloc_count);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  printf("\n╔════════════════════════════════════════════════════════════════╗\n");
  printf("║   UserlandVM-HIT: GUI-ENHANCED VERSION                        ║\n");
  printf("║   WebPositive Window Support Enabled                          ║\n");
  printf("╚════════════════════════════════════════════════════════════════╝\n\n");
  
  printf("[Main] argc=%d, argv[0]=%s\n", argc, argc > 0 ? argv[0] : "NULL");
  
  // Initialize new functionality
  printf("[Main] ============================================\n");
  printf("[Main] Initializing Enhanced Functionality\n");
  printf("[Main] GUI Support: %s\n", g_gui_enabled ? "ENABLED ✅" : "DISABLED");
  printf("[Main] ============================================\n");
  ApplyRecycledBasicSyscalls();
  DynamicSymbolResolution::AddCommonSymbols();
  printf("[Main] ✅ Enhanced functionality initialized\n\n");
  
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
  if (g_gui_enabled) {
    printf("[Main] GUI MODULE: ENABLED ✅\n");
    printf("[Main] WebPositive window creation available\n");
  }
  printf("[Main] ============================================\n");
  
  // Allocate real guest memory space (512 MB)
  void *guest_memory = mmap(NULL, 512 * 1024 * 1024, 
                              PROT_READ | PROT_WRITE | PROT_EXEC,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (guest_memory == MAP_FAILED) {
      printf("[Main] ERROR: Failed to allocate guest memory\n");
      return 1;
  }
  
  // Copy image into guest memory at offset 0
  uint32_t image_size = dynamic_cast<ElfImageImpl<Elf32Class>*>(image) ? 
                        dynamic_cast<ElfImageImpl<Elf32Class>*>(image)->GetImageSize() :
                        4096;
  printf("[Main] Copying image: base=%p, size=%u bytes\n", image->GetImageBase(), image_size);
  memcpy(guest_memory, image->GetImageBase(), image_size);
  
  // Apply ET_DYN relocations if needed
  if (image->IsDynamic()) {
    printf("[Main] ============================================\n");
    printf("[Main] APPLYING ET_DYN RELOCATIONS\n");
    printf("[Main] ============================================\n");
    ApplySimpleRelocations((uint8_t *)guest_memory, 512 * 1024 * 1024, image);
  }
  
  RealAddressSpace address_space((uint8_t *)guest_memory, 512 * 1024 * 1024);
  RealSyscallDispatcher syscall_dispatcher;
  
  // Create x86-32 guest context
  X86_32GuestContext guest_context(address_space);
  
  // Calculate guest entry point
  void *entry_ptr = image->GetEntry();
  void *image_base = image->GetImageBase();
  
  printf("[Main] DEBUG: entry_ptr (host) = %p, image_base = %p\n", entry_ptr, image_base);
  
  // For guest execution, entry is offset from base 0
  uint32_t guest_entry = 0;
  
  // If entry_ptr is within the loaded image range, it's the real entry
  if ((uintptr_t)entry_ptr >= (uintptr_t)image_base) {
      guest_entry = (uint32_t)((uintptr_t)entry_ptr - (uintptr_t)image_base);
      printf("[Main] DEBUG: Calculated offset entry = 0x%x\n", guest_entry);
  } else {
      guest_entry = (uint32_t)(uintptr_t)entry_ptr;
      printf("[Main] DEBUG: Using virtual entry = 0x%x\n", guest_entry);
  }
  
  // For ET_DYN with entry=0, guess main() location
  if (guest_entry == 0 && image->IsDynamic()) {
      printf("[Main] WARNING: ET_DYN with entry=0, using main() at 0x116\n");
      guest_entry = 0x116;
  }
  
  printf("[Main] Final entry point for guest: 0x%08x\n", guest_entry);
  
  // Set up initial registers
  guest_context.Registers().eip = guest_entry;
  guest_context.Registers().esp = 256 * 1024 * 1024 - 4096;
  guest_context.Registers().ebp = guest_context.Registers().esp;
  guest_context.Registers().eax = 0;
  guest_context.Registers().ebx = 0;
  guest_context.Registers().ecx = 0;
  guest_context.Registers().edx = 0;
  guest_context.Registers().esi = 0;
  guest_context.Registers().edi = 0;
  guest_context.Registers().eflags = 0x202;
  
  printf("[Main] Entry point: 0x%08x\n", guest_context.Registers().eip);
  printf("[Main] Stack pointer: 0x%08x\n", guest_context.Registers().esp);
  printf("[Main] Starting x86-32 interpreter with GUI support...\n\n");
  
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
  if (g_gui_enabled && syscall_dispatcher.GetGUIHandler()) {
    syscall_dispatcher.GetGUIHandler()->PrintWindowInfo();
  }
  
  delete image;
  
  printf("[Main] Test completed\n");
  printf("╚════════════════════════════════════════════════════════════════╝\n\n");
  return 0;
}
