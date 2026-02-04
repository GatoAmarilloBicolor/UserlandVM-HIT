#pragma once

#include <OS.h>
#include <elf.h>
#include <cstdint>

class ElfImage;
class DynamicLinker;

/**
 * RelocationProcessor handles applying ELF relocations (GOT/PLT patching)
 * for dynamically linked x86-32 binaries on x86-64 host.
 *
 * Supports:
 * - R_386_32      : Direct 32-bit address relocation
 * - R_386_PC32    : PC-relative 32-bit relocation
 * - R_386_GLOB_DAT: Global Offset Table entry
 * - R_386_JMP_SLOT: Procedure Linkage Table entry
 * - R_386_RELATIVE: Relative relocation (for ASLR)
 */
class RelocationProcessor {
public:
  RelocationProcessor(DynamicLinker *linker);
  ~RelocationProcessor();

  /**
   * Process all relocations for a loaded image.
   * This should be called after the image is loaded but before execution.
   */
  status_t ProcessRelocations(ElfImage *image);

private:
  DynamicLinker *fLinker;

  /**
   * Apply a single relocation entry.
   */
  status_t ApplyRelocation(
    ElfImage *image,
    uint32_t reloc_addr,
    uint32_t sym_value,
    uint32_t reloc_type,
    uint32_t addend = 0
  );

  /**
   * Resolve a symbol for relocation.
   */
  uint32_t ResolveSymbol(ElfImage *image, const char *name);
};
