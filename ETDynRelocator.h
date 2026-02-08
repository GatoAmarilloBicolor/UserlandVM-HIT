#pragma once

#include "GuestContext.h"
#include "AddressSpace.h"
#include "ELFImage.h"
#include "SupportDefs.h"
#include <cstdint>
#include <vector>
#include <string>

// Constants for ELF relocation types
#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2
#define R_386_GOT32 3
#define R_386_PLT32 4
#define R_386_COPY 5
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8
#define R_386_GOTOFF 9
#define R_386_GOTPC 10

// ET_DYN Relocation support for Guest Context
// Handles PIE (Position Independent Executable) binaries

struct ETDynRelocation {
    uint32_t offset;        // Offset in binary
    uint32_t info;          // Relocation info
    uint32_t addend;        // Addend value
    uint32_t target_addr;    // Target address after relocation
    std::string symbol_name; // Symbol name if applicable
};

struct ETDynSymbol {
    std::string name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t shndx;
};

class ETDynRelocator {
private:
    AddressSpace& fAddressSpace;
    uint32_t fBaseAddress;
    uint32_t fLoadBias;
    
    std::vector<ETDynRelocation> fRelocations;
    std::vector<ETDynSymbol> fSymbols;
    std::vector<uint32_t> fGotEntries;
    
public:
    ETDynRelocator(AddressSpace& addressSpace);
    ~ETDynRelocator();
    
    // Load and analyze ET_DYN binary
    status_t LoadETDynBinary(const uint8_t* data, size_t size);
    
    // Apply relocations
    status_t ApplyRelocations(uint32_t base_address);
    
    // Resolve symbols
    uint32_t ResolveSymbol(const std::string& name);
    
    // Guest context setup
    status_t SetupGuestContext(GuestContext& context);
    
    // Memory layout management
    status_t SetupMemoryLayout(uint32_t& base_addr, uint32_t& stack_addr);
    
    // Stack setup with argc/argv
    status_t SetupStack(uint32_t stack_top, int argc, char* argv[], char* envp[]);
    
private:
    // Parse ELF sections
    status_t ParseELFHeader(const uint8_t* data, size_t size);
    status_t ParseProgramHeaders(const uint8_t* data);
    status_t ParseSectionHeaders(const uint8_t* data);
    status_t ParseDynamicSection(const uint8_t* data);
    status_t ParseRelocations(const uint8_t* data);
    status_t ParseSymbols(const uint8_t* data);
    
    // Relocation processing
    status_t ProcessRelocation(const ETDynRelocation& reloc);
    status_t ProcessRelativeRelocation(const ETDynRelocation& reloc);
    status_t ProcessAbsoluteRelocation(const ETDynRelocation& reloc);
    status_t ProcessPltRelocation(const ETDynRelocation& reloc);
    
    // Helper methods
    uint32_t CalculateLoadBias(const Elf32_Phdr* phdr, int phnum);
    uint32_t GetSymbolAddress(const std::string& name);
    status_t WriteGuestMemory(uint32_t addr, const void* data, size_t size);
    status_t ReadGuestMemory(uint32_t addr, void* data, size_t size);
};

// Enhanced Guest Context with ET_DYN support
class EnhancedGuestContext : public GuestContext {
private:
    ETDynRelocator* fRelocator;
    uint32_t fBaseAddress;
    uint32_t fEntryPoint;
    bool fIsETDyn;
    
public:
    EnhancedGuestContext(AddressSpace& addressSpace);
    ~EnhancedGuestContext();
    
    // ET_DYN specific methods
    status_t LoadETDynBinary(const uint8_t* data, size_t size);
    status_t InitializeForETDyn();
    
    // Override base class methods  
    status_t Initialize();
    status_t Cleanup();
    
    // Memory management
    status_t SetupStackForETDyn(int argc, char* argv[], char* envp[]);
    
    // Accessors
    uint32_t GetBaseAddress() const { return fBaseAddress; }
    uint32_t GetEntryPoint() const { return fEntryPoint; }
    bool IsETDyn() const { return fIsETDyn; }
    
    // Debug helpers
    void PrintMemoryLayout() const;
    void PrintRelocationInfo() const;
    
private:
    // Internal helpers
    status_t ValidateETDynBinary(const uint8_t* data, size_t size);
    status_t LoadProgramHeaders(const uint8_t* data);
    status_t LoadDynamicSection(const uint8_t* data);
};