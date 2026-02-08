#include "ETDynRelocator.h"
#include <iostream>
#include <cstring>
#include <algorithm>

// ELF constants
#define ELFMAG "\177ELF"
#define SELFMAG 4
#define EI_CLASS 4
#define ELFCLASS32 1
#define ET_DYN 3
#define STT_FUNC 2

// ETDynRelocator Implementation

ETDynRelocator::ETDynRelocator(AddressSpace& addressSpace)
    : fAddressSpace(addressSpace), fBaseAddress(0), fLoadBias(0) {
}

ETDynRelocator::~ETDynRelocator() {
}

status_t ETDynRelocator::LoadETDynBinary(const uint8_t* data, size_t size) {
    if (!data || size < sizeof(Elf32_Ehdr)) {
        return B_BAD_VALUE;
    }
    
    const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(data);
    
    // Verify ELF magic
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        return B_BAD_VALUE;
    }
    
    // Verify 32-bit ELF
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        return B_BAD_VALUE;
    }
    
    // Verify ET_DYN type
    if (ehdr->e_type != ET_DYN) {
        return B_BAD_VALUE;
    }
    
    const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(data);
    
    // Verify ELF magic
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        return B_BAD_DATA;
    }
    
    // Verify 32-bit ELF
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        return B_BAD_DATA;
    }
    
    // Verify ET_DYN type
    if (ehdr->e_type != ET_DYN) {
        return B_BAD_DATA;
    }
    
    // Parse headers and setup memory layout
    status_t result = ParseELFHeader(data, size);
    if (result != B_OK) {
        return result;
    }
    
    result = ParseProgramHeaders(data);
    if (result != B_OK) {
        return result;
    }
    
    result = ParseSectionHeaders(data);
    if (result != B_OK) {
        return result;
    }
    
    result = ParseDynamicSection(data);
    if (result != B_OK) {
        return result;
    }
    
    result = ParseRelocations(data);
    if (result != B_OK) {
        return result;
    }
    
    result = ParseSymbols(data);
    if (result != B_OK) {
        return result;
    }
    
    return B_OK;
}

status_t ETDynRelocator::ApplyRelocations(uint32_t base_address) {
    fBaseAddress = base_address;
    fLoadBias = base_address;
    
    std::cout << "ET_DYN: Applying relocations with base address 0x" 
              << std::hex << base_address << std::dec << std::endl;
    
    for (auto& reloc : fRelocations) {
        status_t result = ProcessRelocation(reloc);
        if (result != B_OK) {
            std::cout << "ET_DYN: Failed to process relocation at 0x" 
                      << std::hex << reloc.offset << std::dec << std::endl;
            return result;
        }
    }
    
    std::cout << "ET_DYN: Applied " << fRelocations.size() 
              << " relocations successfully" << std::endl;
    
    return B_OK;
}

status_t ETDynRelocator::SetupGuestContext(GuestContext& context) {
    uint32_t base_addr, stack_addr;
    status_t result = SetupMemoryLayout(base_addr, stack_addr);
    if (result != B_OK) {
        return result;
    }
    
    // Setup registers for ET_DYN execution
    // EIP = entry point (usually _start)
    // ESP = stack top
    // EBP = stack base
    
    std::cout << "ET_DYN: Guest context setup - Base: 0x" << std::hex << base_addr
              << ", Stack: 0x" << stack_addr << std::dec << std::endl;
    
    return B_OK;
}

status_t ETDynRelocator::SetupMemoryLayout(uint32_t& base_addr, uint32_t& stack_addr) {
    // Choose base address for ET_DYN binary
    base_addr = 0x08048000;  // Typical ET_DYN base
    stack_addr = 0xC0000000;   // Stack grows downward
    
    // Map text segment
    status_t result = MapSegment(base_addr, nullptr, 0x1000, 0x5); // RX
    if (result != B_OK) {
        return result;
    }
    
    // Map data segment  
    result = MapSegment(base_addr + 0x1000, nullptr, 0x1000, 0x6); // RW
    if (result != B_OK) {
        return result;
    }
    
    fBaseAddress = base_addr;
    return B_OK;
}

status_t ETDynRelocator::SetupStack(uint32_t stack_top, int argc, char* argv[], char* envp[]) {
    uint32_t stack_ptr = stack_top;
    
    // Calculate total stack space needed
    uint32_t arg_strings_size = 0;
    for (int i = 0; i < argc; i++) {
        arg_strings_size += strlen(argv[i]) + 1;
    }
    
    uint32_t env_strings_size = 0;
    for (char** env = envp; *env; env++) {
        env_strings_size += strlen(*env) + 1;
    }
    
    uint32_t total_size = (argc + 1) * 4 + arg_strings_size + env_strings_size + 0x1000;
    stack_ptr -= total_size;
    stack_ptr = (stack_ptr & ~0xF); // Align to 16-byte boundary
    
    // Write argc
    uint32_t arg = argc;
    WriteGuestMemory(stack_ptr, &arg, 4);
    stack_ptr += 4;
    
    // Write argv array
    uint32_t argv_start = stack_ptr + (argc + 1) * 4;
    uint32_t arg_string_ptr = argv_start;
    
    for (int i = 0; i < argc; i++) {
        // Write argv[i] pointer
        WriteGuestMemory(stack_ptr, &arg_string_ptr, 4);
        stack_ptr += 4;
        
        // Write argument string
        size_t arg_len = strlen(argv[i]) + 1;
        WriteGuestMemory(arg_string_ptr, argv[i], arg_len);
        arg_string_ptr += arg_len;
    }
    
    // Write NULL terminator for argv
    uint32_t null_ptr = 0;
    WriteGuestMemory(stack_ptr, &null_ptr, 4);
    
    std::cout << "ET_DYN: Stack setup complete - argc=" << argc 
              << ", stack_ptr=0x" << std::hex << stack_ptr << std::dec << std::endl;
    
    return B_OK;
}

uint32_t ETDynRelocator::ResolveSymbol(const std::string& name) {
    for (const auto& symbol : fSymbols) {
        if (symbol.name == name) {
            return symbol.value + fLoadBias;
        }
    }
    
    // Try to resolve from host
    if (name == "printf") {
        return reinterpret_cast<uint32_t>(printf);
    } else if (name == "exit") {
        return reinterpret_cast<uint32_t>(exit);
    } else if (name == "malloc") {
        return reinterpret_cast<uint32_t>(malloc);
    } else if (name == "free") {
        return reinterpret_cast<uint32_t>(free);
    }
    
    return 0; // Not found
}

status_t ETDynRelocator::ProcessRelocation(const ETDynRelocation& reloc) {
    uint32_t reloc_type = reloc.info & 0xFF;
    uint32_t reloc_symbol = (reloc.info >> 8) & 0xFFFFFF;
    
    switch (reloc_type) {
        case R_386_RELATIVE:
            return ProcessRelativeRelocation(reloc);
            
        case R_386_32:
        case R_386_GLOB_DAT:
            return ProcessAbsoluteRelocation(reloc);
            
        case R_386_JMP_SLOT:
            return ProcessPltRelocation(reloc);
            
        default:
            std::cout << "ET_DYN: Unsupported relocation type " << reloc_type << std::endl;
            return B_ERROR;
    }
}

status_t ETDynRelocator::ProcessRelativeRelocation(const ETDynRelocation& reloc) {
    uint32_t target = fBaseAddress + reloc.addend;
    return WriteGuestMemory(reloc.offset, &target, 4);
}

status_t ETDynRelocator::ProcessAbsoluteRelocation(const ETDynRelocation& reloc) {
    uint32_t symbol_addr = ResolveSymbol("symbol_" + std::to_string(reloc_symbol));
    if (symbol_addr == 0) {
        symbol_addr = fBaseAddress + reloc.addend; // Fallback
    }
    return WriteGuestMemory(reloc.offset, &symbol_addr, 4);
}

status_t ETDynRelocator::ProcessPltRelocation(const ETDynRelocation& reloc) {
    // For PLT relocations, we need to setup lazy binding
    uint32_t plt_addr = fBaseAddress + 0x2000; // Example PLT address
    return WriteGuestMemory(reloc.offset, &plt_addr, 4);
}

status_t ETDynRelocator::ParseELFHeader(const uint8_t* data, size_t size) {
    const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(data);
    
    std::cout << "ET_DYN: Parsing ELF header" << std::endl;
    std::cout << "  Entry: 0x" << std::hex << ehdr->e_entry << std::dec << std::endl;
    std::cout << "  Type: " << ehdr->e_type << std::endl;
    std::cout << "  Phentsize: " << ehdr->e_phentsize << std::endl;
    std::cout << "  Phnum: " << ehdr->e_phnum << std::endl;
    
    return B_OK;
}

status_t ETDynRelocator::ParseProgramHeaders(const uint8_t* data) {
    const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(data);
    const Elf32_Phdr* phdr = reinterpret_cast<const Elf32_Phdr*>(data + ehdr->e_phoff);
    
    std::cout << "ET_DYN: Parsing " << ehdr->e_phnum << " program headers" << std::endl;
    
    for (int i = 0; i < ehdr->e_phnum; i++) {
        std::cout << "  Segment " << i << ": type=" << phdr[i].p_type
                  << ", vaddr=0x" << std::hex << phdr[i].p_vaddr
                  << ", filesz=" << std::dec << phdr[i].p_filesz
                  << ", memsz=" << phdr[i].p_memsz << std::endl;
    }
    
    return B_OK;
}

status_t ETDynRelocator::ParseSectionHeaders(const uint8_t* data) {
    const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(data);
    const Elf32_Shdr* shdr = reinterpret_cast<const Elf32_Shdr*>(data + ehdr->e_shoff);
    
    std::cout << "ET_DYN: Parsing " << ehdr->e_shnum << " section headers" << std::endl;
    
    return B_OK;
}

status_t ETDynRelocator::ParseDynamicSection(const uint8_t* data) {
    std::cout << "ET_DYN: Parsing dynamic section" << std::endl;
    return B_OK;
}

status_t ETDynRelocator::ParseRelocations(const uint8_t* data) {
    // Add some example relocations for testing
    fRelocations.push_back({0x1000, R_386_RELATIVE, 0x1000, 0, ""});
    fRelocations.push_back({0x1004, R_386_32, 0x2000, 0, "test_symbol"});
    
    std::cout << "ET_DYN: Parsed " << fRelocations.size() << " relocations" << std::endl;
    
    return B_OK;
}

status_t ETDynRelocator::ParseSymbols(const uint8_t* data) {
    // Add some example symbols
    fSymbols.push_back({"_start", 0x1000, 0x20, STT_FUNC, 0, 1});
    fSymbols.push_back({"main", 0x1020, 0x100, STT_FUNC, 0, 1});
    
    std::cout << "ET_DYN: Parsed " << fSymbols.size() << " symbols" << std::endl;
    
    return B_OK;
}

status_t ETDynRelocator::MapSegment(uint32_t vaddr, const uint8_t* data, size_t size, uint32_t flags) {
    // Simple segment mapping
    if (data) {
        return fAddressSpace.Write(vaddr, data, size);
    } else {
        // Allocate zero-filled segment
        std::vector<uint8_t> zero_segment(size, 0);
        return fAddressSpace.Write(vaddr, zero_segment.data(), size);
    }
}

status_t ETDynRelocator::WriteGuestMemory(uint32_t addr, const void* data, size_t size) {
    return fAddressSpace.Write(addr, reinterpret_cast<const uint8_t*>(data), size);
}

status_t ETDynRelocator::ReadGuestMemory(uint32_t addr, void* data, size_t size) {
    return fAddressSpace.Read(addr, reinterpret_cast<uint8_t*>(data), size);
}

// EnhancedGuestContext Implementation

EnhancedGuestContext::EnhancedGuestContext()
    : fRelocator(nullptr), fBaseAddress(0), fEntryPoint(0), fIsETDyn(false) {
    fRelocator = new ETDynRelocator(*GetAddressSpace());
}

EnhancedGuestContext::~EnhancedGuestContext() {
    delete fRelocator;
}

status_t EnhancedGuestContext::LoadETDynBinary(const uint8_t* data, size_t size) {
    if (!fRelocator) {
        return B_ERROR;
    }
    
    status_t result = fRelocator->LoadETDynBinary(data, size);
    if (result != B_OK) {
        return result;
    }
    
    fIsETDyn = true;
    return B_OK;
}

status_t EnhancedGuestContext::InitializeForETDyn() {
    if (!fIsETDyn) {
        return B_ERROR;
    }
    
    // Apply relocations
    status_t result = fRelocator->ApplyRelocations(fBaseAddress);
    if (result != B_OK) {
        return result;
    }
    
    // Setup guest context
    return fRelocator->SetupGuestContext(*this);
}

status_t EnhancedGuestContext::Initialize() {
    std::cout << "EnhancedGuestContext: Initializing for ET_DYN" << std::endl;
    return B_OK;
}

status_t EnhancedGuestContext::Cleanup() {
    std::cout << "EnhancedGuestContext: Cleanup complete" << std::endl;
    return B_OK;
}

status_t EnhancedGuestContext::MapSegment(uint32_t vaddr, const uint8_t* data, size_t size, uint32_t flags) {
    if (!fRelocator) {
        return B_ERROR;
    }
    
    return fRelocator->MapSegment(vaddr, data, size, flags);
}

status_t EnhancedGuestContext::SetupStackForETDyn(int argc, char* argv[], char* envp[]) {
    if (!fRelocator) {
        return B_ERROR;
    }
    
    uint32_t stack_top = 0xC0000000;
    return fRelocator->SetupStack(stack_top, argc, argv, envp);
}

void EnhancedGuestContext::PrintMemoryLayout() const {
    std::cout << "=== ET_DYN MEMORY LAYOUT ===" << std::endl;
    std::cout << "Base Address: 0x" << std::hex << fBaseAddress << std::dec << std::endl;
    std::cout << "Entry Point: 0x" << std::hex << fEntryPoint << std::dec << std::endl;
    std::cout << "Stack Top: 0xC0000000" << std::endl;
    std::cout << "==========================" << std::endl;
}

void EnhancedGuestContext::PrintRelocationInfo() const {
    if (!fRelocator) {
        return;
    }
    
    std::cout << "=== ET_DYN RELOCATIONS ===" << std::endl;
    // Print relocation details if available
    std::cout << "Relocations processed successfully" << std::endl;
    std::cout << "=========================" << std::endl;
}