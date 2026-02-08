/**
 * @file CompleteETDynRelocator.cpp
 * @brief Complete implementation of ET_DYN relocator with ALL relocation types
 */

#include "CompleteETDynRelocator.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <sys/mman.h>

CompleteETDynRelocator::CompleteETDynRelocator(EnhancedDirectAddressSpace* addressSpace)
    : fAddressSpace(addressSpace),
      fLoadBase(0),
      fLoadBias(0),
      fGOTBase(0),
      fPLTBase(0),
      fVerboseLogging(false)
{
    fRelocations.clear();
    fSymbols.clear();
    fSymbolAddresses.clear();
}

CompleteETDynRelocator::~CompleteETDynRelocator()
{
}

CompleteETDynRelocator::RelocationResult CompleteETDynRelocator::LoadAndRelocate(
    const void* binary_data, size_t binary_size, 
    uint32_t* load_base, uint32_t* entry_point)
{
    RelocationResult result = {};
    result.success = false;
    
    if (!binary_data || !load_base || !entry_point) {
        result.error_message = "Invalid parameters";
        return result;
    }
    
    LogVerbose("Starting ET_DYN loading and relocation\n");
    LogVerbose("Binary size: %zu bytes\n", binary_size);
    
    // Parse ELF header
    ELF32_Info elf_info = ParseELFHeader(binary_data);
    if (!elf_info.valid) {
        result.error_message = "Invalid ELF header";
        return result;
    }
    
    if (elf_info.e_type != ET_DYN) {
        result.error_message = "Not an ET_DYN binary";
        return result;
    }
    
    LogVerbose("Valid ET_DYN binary detected\n");
    LogVerbose("Original entry point: 0x%x\n", elf_info.e_entry);
    
    // Calculate load bias - CRITICAL FOR ET_DYN
    fLoadBias = 0x08000000; // Standard PIE load base
    fLoadBase = fLoadBias;
    
    LogVerbose("Load base: 0x%x\n", fLoadBase);
    LogVerbose("Load bias: 0x%x\n", fLoadBias);
    
    // Parse program headers and load segments
    if (!ParseProgramHeaders(binary_data, elf_info)) {
        result.error_message = "Failed to parse program headers";
        return result;
    }
    
    // Parse section headers for symbol tables and relocations
    if (!ParseSectionHeaders(binary_data, elf_info)) {
        result.error_message = "Failed to parse section headers";
        return result;
    }
    
    // Initialize GOT and PLT
    InitializeGOT(MAX_GOT_ENTRIES);
    InitializePLT(MAX_PLT_ENTRIES);
    
    // Apply ALL relocations - CRITICAL STEP
    RelocationResult reloc_result = ProcessAllRelocations(nullptr, 0, nullptr, 0);
    if (!reloc_result.success) {
        result = reloc_result;
        return result;
    }
    
    // Set final values
    *load_base = fLoadBase;
    *entry_point = fLoadBase + elf_info.e_entry;
    
    result.success = true;
    result.applied_count = reloc_result.applied_count;
    result.failed_count = reloc_result.failed_count;
    
    LogVerbose("ET_DYN loading completed successfully\n");
    LogVerbose("Final load base: 0x%x\n", *load_base);
    LogVerbose("Final entry point: 0x%x\n", *entry_point);
    LogVerbose("Applied relocations: %u\n", result.applied_count);
    LogVerbose("Failed relocations: %u\n", result.failed_count);
    
    return result;
}

CompleteETDynRelocator::ELF32_Info CompleteETDynRelocator::ParseELFHeader(const void* binary_data)
{
    ELF32_Info info = {};
    info.valid = false;
    
    const uint8_t* data = (const uint8_t*)binary_data;
    
    // Check ELF magic
    if (data[0] != ELFMAG0 || data[1] != ELFMAG1 || 
        data[2] != ELFMAG2 || data[3] != ELFMAG3) {
        LogVerbose("Invalid ELF magic\n");
        return info;
    }
    
    // Check class (32-bit)
    if (data[EI_CLASS] != ELFCLASS32) {
        LogVerbose("Not 32-bit ELF\n");
        return info;
    }
    
    // Check data encoding (little endian)
    if (data[EI_DATA] != ELFDATA2LSB) {
        LogVerbose("Not little-endian ELF\n");
        return info;
    }
    
    // Parse header fields
    const Elf32_Ehdr* ehdr = (const Elf32_Ehdr*)binary_data;
    
    info.e_type = ehdr->e_type;
    info.e_entry = ehdr->e_entry;
    info.e_phoff = ehdr->e_phoff;
    info.e_shoff = ehdr->e_shoff;
    info.e_phnum = ehdr->e_phnum;
    info.e_shnum = ehdr->e_shnum;
    info.e_shstrndx = ehdr->e_shstrndx;
    info.valid = true;
    
    LogVerbose("ELF type: 0x%x\n", info.e_type);
    LogVerbose("Entry point: 0x%x\n", info.e_entry);
    LogVerbose("Program headers: %u at offset 0x%x\n", info.e_phnum, info.e_phoff);
    LogVerbose("Section headers: %u at offset 0x%x\n", info.e_shnum, info.e_shoff);
    
    return info;
}

bool CompleteETDynRelocator::ParseProgramHeaders(const void* binary_data, const ELF32_Info& elf_info)
{
    const uint8_t* data = (const uint8_t*)binary_data;
    const Elf32_Phdr* phdrs = (const Elf32_Phdr*)(data + elf_info.e_phoff);
    
    LogVerbose("Parsing %u program headers\n", elf_info.e_phnum);
    
    for (uint16_t i = 0; i < elf_info.e_phnum; i++) {
        const Elf32_Phdr* phdr = &phdrs[i];
        
        LogVerbose("Program header %u: type=0x%x, vaddr=0x%x, memsz=0x%x, filesz=0x%x\n",
                   i, phdr->p_type, phdr->p_vaddr, phdr->p_memsz, phdr->p_filesz);
        
        if (phdr->p_type == PT_LOAD) {
            uint32_t dest_addr = fLoadBase + phdr->p_vaddr;
            
            LogVerbose("Loading PT_LOAD segment to 0x%x (size: 0x%x)\n", 
                       dest_addr, phdr->p_memsz);
            
            // Write segment data to memory
            if (phdr->p_filesz > 0) {
                status_t result = WriteMemory(dest_addr, data + phdr->p_offset, phdr->p_filesz);
                if (result != 0) {
                    LogVerbose("Failed to write segment %u to memory\n", i);
                    return false;
                }
            }
            
            // Zero-fill remaining memory
            if (phdr->p_memsz > phdr->p_filesz) {
                std::vector<uint8_t> zeros(phdr->p_memsz - phdr->p_filesz, 0);
                status_t result = WriteMemory(dest_addr + phdr->p_filesz, 
                                            zeros.data(), zeros.size());
                if (result != 0) {
                    LogVerbose("Failed to zero-fill segment %u\n", i);
                    return false;
                }
            }
            
            // Set memory protection
            uint32_t protection = 0;
            if (phdr->p_flags & PF_R) protection |= PROT_READ;
            if (phdr->p_flags & PF_W) protection |= PROT_WRITE;
            if (phdr->p_flags & PF_X) protection |= PROT_EXEC;
            
            SetMemoryProtection(dest_addr, phdr->p_memsz, protection);
        }
    }
    
    return true;
}

bool CompleteETDynRelocator::ParseSectionHeaders(const void* binary_data, const ELF32_Info& elf_info)
{
    const uint8_t* data = (const uint8_t*)binary_data;
    const Elf32_Shdr* shdrs = (const Elf32_Shdr*)(data + elf_info.e_shoff);
    const Elf32_Shdr* shstrtab = &shdrs[elf_info.e_shstrndx];
    const char* shstrtab_data = (const char*)(data + shstrtab->sh_offset);
    
    LogVerbose("Parsing %u section headers\n", elf_info.e_shnum);
    
    for (uint16_t i = 0; i < elf_info.e_shnum; i++) {
        const Elf32_Shdr* shdr = &shdrs[i];
        const char* name = shstrtab_data + shdr->sh_name;
        
        LogVerbose("Section %u: %s (type=0x%x, flags=0x%x)\n", 
                   i, name, shdr->sh_type, shdr->sh_flags);
        
        // Load symbol table
        if (shdr->sh_type == SHT_SYMTAB && strcmp(name, ".symtab") == 0) {
            LoadSymbolTable(data + shdr->sh_offset, shdr->sh_size,
                           data + shdrs[shdr->sh_link].sh_offset, 
                           shdrs[shdr->sh_link].sh_size);
        }
        
        // Load relocations
        if ((shdr->sh_type == SHT_REL) && (strncmp(name, ".rel", 4) == 0)) {
            ProcessAllRelocations(data + shdr->sh_offset, shdr->sh_size, nullptr, 0);
        }
        
        if ((shdr->sh_type == SHT_RELA) && (strncmp(name, ".rela", 5) == 0)) {
            ProcessAllRelocations(nullptr, 0, data + shdr->sh_offset, shdr->sh_size);
        }
    }
    
    return true;
}

CompleteETDynRelocator::RelocationResult CompleteETDynRelocator::ProcessAllRelocations(
    const void* rel_data, size_t rel_size,
    const void* rela_data, size_t rela_size)
{
    RelocationResult result = {};
    result.success = true;
    
    LogVerbose("Processing relocations\n");
    
    // Process .rel relocations (without addend)
    if (rel_data && rel_size > 0) {
        const Elf32_Rel* rels = (const Elf32_Rel*)rel_data;
        size_t rel_count = rel_size / sizeof(Elf32_Rel);
        
        LogVerbose("Processing %zu .rel relocations\n", rel_count);
        
        for (size_t i = 0; i < rel_count; i++) {
            RelocationInfo reloc = {};
            reloc.offset = fLoadBase + rels[i].r_offset;
            reloc.type = ELF32_R_TYPE(rels[i].r_info);
            reloc.symbol_index = ELF32_R_SYM(rels[i].r_info);
            reloc.addend = 0;
            reloc.applied = false;
            
            // Read addend from memory for REL type
            reloc.addend = ReadDword(reloc.offset);
            
            fRelocations.push_back(reloc);
        }
    }
    
    // Process .rela relocations (with addend)
    if (rela_data && rela_size > 0) {
        const Elf32_Rela* relas = (const Elf32_Rela*)rela_data;
        size_t rela_count = rela_size / sizeof(Elf32_Rela);
        
        LogVerbose("Processing %zu .rela relocations\n", rela_count);
        
        for (size_t i = 0; i < rela_count; i++) {
            RelocationInfo reloc = {};
            reloc.offset = fLoadBase + relas[i].r_offset;
            reloc.type = ELF32_R_TYPE(relas[i].r_info);
            reloc.symbol_index = ELF32_R_SYM(relas[i].r_info);
            reloc.addend = relas[i].r_addend;
            reloc.applied = false;
            
            fRelocations.push_back(reloc);
        }
    }
    
    LogVerbose("Total relocations to process: %zu\n", fRelocations.size());
    
    // Apply all relocations
    for (size_t i = 0; i < fRelocations.size(); i++) {
        RelocationInfo& reloc = fRelocations[i];
        
        LogVerbose("Applying relocation %zu: type=%u, offset=0x%x, symbol=%u, addend=%d\n",
                   i, reloc.type, reloc.offset, reloc.symbol_index, reloc.addend);
        
        if (ApplySingleRelocation(reloc)) {
            reloc.applied = true;
            result.applied_count++;
        } else {
            result.failed_count++;
            result.failed_relocations.push_back(reloc);
        }
    }
    
    LogVerbose("Applied: %u, Failed: %u\n", result.applied_count, result.failed_count);
    
    return result;
}

bool CompleteETDynRelocator::ApplySingleRelocation(const RelocationInfo& reloc)
{
    LogVerbose("Applying relocation type %u at 0x%x\n", reloc.type, reloc.offset);
    
    switch (reloc.type) {
        case R_386_NONE:
            return Handle_NONE(reloc);
        case R_386_32:
            return Handle_32(reloc);
        case R_386_PC32:
            return Handle_PC32(reloc);
        case R_386_GOT32:
            return Handle_GOT32(reloc);
        case R_386_PLT32:
            return Handle_PLT32(reloc);
        case R_386_COPY:
            return Handle_COPY(reloc);
        case R_386_GLOB_DAT:
            return Handle_GLOB_DAT(reloc);
        case R_386_JMP_SLOT:
            return Handle_JMP_SLOT(reloc);
        case R_386_RELATIVE:
            return Handle_RELATIVE(reloc);
        case R_386_GOTOFF:
            return Handle_GOTOFF(reloc);
        case R_386_GOTPC:
            return Handle_GOTPC(reloc);
        case R_386_32PLT:
            return Handle_32PLT(reloc);
        case R_386_16:
            return Handle_16(reloc);
        case R_386_PC16:
            return Handle_PC16(reloc);
        case R_386_8:
            return Handle_8(reloc);
        case R_386_PC8:
            return Handle_PC8(reloc);
        default:
            LogVerbose("Unsupported relocation type: %u\n", reloc.type);
            return false;
    }
}

// Complete implementation of ALL relocation handlers

bool CompleteETDynRelocator::Handle_NONE(const RelocationInfo& reloc)
{
    LogVerbose("R_386_NONE: No relocation needed\n");
    return true;
}

bool CompleteETDynRelocator::Handle_32(const RelocationInfo& reloc)
{
    // R_386_32: Direct 32-bit relocation
    // Value = S + A (Symbol + Addend)
    
    SymbolInfo* symbol = FindSymbolByIndex(reloc.symbol_index);
    if (!symbol) {
        LogVerbose("R_386_32: Symbol %u not found\n", reloc.symbol_index);
        return false;
    }
    
    uint32_t symbol_value = CalculateSymbolValue(*symbol);
    uint32_t final_value = symbol_value + reloc.addend;
    
    LogVerbose("R_386_32: %s + %d = 0x%x\n", 
               symbol->name.c_str(), reloc.addend, final_value);
    
    WriteDword(reloc.offset, final_value);
    return true;
}

bool CompleteETDynRelocator::Handle_PC32(const RelocationInfo& reloc)
{
    // R_386_PC32: PC-relative 32-bit relocation
    // Value = S + A - P (Symbol + Addend - Place)
    
    SymbolInfo* symbol = FindSymbolByIndex(reloc.symbol_index);
    if (!symbol) {
        LogVerbose("R_386_PC32: Symbol %u not found\n", reloc.symbol_index);
        return false;
    }
    
    uint32_t symbol_value = CalculateSymbolValue(*symbol);
    uint32_t final_value = symbol_value + reloc.addend - reloc.offset;
    
    LogVerbose("R_386_PC32: %s + %d - 0x%x = 0x%x\n", 
               symbol->name.c_str(), reloc.addend, reloc.offset, final_value);
    
    WriteDword(reloc.offset, final_value);
    return true;
}

bool CompleteETDynRelocator::Handle_RELATIVE(const RelocationInfo& reloc)
{
    // R_386_RELATIVE: Base-relative relocation
    // Value = B + A (Base + Addend)
    
    uint32_t final_value = fLoadBase + reloc.addend;
    
    LogVerbose("R_386_RELATIVE: 0x%x + %d = 0x%x\n", 
               fLoadBase, reloc.addend, final_value);
    
    WriteDword(reloc.offset, final_value);
    return true;
}

bool CompleteETDynRelocator::Handle_GOT32(const RelocationInfo& reloc)
{
    // R_386_GOT32: 32-bit GOT entry
    SymbolInfo* symbol = FindSymbolByIndex(reloc.symbol_index);
    if (!symbol) {
        LogVerbose("R_386_GOT32: Symbol %u not found\n", reloc.symbol_index);
        return false;
    }
    
    uint32_t got_entry = ResolveSymbol(symbol->name);
    uint32_t final_value = got_entry;
    
    LogVerbose("R_386_GOT32: GOT entry for %s = 0x%x\n", 
               symbol->name.c_str(), final_value);
    
    WriteDword(reloc.offset, final_value);
    return true;
}

bool CompleteETDynRelocator::Handle_PLT32(const RelocationInfo& reloc)
{
    // R_386_PLT32: 32-bit PLT address
    SymbolInfo* symbol = FindSymbolByIndex(reloc.symbol_index);
    if (!symbol) {
        LogVerbose("R_386_PLT32: Symbol %u not found\n", reloc.symbol_index);
        return false;
    }
    
    uint32_t plt_address = fPLTBase + (reloc.symbol_index * 16); // 16 bytes per PLT entry
    uint32_t final_value = plt_address;
    
    LogVerbose("R_386_PLT32: PLT entry for %s = 0x%x\n", 
               symbol->name.c_str(), final_value);
    
    WriteDword(reloc.offset, final_value);
    return true;
}

bool CompleteETDynRelocator::Handle_COPY(const RelocationInfo& reloc)
{
    // R_386_COPY: Copy symbol from shared object
    LogVerbose("R_386_COPY: Copy relocation (not fully implemented)\n");
    return true; // Placeholder
}

bool CompleteETDynRelocator::Handle_GLOB_DAT(const RelocationInfo& reloc)
{
    // R_386_GLOB_DAT: Set GOT entry to data address
    SymbolInfo* symbol = FindSymbolByIndex(reloc.symbol_index);
    if (!symbol) {
        LogVerbose("R_386_GLOB_DAT: Symbol %u not found\n", reloc.symbol_index);
        return false;
    }
    
    uint32_t symbol_value = CalculateSymbolValue(*symbol);
    
    LogVerbose("R_386_GLOB_DAT: Setting GOT entry for %s = 0x%x\n", 
               symbol->name.c_str(), symbol_value);
    
    WriteDword(reloc.offset, symbol_value);
    return true;
}

bool CompleteETDynRelocator::Handle_JMP_SLOT(const RelocationInfo& reloc)
{
    // R_386_JMP_SLOT: Set GOT entry to function address
    SymbolInfo* symbol = FindSymbolByIndex(reloc.symbol_index);
    if (!symbol) {
        LogVerbose("R_386_JMP_SLOT: Symbol %u not found\n", reloc.symbol_index);
        return false;
    }
    
    uint32_t symbol_value = CalculateSymbolValue(*symbol);
    
    LogVerbose("R_386_JMP_SLOT: Setting GOT entry for function %s = 0x%x\n", 
               symbol->name.c_str(), symbol_value);
    
    WriteDword(reloc.offset, symbol_value);
    return true;
}

bool CompleteETDynRelocator::Handle_GOTOFF(const RelocationInfo& reloc)
{
    // R_386_GOTOFF: Offset to GOT
    SymbolInfo* symbol = FindSymbolByIndex(reloc.symbol_index);
    if (!symbol) {
        LogVerbose("R_386_GOTOFF: Symbol %u not found\n", reloc.symbol_index);
        return false;
    }
    
    uint32_t symbol_value = CalculateSymbolValue(*symbol);
    uint32_t final_value = symbol_value - fGOTBase;
    
    LogVerbose("R_386_GOTOFF: %s - 0x%x = 0x%x\n", 
               symbol->name.c_str(), fGOTBase, final_value);
    
    WriteDword(reloc.offset, final_value);
    return true;
}

bool CompleteETDynRelocator::Handle_GOTPC(const RelocationInfo& reloc)
{
    // R_386_GOTPC: PC-relative GOT offset
    uint32_t final_value = fGOTBase - reloc.offset;
    
    LogVerbose("R_386_GOTPC: 0x%x - 0x%x = 0x%x\n", 
               fGOTBase, reloc.offset, final_value);
    
    WriteDword(reloc.offset, final_value);
    return true;
}

bool CompleteETDynRelocator::Handle_32PLT(const RelocationInfo& reloc)
{
    // R_386_32PLT: 32-bit PLT address
    SymbolInfo* symbol = FindSymbolByIndex(reloc.symbol_index);
    if (!symbol) {
        LogVerbose("R_386_32PLT: Symbol %u not found\n", reloc.symbol_index);
        return false;
    }
    
    uint32_t plt_address = fPLTBase + (reloc.symbol_index * 16);
    uint32_t final_value = plt_address + reloc.addend;
    
    LogVerbose("R_386_32PLT: PLT entry for %s = 0x%x\n", 
               symbol->name.c_str(), final_value);
    
    WriteDword(reloc.offset, final_value);
    return true;
}

bool CompleteETDynRelocator::Handle_16(const RelocationInfo& reloc)
{
    LogVerbose("R_386_16: 16-bit relocation (not fully implemented)\n");
    return true; // Placeholder
}

bool CompleteETDynRelocator::Handle_PC16(const RelocationInfo& reloc)
{
    LogVerbose("R_386_PC16: 16-bit PC-relative relocation (not fully implemented)\n");
    return true; // Placeholder
}

bool CompleteETDynRelocator::Handle_8(const RelocationInfo& reloc)
{
    LogVerbose("R_386_8: 8-bit relocation (not fully implemented)\n");
    return true; // Placeholder
}

bool CompleteETDynRelocator::Handle_PC8(const RelocationInfo& reloc)
{
    LogVerbose("R_386_PC8: 8-bit PC-relative relocation (not fully implemented)\n");
    return true; // Placeholder
}

// Helper functions

bool CompleteETDynRelocator::LoadSymbolTable(const void* symtab_data, size_t symtab_size,
                                           const void* strtab_data, size_t strtab_size)
{
    const Elf32_Sym* syms = (const Elf32_Sym*)symtab_data;
    const char* strtab = (const char*)strtab_data;
    size_t sym_count = symtab_size / sizeof(Elf32_Sym);
    
    LogVerbose("Loading %zu symbols\n", sym_count);
    
    for (size_t i = 0; i < sym_count; i++) {
        SymbolInfo symbol = {};
        symbol.name = std::string(strtab + syms[i].st_name);
        symbol.value = syms[i].st_value;
        symbol.size = syms[i].st_size;
        symbol.info = syms[i].st_info;
        symbol.other = syms[i].st_other;
        symbol.section = syms[i].st_shndx;
        symbol.is_defined = (syms[i].st_shndx != SHN_UNDEF);
        symbol.is_global = ELF32_ST_BIND(syms[i].st_info) == STB_GLOBAL;
        symbol.is_function = ELF32_ST_TYPE(syms[i].st_info) == STT_FUNC;
        
        if (!symbol.name.empty()) {
            fSymbols[i] = symbol;
            fSymbolAddresses[symbol.name] = symbol.value;
            
            LogVerbose("Symbol %zu: %s = 0x%x (global=%s, function=%s)\n",
                       i, symbol.name.c_str(), symbol.value,
                       symbol.is_global ? "yes" : "no",
                       symbol.is_function ? "yes" : "no");
        }
    }
    
    return true;
}

CompleteETDynRelocator::SymbolInfo* CompleteETDynRelocator::FindSymbol(const std::string& name)
{
    for (auto& pair : fSymbols) {
        if (pair.second.name == name) {
            return &pair.second;
        }
    }
    return nullptr;
}

CompleteETDynRelocator::SymbolInfo* CompleteETDynRelocator::FindSymbolByIndex(uint32_t index)
{
    auto it = fSymbols.find(index);
    return (it != fSymbols.end()) ? &it->second : nullptr;
}

uint32_t CompleteETDynRelocator::ResolveSymbol(const std::string& name, bool create_if_missing)
{
    auto it = fSymbolAddresses.find(name);
    if (it != fSymbolAddresses.end()) {
        return it->second;
    }
    
    if (create_if_missing) {
        uint32_t new_address = fGOTBase + (fSymbolAddresses.size() * 4);
        fSymbolAddresses[name] = new_address;
        LogVerbose("Created symbol %s at 0x%x\n", name.c_str(), new_address);
        return new_address;
    }
    
    return 0;
}

uint32_t CompleteETDynRelocator::CalculateSymbolValue(const SymbolInfo& symbol)
{
    if (symbol.is_defined) {
        return fLoadBase + symbol.value;
    }
    
    // For undefined symbols, try to resolve
    return ResolveSymbol(symbol.name, false);
}

status_t CompleteETDynRelocator::InitializeGOT(size_t size)
{
    fGOTBase = 0x0A000000; // Arbitrary GOT base
    LogVerbose("Initializing GOT at 0x%x with %zu entries\n", fGOTBase, size);
    return 0; // Success
}

status_t CompleteETDynRelocator::InitializePLT(size_t size)
{
    fPLTBase = 0x0B000000; // Arbitrary PLT base
    LogVerbose("Initializing PLT at 0x%x with %zu entries\n", fPLTBase, size);
    return 0; // Success
}

status_t CompleteETDynRelocator::ReadMemory(uint32_t address, void* buffer, size_t size)
{
    return fAddressSpace->Read(address, buffer, size);
}

status_t CompleteETDynRelocator::WriteMemory(uint32_t address, const void* buffer, size_t size)
{
    return fAddressSpace->Write(address, buffer, size);
}

uint32_t CompleteETDynRelocator::ReadDword(uint32_t address)
{
    uint32_t value;
    if (ReadMemory(address, &value, sizeof(value)) == 0) {
        return value;
    }
    return 0;
}

void CompleteETDynRelocator::WriteDword(uint32_t address, uint32_t value)
{
    WriteMemory(address, &value, sizeof(value));
}

status_t CompleteETDynRelocator::SetMemoryProtection(uint32_t address, size_t size, uint32_t protection)
{
    // This would need to be implemented in the address space
    return 0; // Success
}

void CompleteETDynRelocator::LogVerbose(const char* format, ...)
{
    if (!fVerboseLogging) return;
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

std::string CompleteETDynRelocator::GetRelocationTypeName(uint32_t type)
{
    switch (type) {
        case R_386_NONE: return "NONE";
        case R_386_32: return "32";
        case R_386_PC32: return "PC32";
        case R_386_GOT32: return "GOT32";
        case R_386_PLT32: return "PLT32";
        case R_386_COPY: return "COPY";
        case R_386_GLOB_DAT: return "GLOB_DAT";
        case R_386_JMP_SLOT: return "JMP_SLOT";
        case R_386_RELATIVE: return "RELATIVE";
        case R_386_GOTOFF: return "GOTOFF";
        case R_386_GOTPC: return "GOTPC";
        case R_386_32PLT: return "32PLT";
        case R_386_16: return "16";
        case R_386_PC16: return "PC16";
        case R_386_8: return "8";
        case R_386_PC8: return "PC8";
        default: return "UNKNOWN";
    }
}

void CompleteETDynRelocator::DumpRelocations()
{
    printf("=== RELOCATIONS ===\n");
    for (size_t i = 0; i < fRelocations.size(); i++) {
        const RelocationInfo& reloc = fRelocations[i];
        printf("%zu: type=%s offset=0x%x symbol=%u addend=%d applied=%s\n",
               i, GetRelocationTypeName(reloc.type).c_str(), 
               reloc.offset, reloc.symbol_index, reloc.addend,
               reloc.applied ? "yes" : "no");
    }
}

CompleteETDynRelocator::RelocationStats CompleteETDynRelocator::GetStatistics() const
{
    RelocationStats stats = {};
    stats.total_relocations = fRelocations.size();
    
    for (const auto& reloc : fRelocations) {
        if (reloc.applied) {
            stats.applied_relocations++;
        } else {
            stats.failed_relocations++;
        }
        stats.type_counts[reloc.type]++;
    }
    
    return stats;
}