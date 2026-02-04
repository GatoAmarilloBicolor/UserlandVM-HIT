#include "Loader.h"

#define __STDC_FORMAT_MACROS
// Fix for __GLIBC_USE errors
#ifdef __linux__
#include <features.h>
#endif
#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <vector>

#include "Loader.h"

// Platform-specific bypass for Haiku headers on Linux
#define _SYS_TYPES_H                                                           \
  1 // Prevent re-inclusion of sys/types.h from Haiku headers if possible
#include <OS.h>
#include <image_defs.h>
#include <private/system/arch/arm/arch_elf.h>
#include <private/system/arch/riscv64/arch_elf.h>
#include <private/system/arch/x86/arch_elf.h>
#include <private/system/arch/x86_64/arch_elf.h>
#include <private/system/syscalls.h>

area_id vm32_create_area(const char *name, void **address, uint32 addressSpec,
                         size_t size, uint32 lock, uint32 protection);

static bool FileRead(FILE *f, void *data, size_t size) {
  size_t read = fread(data, size, 1, f);
  if (read != 1) {
    printf("[FileRead] ERROR: Expected %zu bytes, got %zu\n", size,
           read * size);
    return false;
  }
  return true;
}

template <typename Class> void ElfImageImpl<Class>::LoadHeaders() {
  printf("[ELF] LoadHeaders starting\n");
  fflush(stdout);

  fseek(fFile.Get(), 0, SEEK_SET);
  if (!FileRead(fFile.Get(), &fHeader, sizeof(fHeader))) {
    printf("[ELF] ERROR: Failed to read ELF header\n");
    abort();
  }

  printf("[ELF] ELF Header loaded: e_machine=%u, e_phnum=%u\n",
         fHeader.e_machine, fHeader.e_phnum);
  fflush(stdout);

  fPhdrs.SetTo(new typename Class::Phdr[fHeader.e_phnum]);
  for (uint32 i = 0; i < fHeader.e_phnum; i++) {
    fseek(fFile.Get(), fHeader.e_phoff + (off_t)i * fHeader.e_phentsize,
          SEEK_SET);
    if (!FileRead(fFile.Get(), &fPhdrs[i], sizeof(typename Class::Phdr))) {
      printf("[ELF] ERROR: Failed to read program header %u\n", i);
      abort();
    }
    printf("[ELF] Program header %u: type=%u, offset=%u\n", i, fPhdrs[i].p_type,
           (uint32)fPhdrs[i].p_offset);
  }
}

template <typename Class> void ElfImageImpl<Class>::LoadSegments() {
  Address minAdr = (Address)(-1), maxAdr = 0;
  for (uint32 i = 0; i < fHeader.e_phnum; i++) {
    typename Class::Phdr &phdr = fPhdrs[i];
    if (phdr.p_type == PT_LOAD) {
      minAdr = std::min<Address>(minAdr, phdr.p_vaddr);
      maxAdr = std::max<Address>(maxAdr, phdr.p_vaddr + phdr.p_memsz - 1);
    }
  }
  fSize = maxAdr - minAdr + 1;

  printf("[ELF] Loading image: min=%#" B_PRIx64 " max=%#" B_PRIx64
         " size=%#" B_PRIx64 "\n",
         (uint64)minAdr, (uint64)maxAdr, (uint64)fSize);

  if constexpr (std::is_same<Class, Elf32Class>()) {
    fArea.SetTo(vm32_create_area("image", &fBase, B_ANY_ADDRESS, fSize,
                                 B_NO_LOCK,
                                 B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA));
  } else {
    fArea.SetTo(create_area("image", &fBase, B_ANY_ADDRESS, fSize, B_NO_LOCK,
                            B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA));
  }

  if (!fArea.IsSet()) {
    printf("[ELF] ERROR: Failed to create area for image\n");
    abort();
  }

  fDelta = (intptr_t)fBase - (intptr_t)minAdr;
  printf("[ELF] Image base: %p, delta: %#" PRIxPTR "\n", fBase,
         (uintptr_t)fDelta);
  fEntry = FromVirt(fHeader.e_entry);
  printf("[ELF] Entry point: %p\n", fEntry);

  for (uint32 i = 0; i < fHeader.e_phnum; i++) {
    typename Class::Phdr &phdr = fPhdrs[i];
    printf("[ELF] Processing segment %u: type=%u\n", i, phdr.p_type);
    fflush(stdout);

    switch (phdr.p_type) {
    case PT_LOAD: {
      printf("[ELF] Loading PT_LOAD segment %u: vaddr=%#" B_PRIx64
             " filesz=%#" B_PRIx64 " memsz=%#" B_PRIx64 " offset=%#" B_PRIx64
             "\n",
             i, (uint64)phdr.p_vaddr, (uint64)phdr.p_filesz,
             (uint64)phdr.p_memsz, (uint64)phdr.p_offset);
      fflush(stdout);

      // Validate addresses are within allocated range
      void *dst = FromVirt(phdr.p_vaddr);
      void *segEnd = (void *)((addr_t)dst + phdr.p_filesz);
      void *areaEnd = (void *)((addr_t)fBase + fSize);

      printf("[ELF] Address check: dst=%p, end=%p, areaBase=%p, areaEnd=%p\n",
             dst, segEnd, fBase, areaEnd);
      fflush(stdout);

      if ((addr_t)dst < (addr_t)fBase || (addr_t)segEnd > (addr_t)areaEnd) {
        printf("[ELF] ERROR: Segment address out of bounds\n");
        printf("[ELF]   dst=%p, end=%p\n", dst, segEnd);
        printf("[ELF]   area=[%p, %p)\n", fBase, areaEnd);
        abort();
      }

      printf("[ELF] Seeking to offset %#" B_PRIx64 "\n", (uint64)phdr.p_offset);
      fflush(stdout);
      fseek(fFile.Get(), phdr.p_offset, SEEK_SET);
      printf("[ELF] Reading %#" B_PRIx64 " bytes to %p\n",
             (uint64)phdr.p_filesz, dst);
      fflush(stdout);

      if (!FileRead(fFile.Get(), dst, phdr.p_filesz)) {
        printf("[ELF] ERROR: Failed to read segment data\n");
        abort();
      }

      // Zero-fill BSS (memsz > filesz)
      if (phdr.p_memsz > phdr.p_filesz) {
        void *bssStart = (void *)((addr_t)dst + phdr.p_filesz);
        size_t bssSize = phdr.p_memsz - phdr.p_filesz;
        printf("[ELF] Zeroing BSS: %p, size=%zu\n", bssStart, bssSize);
        memset(bssStart, 0, bssSize);
      }

      printf("[ELF] Segment %u loaded successfully\n", i);
      fflush(stdout);
      break;
    }
    case PT_DYNAMIC: {
      printf("[ELF] Found PT_DYNAMIC at vaddr=%#" B_PRIx64 "\n",
             (uint64)phdr.p_vaddr);
      fDynamic = (typename Class::Dyn *)FromVirt(phdr.p_vaddr);
      break;
    }
    case PT_INTERP: {
      printf("[ELF] Interpreter segment found\n");
      break;
    }
    default:
      printf("[ELF] Ignoring segment type %u\n", phdr.p_type);
      break;
    }
  }
}

template <typename Class> void ElfImageImpl<Class>::Relocate() {
  typename Class::Rel *relocAdr = NULL;
  Address relocSize = 0;
  typename Class::Rela *relocAAdr = NULL;
  Address relocASize = 0;
  void *pltRelocAdr = NULL;
  Address pltRelocSize = 0;
  Address pltRelocType;
  if (fDynamic == NULL)
    return;
  for (typename Class::Dyn *dyn = fDynamic; dyn->d_tag != DT_NULL; dyn++) {
    switch (dyn->d_tag) {
    case DT_REL:
      relocAdr = (typename Class::Rel *)FromVirt(dyn->d_un.d_ptr);
      break;
    case DT_RELSZ:
      relocSize = dyn->d_un.d_ptr;
      break;
    case DT_RELA:
      relocAAdr = (typename Class::Rela *)FromVirt(dyn->d_un.d_ptr);
      break;
    case DT_RELASZ:
      relocASize = dyn->d_un.d_ptr;
      break;
    case DT_PLTREL:
      pltRelocType = dyn->d_un.d_ptr;
      break;
    case DT_JMPREL:
      pltRelocAdr = FromVirt(dyn->d_un.d_ptr);
      break;
    case DT_PLTRELSZ:
      pltRelocSize = dyn->d_un.d_ptr;
      break;
    case DT_SYMTAB:
      fSymbols = (typename Class::Sym *)FromVirt(dyn->d_un.d_ptr);
      break;
    case DT_STRTAB:
      fStrings = (const char *)FromVirt(dyn->d_un.d_ptr);
      break;
    case DT_HASH:
      fHash = (uint32 *)FromVirt(dyn->d_un.d_ptr);
      break;
    }
  }
  if (relocAdr != NULL)
    DoRelocate<typename Class::Rel>(relocAdr, relocSize);
  if (relocAAdr != NULL)
    DoRelocate<typename Class::Rela>(relocAAdr, relocASize);
  if (pltRelocAdr != NULL) {
    switch (pltRelocType) {
    case DT_REL:
      DoRelocate<typename Class::Rel>((typename Class::Rel *)pltRelocAdr,
                                      pltRelocSize);
      break;
    case DT_RELA:
      DoRelocate<typename Class::Rela>((typename Class::Rela *)pltRelocAdr,
                                       pltRelocSize);
      break;
    default:
      abort();
    }
  }
}

template <typename Class>
template <typename Reloc>
void ElfImageImpl<Class>::DoRelocate(Reloc *reloc, Address relocSize) {
  Address count = relocSize / sizeof(Reloc);
  for (; count > 0; reloc++, count--) {
    Address *dst = (Address *)FromVirt(reloc->r_offset);
    Address old = 0, sym = 0;
    if constexpr (std::is_same<Reloc, typename Class::Rel>()) {
      old = *dst;
    } else if constexpr (std::is_same<Reloc, typename Class::Rela>()) {
      old = reloc->r_addend;
    } else {
      abort();
    }
    if (reloc->SymbolIndex() != 0) {
      sym = (Address)(addr_t)FromVirt(fSymbols[reloc->SymbolIndex()].st_value);
    }

    switch (fHeader.e_machine) {
    case EM_386:
    case EM_486:
      switch (reloc->Type()) {
      case R_386_NONE:
        break;
      case R_386_32:
        *dst = old + sym;
        break;
      case R_386_GLOB_DAT:
      case R_386_JMP_SLOT:
        *dst = sym;
        break;
      case R_386_RELATIVE:
        *dst = (Address)(addr_t)FromVirt(old + sym);
        break;
      default:
        abort();
      }
      break;
    case EM_68K:
      abort();
      break;
    case EM_PPC:
      abort();
      break;
    case EM_ARM:
      switch (reloc->Type()) {
      case R_ARM_NONE:
        continue;
      case R_ARM_RELATIVE:
        *dst = (Address)(addr_t)FromVirt(old + sym);
        break;
      case R_ARM_JMP_SLOT:
      case R_ARM_GLOB_DAT:
        *dst = sym;
        break;
      case R_ARM_ABS32:
        *dst = old + sym;
        break;
      default:
        abort();
      }
      break;
    case EM_ARM64:
      abort();
      break;
    case EM_X86_64:
      switch (reloc->Type()) {
      case R_X86_64_NONE:
        break;
      case R_X86_64_64:
        *dst = old + sym;
        break;
      case R_X86_64_JUMP_SLOT:
      case R_X86_64_GLOB_DAT:
        *dst = sym;
        break;
      case R_X86_64_RELATIVE:
        *dst = (Address)(addr_t)FromVirt(old + sym);
        break;
      default:
        abort();
      }
      break;
    case EM_RISCV:
      switch (reloc->Type()) {
      case R_RISCV_NONE:
        break;
      case R_RISCV_64:
        *dst = old + sym;
        break;
      case R_RISCV_JUMP_SLOT:
        *dst = sym;
        break;
      case R_RISCV_RELATIVE:
        *dst = (Address)(addr_t)FromVirt(old + sym);
        break;
      default:
        abort();
      }
      break;
    default:
      abort();
    }
  }
}

template <typename Class> void ElfImageImpl<Class>::Register() {
  extended_image_info info{.basic_info =
                               {
                                   .type = B_LIBRARY_IMAGE,
                                   .text = fBase,
                                   .text_size = (int32)fSize,
                               },
                           .text_delta = fDelta,
                           .symbol_table = fSymbols};

  struct stat stat;
  if (_kern_read_stat(fileno(fFile.Get()), NULL, false, &stat,
                      sizeof(struct stat)) == B_OK) {
    info.basic_info.device = stat.st_dev;
    info.basic_info.node = stat.st_ino;
  } else {
    info.basic_info.device = -1;
    info.basic_info.node = -1;
  }

  strcpy(info.basic_info.name, fPath.Get());

  _kern_register_image(&info, sizeof(info));
}

template <typename Class> void ElfImageImpl<Class>::LoadDynamic() {
  if (!fDynamic) {
    printf("[ELF] No PT_DYNAMIC section found\n");
    return;
  }

  printf("[ELF] Processing PT_DYNAMIC section at %p\n", fDynamic);

  // Find string table and symbol table pointers
  int dynCnt = 0;
  for (typename Class::Dyn *dyn = fDynamic; dyn->d_tag != DT_NULL; dyn++) {
    dynCnt++;
    if (dynCnt > 1000) {
      printf("[ELF] ERROR: Too many DT entries, likely corrupted\n");
      break;
    }

    switch (dyn->d_tag) {
    case DT_STRTAB:
      fStrings = (const char *)FromVirt(dyn->d_un.d_ptr);
      printf("[ELF] DT_STRTAB at %p\n", fStrings);
      break;
    case DT_SYMTAB:
      fSymbols = (typename Class::Sym *)FromVirt(dyn->d_un.d_ptr);
      printf("[ELF] DT_SYMTAB at %p\n", fSymbols);
      break;
    case DT_HASH:
      fHash = (uint32 *)FromVirt(dyn->d_un.d_ptr);
      if (fHash) {
        printf("[ELF] DT_HASH at %p (nbucket=%u, nchain=%u)\n", fHash, fHash[0],
               fHash[1]);
      }
      break;
    case DT_NEEDED: {
      const char *depName = fStrings ? fStrings + dyn->d_un.d_val : "???";
      printf("[ELF] Dependency: %s\n", depName);
    } break;
    case DT_REL:
    case DT_RELA:
    case DT_RELENT:
    case DT_RELAENT:
    case DT_PLTREL:
      printf("[ELF] Relocation tag: %#lx\n", (unsigned long)dyn->d_tag);
      break;
    }
  }

  printf("[ELF] Processed %d dynamic entries\n", dynCnt);
  fIsDynamic = true;
}

template <typename Class> void ElfImageImpl<Class>::DoLoad() {
  LoadHeaders();
  LoadSegments();
  LoadDynamic();
  Relocate();
  // Register();
}

template <typename Class> const char *ElfImageImpl<Class>::GetArchString() {
  switch (fHeader.e_machine) {
  case EM_386:
  case EM_486:
    return "x86";
  case EM_68K:
    return "m68k"; // big endian
  case EM_PPC:
    return "ppc"; // big endian
  case EM_ARM:
    return "arm";
  case EM_ARM64:
    return "arm64";
  case EM_X86_64:
    return "x86_64";
  case EM_SPARCV9:
    return "sparc"; // big endian, 64 bit
  case EM_RISCV:
    if constexpr (std::is_same<Class, Elf32Class>())
      return "riscv32";
    else if constexpr (std::is_same<Class, Elf64Class>())
      return "riscv64";
    else
      abort();
    break;
  }
  return NULL;
}

template <typename Class> void *ElfImageImpl<Class>::GetImageBase() {
  return fBase;
}

template <typename Class>
bool ElfImageImpl<Class>::FindSymbol(const char *name, void **adr,
                                     size_t *size) {
  if (fSymbols == NULL || fHash == NULL || fStrings == NULL) {
    printf("[ELF] Symbol lookup failed: symbols=%p hash=%p strings=%p\n",
           (void *)fSymbols, (void *)fHash, (void *)fStrings);
    return false;
  }

  uint32 symCnt = fHash[1];
  printf("[ELF] Searching for symbol '%s' in %u symbols\n", name, symCnt);

  for (uint32 i = 0; i < symCnt; i++) {
    typename Class::Sym &sym = fSymbols[i];
    if (sym.st_shndx == SHN_UNDEF)
      continue;

    const char *symName = fStrings + sym.st_name;
    if (strcmp(name, symName) != 0)
      continue;

    if (adr != NULL)
      *adr = FromVirt(sym.st_value);
    if (size != NULL)
      *size = sym.st_size;

    printf("[ELF] Found symbol '%s' at %p (size=%zu)\n", name, *adr, *size);
    return true;
  }

  printf("[ELF] Symbol '%s' not found\n", name);
  return false;
}

template class ElfImageImpl<Elf32Class>;
template class ElfImageImpl<Elf64Class>;

ElfImage *ElfImage::Load(const char *path) {
  printf("[ELF] Loading file: %s\n", path);
  fflush(stdout);

  ObjectDeleter<ElfImage> image;
  FileCloser file(fopen(path, "rb"));
  if (!file.IsSet()) {
    printf("[ELF] ERROR: Failed to open file: %s\n", path);
    return NULL;
  }

  printf("[ELF] File opened, reading ELF header\n");
  fflush(stdout);

  uint8 ident[EI_NIDENT];
  if (fread(ident, sizeof(ident), 1, file.Get()) != 1) {
    printf("[ELF] ERROR: Failed to read ELF identification\n");
    return NULL;
  }

  printf("[ELF] ELF header read: magic=%02x%02x%02x%02x\n", ident[EI_MAG0],
         ident[EI_MAG1], ident[EI_MAG2], ident[EI_MAG3]);
  fflush(stdout);

  if (!((ident[EI_MAG0] == ELFMAG0 && ident[EI_MAG1] == ELFMAG1 &&
         ident[EI_MAG2] == ELFMAG2 && ident[EI_MAG3] == ELFMAG3))) {
    printf("[ELF] ERROR: Invalid ELF magic number\n");
    return NULL;
  }

  printf("[ELF] ELF magic valid, class=%u\n", ident[EI_CLASS]);
  fflush(stdout);

  switch (ident[EI_CLASS]) {
  case ELFCLASS32:
    printf("[ELF] 32-bit ELF detected\n");
    image.SetTo(new ElfImageImpl<Elf32Class>());
    break;
  case ELFCLASS64:
    printf("[ELF] 64-bit ELF detected\n");
    image.SetTo(new ElfImageImpl<Elf64Class>());
    break;
  default:
    printf("[ELF] ERROR: Unknown ELF class: %u\n", ident[EI_CLASS]);
    return NULL;
  }

  image->fPath.SetTo(new char[strlen(path) + 1]);
  strcpy(image->fPath.Get(), path);
  image->fFile.SetTo(file.Detach());

  printf("[ELF] Starting ELF image load\n");
  fflush(stdout);
  image->DoLoad();
  printf("[ELF] ELF image load complete\n");
  fflush(stdout);

  return image.Detach();
}
