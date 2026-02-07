/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "GuestElfLoader.h"
#include "AddressSpace.h"
#include "DynamicLinker.h"
#include "ElfDynamic.h"
#include "GuestMemoryAllocator.h"
#include "TLSSetup.h"
#include "X86_32GuestContext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Sprint 2: Basic ELF loader stub
// Full functionality will be implemented in Sprint 3

status_t GuestElfLoader::Load(const char *path, X86_32GuestContext &context,
                              AddressSpace &addressSpace, int argc, char **argv,
                              char **envp, SymbolResolver *symbol_resolver) {
  if (!path)
    return B_BAD_VALUE;

  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("Error: Cannot open file %s\n", path);
    return B_ERROR;
  }

  // Read ELF header
  unsigned char ident[16];
  if (fread(ident, 1, 16, f) != 16) {
    fclose(f);
    printf("Error: Cannot read ELF header\n");
    return B_ERROR;
  }

  // Verify ELF magic
  if (ident[0] != 0x7f || ident[1] != 'E' || ident[2] != 'L' ||
      ident[3] != 'F') {
    printf("Error: Not an ELF file\n");
    fclose(f);
    return B_BAD_VALUE;
  }

  // Parse ELF header
  fseek(f, 0, SEEK_SET);
  uint32_t e_entry = 0;
  uint32_t e_phoff = 0;
  uint16_t e_type = 0;
  uint16_t e_phentsize = 0;
  uint16_t e_phnum = 0;

  // ELF32 structure:
  // Offsets from start (0x00):
  // 0x00-0x0F: e_ident
  // 0x10-0x11: e_type (ET_EXEC=2, ET_DYN=3)
  // 0x12-0x13: e_machine
  // 0x14-0x17: e_version
  // 0x18-0x1B: e_entry (ENTRY POINT)
  // 0x1C-0x1F: e_phoff
  // 0x20-0x23: e_shoff
  // 0x24-0x27: e_flags
  // 0x28-0x29: e_ehsize
  // 0x2A-0x2B: e_phentsize
  // 0x2C-0x2D: e_phnum

  // Read e_type (offset 0x10) - to detect dynamic binaries
  fseek(f, 0x10, SEEK_SET);
  fread(&e_type, 2, 1, f);

  // Read entry point (offset 0x18)
  fseek(f, 0x18, SEEK_SET);
  fread(&e_entry, 4, 1, f);

  // Read program header offset (offset 0x1C)
  fseek(f, 0x1C, SEEK_SET);
  fread(&e_phoff, 4, 1, f);

  // Read program header entry size (offset 0x2A)
  fseek(f, 0x2A, SEEK_SET);
  fread(&e_phentsize, 2, 1, f);

  // Read number of program headers (offset 0x2C)
  fseek(f, 0x2C, SEEK_SET);
  fread(&e_phnum, 2, 1, f);

  // Detect PT_INTERP (type 3)
  char interpreter_path[256];
  bool has_interpreter = false;

  for (uint16_t i = 0; i < e_phnum; i++) {
    uint32_t ph_start = e_phoff + (i * e_phentsize);
    fseek(f, ph_start, SEEK_SET);
    uint32_t p_type, p_offset, p_filesz;
    fread(&p_type, 4, 1, f);
    fread(&p_offset, 4, 1, f);
    fseek(f, ph_start + 16, SEEK_SET); // skip to p_filesz
    fread(&p_filesz, 4, 1, f);

    if (p_type == 3) { // PT_INTERP
      fseek(f, p_offset, SEEK_SET);
      uint32_t to_read = (p_filesz < sizeof(interpreter_path) - 1)
                             ? p_filesz
                             : sizeof(interpreter_path) - 1;
      fread(interpreter_path, 1, to_read, f);
      interpreter_path[to_read] = '\0';
      has_interpreter = true;
      printf("[ELFLoader] Found interpreter: %s\n", interpreter_path);
      break;
    }
  }

  fclose(f);

  if (has_interpreter) {
    printf("[ELFLoader] Delegating to interpreter: %s\n", interpreter_path);
    // In a real Haiku environment, we would load the interpreter and jump to
    // it. For now, let the DynamicLinker handle the heavy lifting of loading
    // dependencies if ET_DYN, but eventually we should just load
    // 'runtime_loader'.
  }

  // DELEGACIÓN: Si es ET_DYN, usar DynamicLinker
  if (e_type == 3) { // ET_DYN
    printf(
        "[ELFLoader] DYNAMIC BINARY (ET_DYN) - delegating to DynamicLinker\n");

    DynamicLinker linker(addressSpace, context);
    uint32_t actual_entry = 0;

    status_t dyn_status = linker.LoadDynamicBinary(path, actual_entry);
    if (dyn_status != B_OK) {
      printf("[ELFLoader] DynamicLinker failed: %d\n", dyn_status);
      return dyn_status;
    }

    // Setup stack (increase size to account for local variables and function
    // arguments) Use a larger stack to account for parameters above stackBase
    uint32_t stackBase =
        0xC0000000 - 32768; // Leave 32KB above for parameters/arguments
    uint32_t stackSize = 4 * 1024 * 1024 + 32768; // 4 MB + 32KB extra
    uint32_t stackOffset = 128 * 1024 * 1024;
    // Register one extra page to handle address calculations slightly above
    // stackBase
    addressSpace.RegisterMapping(stackBase - stackSize, stackOffset,
                                 stackSize + 4096);

    // Setup TLS area for guest
    printf("[+] Setting up TLS area for guest\n");
    status_t tls_status =
        TLSSetup::Initialize(addressSpace, 1); // thread_id = 1
    if (tls_status != B_OK) {
      fprintf(stderr,
              "[ELFLoader] Warning: TLS setup failed, continuing anyway: %s\n",
              strerror(tls_status));
      // Not fatal - some programs may not need TLS
    }

    // Setup context
    // Initialize all general purpose registers to zero to avoid undefined behavior
    context.Registers().eax = 0;
    context.Registers().ebx = 0;
    context.Registers().ecx = 0;
    context.Registers().edx = 0;
    context.Registers().esi = 0;
    context.Registers().edi = 0;
    context.Registers().ebp = 0; // EBP should be NULL initially
    context.Registers().eip = actual_entry;
    context.Registers().esp = stackBase;

    // Initialize stack with basic argument structure
    // argc, argv[], envp[], auxv[] are expected at the bottom of the stack
    // For now, set up a minimal structure with argc=1 and argv[0]="program"
    uint32_t stack_ptr = stackBase;

    // Push argc (1 for the program name)
    uint32_t argc_val = 1;
    stack_ptr -= 4;
    addressSpace.Write(stack_ptr, &argc_val, 4);

    // Push argv[0] pointer (points to program name string on stack)
    uint32_t argv0_ptr = stack_ptr - 256; // Leave space for strings
    stack_ptr -= 4;
    addressSpace.Write(stack_ptr, &argv0_ptr, 4);

    // Push NULL terminator for argv
    uint32_t null_ptr = 0;
    stack_ptr -= 4;
    addressSpace.Write(stack_ptr, &null_ptr, 4);

    // Write program name string
    const char *prog_name = "pwd";
    addressSpace.Write(argv0_ptr, (void *)prog_name, strlen(prog_name) + 1);

    // Update ESP to point to argc
    context.Registers().esp = stack_ptr;

    printf(
        "[ELFLoader] Stack initialized: argc=1, argv[0]=\"%s\", ESP=0x%08x\n",
        prog_name, stack_ptr);

    printf("[ELFLoader] Dynamic binary ready: entry=0x%08x\n", actual_entry);
    return B_OK;
  }

  // STATIC: Continuar con loading manual para ET_EXEC
  printf("[ELFLoader] STATIC BINARY (ET_EXEC) - manual loading\n");

  f = fopen(path, "rb");
  if (!f) {
    printf("Error: Cannot reopen file %s\n", path);
    return B_ERROR;
  }

  uint32_t load_base = 0; // ET_EXEC: use addresses as-is

  // FIXED: Use global memory allocator to prevent overlap
  // This ensures all binaries (ET_EXEC and libraries) use non-overlapping
  // memory
  GuestMemoryAllocator &allocator = GuestMemoryAllocator::Get();

  // For ET_DYN, we'll need to read relocations from DYNAMIC segment
  // But first, load all PT_LOAD segments

  // Load program segments (PT_LOAD only)
  for (uint16_t i = 0; i < e_phnum; i++) {
    uint32_t ph_start = e_phoff + (i * e_phentsize);
    fseek(f, ph_start, SEEK_SET);

    uint32_t p_type = 0, p_offset = 0, p_vaddr = 0, p_paddr = 0;
    uint32_t p_filesz = 0, p_memsz = 0, p_flags = 0, p_align = 0;

    // Program header (ELF32) is 32 bytes:
    // 0x00-0x03: p_type
    // 0x04-0x07: p_offset
    // 0x08-0x0B: p_vaddr (relative to load base for ET_DYN)
    // 0x0C-0x0F: p_paddr
    // 0x10-0x13: p_filesz
    // 0x14-0x17: p_memsz
    // 0x18-0x1B: p_flags
    // 0x1C-0x1F: p_align

    fread(&p_type, 4, 1, f);   // +0
    fread(&p_offset, 4, 1, f); // +4
    fread(&p_vaddr, 4, 1, f);  // +8
    fread(&p_paddr, 4, 1, f);  // +12
    fread(&p_filesz, 4, 1, f); // +16
    fread(&p_memsz, 4, 1, f);  // +20

    if (p_type != 1)
      continue; // PT_LOAD = 1

    // For ET_DYN, add load_base to virtual address
    uint32_t actual_vaddr = p_vaddr + load_base;

    printf("[ELFLoader] PT_LOAD: vaddr=0x%08x (orig) → 0x%08x (with base), "
           "filesz=%u, memsz=%u\n",
           p_vaddr, actual_vaddr, p_filesz, p_memsz);

    // Leer datos del archivo
    uint8_t buffer[4096];
    uint32_t remaining = p_filesz;

    // Mapeo relativo: El addressSpace de guest comienza en 0x00000000
    // Los binarios x86-32 típicamente usan 0x08048000 como base
    // Para ET_DYN, usamos 0x40000000 como base
    // Mapeamos dirección virtual a offset en memoria host
    // Todos los segmentos PT_LOAD se cargan secuencialmente

    // Use global allocator to ensure no overlap
    uint32_t guest_offset = allocator.Allocate(p_memsz);
    uint32_t guest_addr = actual_vaddr; // Usar dirección virtual, no offset

    // Registrar esta sección en el mapa de traducción de direcciones
    // Mapear la dirección virtual (con load_base) al offset en memoria guest
    addressSpace.RegisterMapping(actual_vaddr, guest_offset, p_memsz);

    printf("[ELFLoader] Loading to guest offset: 0x%08x (from vaddr=0x%08x)\n",
           guest_offset, p_vaddr);

    fseek(f, p_offset, SEEK_SET);
    bool load_error = false;
    while (remaining > 0) {
      uint32_t to_read =
          (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
      size_t read_bytes = fread(buffer, 1, to_read, f);

      if (read_bytes == 0)
        break;

      // Escribir a guest memory
      status_t status = addressSpace.Write(guest_addr, buffer, read_bytes);
      if (status != B_OK) {
        printf("[ELFLoader] Failed to write to guest memory at offset 0x%08x\n",
               guest_addr);
        load_error = true;
        break;
      }

      guest_addr += read_bytes;
      remaining -= read_bytes;
    }

    if (load_error) {
      fclose(f);
      return B_ERROR;
    }

    // Zero fill if memsz > filesz
    if (p_memsz > p_filesz) {
      uint8_t zeros[256] = {0};
      uint32_t to_fill = p_memsz - p_filesz;
      while (to_fill > 0) {
        uint32_t fill_amount =
            (to_fill > sizeof(zeros)) ? sizeof(zeros) : to_fill;
        addressSpace.Write(guest_addr, zeros, fill_amount);
        guest_addr += fill_amount;
        to_fill -= fill_amount;
      }
    }
  }

  // Dynamic binaries are now handled above by DynamicLinker

  // Initialize stack region (4 MB + 32KB for more headroom)
  // Register the stack as a writable memory region
  // Use a larger stack to account for parameters above stackBase
  uint32_t stackBase =
      0xC0000000 - 32768; // Leave 32KB above for parameters/arguments
  uint32_t stackSize = 4 * 1024 * 1024 + 32768; // 4 MB + 32KB extra
  // FIXED: Stack offset should be dynamically allocated using the global
  // allocator This ensures the stack doesn't overlap with code/data segments
  uint32_t stackOffset = allocator.Allocate(stackSize + 4096);

  // Register one extra page to handle address calculations slightly above
  // stackBase
  addressSpace.RegisterMapping(stackBase - stackSize, stackOffset,
                               stackSize + 4096);

  // Initialize context registers
  // For ET_DYN binaries, entry point is already relative to load base
  // No need to add load_base again - this causes double addition
  uint32_t actual_entry;
  if (fHeader.e_type == ET_DYN) {
    // For ET_DYN, entry point should already be relative to mapped base
    actual_entry = e_entry;  // Entry is already correct
    printf("[ELFLoader] ET_DYN: Using relative entry point 0x%08x\n", e_entry);
  } else {
    // For ET_EXEC, we need to add load base
    actual_entry = e_entry + load_base;
    printf("[ELFLoader] ET_EXEC: Using absolute entry point 0x%08x + 0x%08x = 0x%08x\n", e_entry, load_base, actual_entry);
  }
  
  context.Registers().eip = actual_entry;
  context.Registers().esp = stackBase;
  context.Registers().ebp = 0; // EBP should be NULL initially

  printf("[ELFLoader] ELF loaded successfully: entry=0x%08x (actual: 0x%08x), "
         "stack=0x%08x\n",
         e_entry, actual_entry, stackBase);
  printf("[ELFLoader] Stack region registered: 0x%08x-0x%08x (offset 0x%08x)\n",
         stackBase - stackSize, stackBase, stackOffset);

  fclose(f);
  return B_OK;
}

// Process relocations for ET_DYN binaries
status_t GuestElfLoader::ProcessRelocations(
    FILE *f, AddressSpace &addressSpace, uint32_t load_base, uint32_t rel_addr,
    uint32_t rel_size, uint32_t rel_entry_size, SymbolResolver *symbol_resolver,
    const char *strtab, const Elf32_Sym *symtab, uint32_t symcount) {
  if (rel_size == 0 || rel_entry_size == 0) {
    return B_OK; // No relocations
  }

  uint32_t num_relocations = rel_size / rel_entry_size;
  printf("[ELFLoader] Processing %u relocations at offset 0x%08x\n",
         num_relocations, rel_addr);

  // Process each relocation entry
  // Relocation entry format (8 bytes for REL):
  // Offset 0-3: r_offset (address to relocate)
  // Offset 4-7: r_info (high 24 bits = symbol index, low 8 bits = type)

  for (uint32_t i = 0; i < num_relocations; i++) {
    uint32_t r_offset = 0, r_info = 0;

    // Read relocation entry from file
    fseek(f, rel_addr + (i * rel_entry_size), SEEK_SET);
    if (fread(&r_offset, 4, 1, f) != 1)
      break;
    if (fread(&r_info, 4, 1, f) != 1)
      break;

    uint32_t r_type = r_info & 0xFF;
    uint32_t r_sym = r_info >> 8;

    // For now, only handle R_386_RELATIVE which is most common for ET_DYN
    if (r_type == R_386_RELATIVE) {
      // R_386_RELATIVE: B + A
      // B = load_base, A = addend (read from location)
      uint32_t reloc_addr = r_offset + load_base;

      // Read the addend from the relocation location
      uint32_t addend = 0;
      status_t status = addressSpace.Read(reloc_addr, &addend, 4);
      if (status != B_OK) {
        printf("[ELFLoader] Warning: Failed to read addend at 0x%08x\n",
               reloc_addr);
        continue;
      }

      // Calculate relocation value: load_base + addend
      uint32_t reloc_value = load_base + addend;

      // Write the relocated value back
      status = addressSpace.Write(reloc_addr, &reloc_value, 4);
      if (status != B_OK) {
        printf("[ELFLoader] Warning: Failed to write relocation at 0x%08x\n",
               reloc_addr);
        continue;
      }

      if (i < 5) { // Print first few for debugging
        printf("[ELFLoader] R_386_RELATIVE: offset=0x%08x, value=0x%08x "
               "(base=0x%08x, addend=0x%08x)\n",
               reloc_addr, reloc_value, load_base, addend);
      }
    } else if (r_type == R_386_GLOB_DAT || r_type == R_386_JMP_SLOT) {
      // R_386_GLOB_DAT / R_386_JUMP_SLOT: symbol resolution needed
      // Use SymbolResolver if available, otherwise zero the entry
      uint32_t reloc_addr = r_offset + load_base;
      uint32_t reloc_value = 0;

      // Try to resolve symbol using SymbolResolver
      if (symbol_resolver && symtab && strtab && r_sym < symcount) {
        const Elf32_Sym &sym = symtab[r_sym];
        if (sym.st_name > 0 && strtab) {
          const char *sym_name = &strtab[sym.st_name];
          reloc_value = symbol_resolver->ResolveSymbol(sym_name);

          if (reloc_value != 0) {
            printf("[ELFLoader] Resolved %s symbol '%s' to 0x%08x\n",
                   (r_type == R_386_GLOB_DAT) ? "GLOB_DAT" : "JMP_SLOT",
                   sym_name, reloc_value);
          } else {
            printf("[ELFLoader] Warning: Could not resolve %s symbol '%s', "
                   "using NULL\n",
                   (r_type == R_386_GLOB_DAT) ? "GLOB_DAT" : "JMP_SLOT",
                   sym_name);
          }
        }
      }

      status_t status = addressSpace.Write(reloc_addr, &reloc_value, 4);
      if (status != B_OK) {
        printf("[ELFLoader] Warning: Failed to write relocation at 0x%08x\n",
               reloc_addr);
        continue;
      }

      if (i < 5) {
        printf("[ELFLoader] %s: offset=0x%08x, symbol=%u, value=0x%08x\n",
               (r_type == R_386_GLOB_DAT) ? "R_386_GLOB_DAT" : "R_386_JMP_SLOT",
               reloc_addr, r_sym, reloc_value);
      }
    } else if (r_type == R_386_COPY) {
      // R_386_COPY: copy symbol data from shared library
      // For now, just write zero
      uint32_t reloc_addr = r_offset + load_base;
      // Skip - handled by dynamic linker typically
      if (i < 5) {
        printf("[ELFLoader] R_386_COPY: offset=0x%08x, symbol=%u (skipped)\n",
               r_offset, r_sym);
      }
    }
    // Other relocation types ignored for now
  }

  return B_OK;
}
