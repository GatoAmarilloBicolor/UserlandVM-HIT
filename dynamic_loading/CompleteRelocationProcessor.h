/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * CompleteRelocationProcessor.h - Complete x86-32 relocation processor for maximum compatibility
 */

#ifndef COMPLETE_RELOCATION_PROCESSOR_H
#define COMPLETE_RELOCATION_PROCESSOR_H

#include <vector>
#include <cstdint>
#include <string>

// Forward declarations
class ElfImage;
class EnhancedDynamicLinker;

class CompleteRelocationProcessor {
public:
    // x86-32 relocation types (complete set)
    enum RelocType32 {
        R_386_NONE = 0,        // No reloc
        R_386_32 = 1,          // Direct 32 bit absolute
        R_386_PC32 = 2,        // PC relative 32 bit signed
        R_386_GOT32 = 3,       // 32 bit GOT entry
        R_386_PLT32 = 4,       // 32 bit PLT address
        R_386_COPY = 5,         // Copy symbol at runtime
        R_386_GLOB_DAT = 6,     // Create GOT entry
        R_386_JMP_SLOT = 7,     // Create PLT entry
        R_386_RELATIVE = 8,     // Adjust by program base
        R_386_GOTOFF = 9,       // 32 bit offset to GOT
        R_386_GOTPC = 10,      // 32 bit PC relative offset to GOT
        
        // GNU extensions
        R_386_32PLT = 11,      // 32 bit PLT address (GNU extension)
        R_386_TLS_TPOFF = 14,   // Offset in static TLS block
        R_386_TLS_IE = 15,      // Address of GOT entry for static TLS block offset
        R_386_TLS_GOTDESC = 16, // GOT entry with TLS descriptor
        R_386_TLS_DESC_CALL = 17, // TLS descriptor call
        R_386_TLS_DESC = 18,    // TLS descriptor
        
        R_386_IRELATIVE = 42    // Indirect relative relocation
    };

    // Relocation entry structures
    struct RelEntry {
        uint32_t r_offset;     // Location to apply relocation
        uint32_t r_info;       // Symbol index + relocation type
    };
    
    struct RelaEntry {
        uint32_t r_offset;     // Location to apply relocation
        uint32_t r_info;       // Symbol index + relocation type
        int32_t r_addend;      // Constant value to add
    };

    // Symbol information needed for relocations
    struct SymbolInfo {
        uint32_t st_value;     // Symbol value/address
        uint32_t st_size;      // Symbol size
        uint8_t st_info;       // Symbol type and binding
        uint8_t st_other;      // Symbol visibility
        uint16_t st_shndx;     // Section index
        std::string name;       // Symbol name
        
        SymbolInfo() : st_value(0), st_size(0), st_info(0), st_other(0), st_shndx(0) {}
    };

    // Global Offset Table (GOT) structure
    struct GOTInfo {
        uint32_t* got_base;    // Base address of GOT
        uint32_t got_size;     // Size of GOT in entries
        uint32_t got_addr;     // Virtual address of GOT
        
        GOTInfo() : got_base(nullptr), got_size(0), got_addr(0) {}
    };

    // Procedure Linkage Table (PLT) structure
    struct PLTInfo {
        uint32_t* plt_base;     // Base address of PLT
        uint32_t plt_size;     // Size of PLT in entries
        uint32_t plt_addr;     // Virtual address of PLT
        uint32_t plt_rel_addr; // Address of relocations for PLT
        
        PLTInfo() : plt_base(nullptr), plt_size(0), plt_addr(0), plt_rel_addr(0) {}
    };

    // Relocation context for processing
    struct RelocationContext {
        ElfImage* image;                    // Image being relocated
        uint32_t image_base;                 // Base address of image
        uint32_t got_addr;                  // Address of GOT
        uint32_t plt_addr;                  // Address of PLT
        const char* string_table;            // String table for symbol names
        SymbolInfo* symbol_table;            // Symbol table
        uint32_t symbol_count;               // Number of symbols
        bool is_pie;                       // Position independent executable
        
        RelocationContext() : image(nullptr), image_base(0), got_addr(0), 
                             plt_addr(0), string_table(nullptr), 
                             symbol_table(nullptr), symbol_count(0), is_pie(false) {}
    };

    // Performance metrics
    struct RelocationMetrics {
        uint64_t total_relocations;
        uint64_t successful_relocations;
        uint64_t failed_relocations;
        uint64_t plt_relocations;
        uint64_t got_relocations;
        uint64_t relative_relocations;
        uint64_t copy_relocations;
        double avg_processing_time_us;
        
        RelocationMetrics() : total_relocations(0), successful_relocations(0),
                           failed_relocations(0), plt_relocations(0),
                           got_relocations(0), relative_relocations(0),
                           copy_relocations(0), avg_processing_time_us(0.0) {}
    };

private:
    // Relocation tables
    std::vector<RelEntry> fRelTable;
    std::vector<RelaEntry> fRelaTable;
    
    // GOT and PLT information
    GOTInfo fGOTInfo;
    PLTInfo fPLTInfo;
    
    // Relocation context
    RelocationContext fContext;
    
    // Performance tracking
    RelocationMetrics fMetrics;
    
    // Debug and logging
    bool fDebugMode;
    bool fLazyBinding;
    bool fBindNow;

public:
    CompleteRelocationProcessor(EnhancedDynamicLinker* linker);
    ~CompleteRelocationProcessor();
    
    // Main relocation processing
    bool ProcessRelocations(ElfImage* image);
    bool ProcessRelocationsWithBinding(ElfImage* image, bool bind_now = false);
    
    // Specific relocation type processing
    bool ProcessRelocations(ElfImage* image, const std::vector<RelEntry>& rels);
    bool ProcessRelocations(ElfImage* image, const std::vector<RelaEntry>& relas);
    
    // GOT and PLT setup
    bool SetupGOT(ElfImage* image);
    bool SetupPLT(ElfImage* image);
    
    // Individual relocation processing
    bool ApplyRelocation(const RelocationContext& ctx, const RelEntry& reloc);
    bool ApplyRelocation(const RelocationContext& ctx, const RelaEntry& reloc);
    bool ApplyRelocationByType(uint32_t reloc_type, uint32_t location, 
                              uint32_t symbol_value, uint32_t addend);
    
    // Symbol resolution for relocations
    bool ResolveSymbolForRelocation(uint32_t symbol_index, SymbolInfo& symbol_info);
    uint32_t GetSymbolValue(const SymbolInfo& symbol, const RelocationContext& ctx);
    
    // Lazy binding support
    bool SetupLazyBinding(ElfImage* image);
    bool BindLazySymbol(uint32_t plt_index);
    bool BindAllSymbols(ElfImage* image);
    
    // Relocation type handlers
    bool HandleNone(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool Handle32(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandlePC32(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleGOT32(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandlePLT32(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleCopy(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleGlobDat(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleJmpSlot(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleRelative(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleGOTOff(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleGOTPC(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleIRelative(uint32_t location, uint32_t symbol_value, uint32_t addend);
    
    // TLS relocations
    bool HandleTLSTPOff(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleTLSIE(uint32_t location, uint32_t symbol_value, uint32_t addend);
    bool HandleTLSDesc(uint32_t location, uint32_t symbol_value, uint32_t addend);
    
    // Utility methods
    const char* GetRelocationTypeName(uint32_t reloc_type) const;
    uint32_t GetRelocationSymbolIndex(uint32_t reloc_info) const;
    uint32_t GetRelocationType(uint32_t reloc_info) const;
    
    // Memory access helpers
    bool ReadMemory(uint32_t addr, void* buffer, size_t size);
    bool WriteMemory(uint32_t addr, const void* data, size_t size);
    uint32_t ReadUint32(uint32_t addr);
    bool WriteUint32(uint32_t addr, uint32_t value);
    
    // Debug and logging
    void SetDebugMode(bool enabled) { fDebugMode = enabled; }
    void SetLazyBinding(bool enabled) { fLazyBinding = enabled; }
    void SetBindNow(bool enabled) { fBindNow = enabled; }
    
    RelocationMetrics GetMetrics() const { return fMetrics; }
    void ResetMetrics();
    void PrintMetrics() const;
    
    // Dump and inspection
    void DumpRelocationTables() const;
    void DumpGOT() const;
    void DumpPLT() const;
    void DumpRelocationContext() const;

private:
    // Internal helpers
    bool LoadRelocationTables(ElfImage* image);
    bool SetupRelocationContext(ElfImage* image);
    
    // GOT and PLT internal methods
    bool AllocateGOT(size_t entries);
    bool AllocatePLT(size_t entries);
    bool WritePLTEntry(uint32_t index, uint32_t target_addr);
    
    // Symbol resolution helpers
    bool IsSymbolDefined(const SymbolInfo& symbol) const;
    bool IsSymbolWeak(const SymbolInfo& symbol) const;
    uint32_t GetSymbolAddress(const SymbolInfo& symbol, const RelocationContext& ctx) const;
    
    // Error handling
    void LogRelocationError(const char* operation, uint32_t addr, uint32_t reloc_type) const;
    void LogRelocationDebug(const char* message, uint32_t addr, uint32_t value) const;
    
    // Validation
    bool ValidateRelocation(const RelEntry& reloc, const RelocationContext& ctx) const;
    bool ValidateRelocation(const RelaEntry& reloc, const RelocationContext& ctx) const;
    bool CheckAddressAlignment(uint32_t addr, size_t size) const;
};

#endif // COMPLETE_RELOCATION_PROCESSOR_H