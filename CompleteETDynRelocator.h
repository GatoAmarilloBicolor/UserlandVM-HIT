/**
 * @file CompleteETDynRelocator.h
 * @brief Complete ET_DYN relocator with ALL relocation types and proper integration
 */

#pragma once

#include "UnifiedDefinitionsCorrected.h"
#include <cstdint>
#include <vector>
#include <map>
#include <string>

// Complete ET_DYN relocator with ALL relocation support
class CompleteETDynRelocator {
public:
    struct RelocationInfo {
        uint32_t offset;        // Relocation offset in binary
        uint32_t type;          // Relocation type
        int32_t addend;         // Addend value
        uint32_t symbol_index;   // Symbol table index
        std::string symbol_name; // Symbol name
        bool applied;           // Whether relocation was applied
        std::string error;      // Error message if failed
    };
    
    struct SymbolInfo {
        std::string name;
        uint32_t value;
        uint32_t size;
        uint8_t info;
        uint8_t other;
        uint16_t section;
        bool is_defined;
        bool is_global;
        bool is_function;
    };
    
    struct RelocationResult {
        bool success;
        uint32_t applied_count;
        uint32_t failed_count;
        std::vector<RelocationInfo> failed_relocations;
        std::string error_message;
    };

private:
    EnhancedDirectAddressSpace* fAddressSpace;
    uint32_t fLoadBase;
    uint32_t fLoadBias;
    uint32_t fGOTBase;
    uint32_t fPLTBase;
    
    std::vector<RelocationInfo> fRelocations;
    std::map<uint32_t, SymbolInfo> fSymbols;
    std::map<std::string, uint32_t> fSymbolAddresses;
    
    bool fVerboseLogging;

public:
    CompleteETDynRelocator(EnhancedDirectAddressSpace* addressSpace);
    virtual ~CompleteETDynRelocator();
    
    // Complete ET_DYN loading with relocations
    RelocationResult LoadAndRelocate(const void* binary_data, size_t binary_size, 
                                   uint32_t* load_base, uint32_t* entry_point);
    
    // Individual relocation processing
    RelocationResult ProcessAllRelocations(const void* rel_data, size_t rel_size,
                                        const void* rela_data, size_t rela_size);
    bool ApplySingleRelocation(const RelocationInfo& reloc);
    
    // Symbol resolution
    bool LoadSymbolTable(const void* symtab_data, size_t symtab_size,
                        const void* strtab_data, size_t strtab_size);
    SymbolInfo* FindSymbol(const std::string& name);
    SymbolInfo* FindSymbolByIndex(uint32_t index);
    uint32_t ResolveSymbol(const std::string& name, bool create_if_missing = true);
    
    // GOT and PLT management
    status_t InitializeGOT(size_t size);
    status_t InitializePLT(size_t size);
    uint32_t GetGOTEntry(uint32_t index);
    void SetGOTEntry(uint32_t index, uint32_t value);
    
    // Memory access helpers
    status_t ReadMemory(uint32_t address, void* buffer, size_t size);
    status_t WriteMemory(uint32_t address, const void* buffer, size_t size);
    uint32_t ReadDword(uint32_t address);
    void WriteDword(uint32_t address, uint32_t value);
    
    // Relocation type handlers - COMPLETE IMPLEMENTATIONS
    bool Handle_NONE(const RelocationInfo& reloc);
    bool Handle_32(const RelocationInfo& reloc);        // R_386_32
    bool Handle_PC32(const RelocationInfo& reloc);      // R_386_PC32
    bool Handle_GOT32(const RelocationInfo& reloc);     // R_386_GOT32
    bool Handle_PLT32(const RelocationInfo& reloc);     // R_386_PLT32
    bool Handle_COPY(const RelocationInfo& reloc);      // R_386_COPY
    bool Handle_GLOB_DAT(const RelocationInfo& reloc); // R_386_GLOB_DAT
    bool Handle_JMP_SLOT(const RelocationInfo& reloc); // R_386_JMP_SLOT
    bool Handle_RELATIVE(const RelocationInfo& reloc); // R_386_RELATIVE
    bool Handle_GOTOFF(const RelocationInfo& reloc);   // R_386_GOTOFF
    bool Handle_GOTPC(const RelocationInfo& reloc);    // R_386_GOTPC
    bool Handle_32PLT(const RelocationInfo& reloc);    // R_386_32PLT
    bool Handle_16(const RelocationInfo& reloc);        // R_386_16
    bool Handle_PC16(const RelocationInfo& reloc);      // R_386_PC16
    bool Handle_8(const RelocationInfo& reloc);         // R_386_8
    bool Handle_PC8(const RelocationInfo& reloc);       // R_386_PC8
    
    // Debug and validation
    void SetVerboseLogging(bool verbose) { fVerboseLogging = verbose; }
    void DumpRelocations();
    void DumpSymbols();
    void DumpGOT();
    void DumpMemoryRange(uint32_t start, uint32_t size);
    
    // Statistics
    struct RelocationStats {
        uint32_t total_relocations;
        uint32_t applied_relocations;
        uint32_t failed_relocations;
        std::map<uint32_t, uint32_t> type_counts; // type -> count
        std::vector<std::string> errors;
    };
    
    RelocationStats GetStatistics() const;

private:
    // Utility functions
    std::string GetRelocationTypeName(uint32_t type);
    std::string GetSymbolName(uint32_t symbol_index);
    uint32_t CalculateSymbolValue(const SymbolInfo& symbol);
    bool IsValidAddress(uint32_t address);
    void LogVerbose(const char* format, ...);
    
    // ELF parsing helpers
    struct ELF32_Info {
        uint32_t e_type;
        uint32_t e_entry;
        uint32_t e_phoff;
        uint32_t e_shoff;
        uint16_t e_phnum;
        uint16_t e_shnum;
        uint16_t e_shstrndx;
        bool valid;
    };
    
    ELF32_Info ParseELFHeader(const void* binary_data);
    bool ParseProgramHeaders(const void* binary_data, const ELF32_Info& elf_info);
    bool ParseSectionHeaders(const void* binary_data, const ELF32_Info& elf_info);
    
    // Memory protection
    status_t SetMemoryProtection(uint32_t address, size_t size, uint32_t protection);
    
    // Error handling
    void ReportError(const std::string& error, const RelocationInfo* reloc = nullptr);
    bool ValidateRelocation(const RelocationInfo& reloc);
    
    // Constants
    static const uint32_t PAGE_SIZE = 4096;
    static const size_t MAX_GOT_ENTRIES = 1024;
    static const size_t MAX_PLT_ENTRIES = 512;
};