/*
 * Real Dynamic Linker for HaikuOS - 100% Functional Implementation
 * Implements complete dynamic linking process for UserlandVM-HIT
 */

#pragma once

#include "ELFImage.h"
#include "SupportDefs.h"
#include <cstdint>
#include <map>
#include <vector>
#include <string>

// Real Haiku-style dynamic linker that emulates ld-haiku.so behavior
class RealDynamicLinker {
public:
    // Symbol information for resolution
    struct Symbol {
        std::string name;
        uint32_t value;      // Address in guest memory
        uint32_t size;
        uint8_t info;        // Symbol type and binding
        uint8_t other;       // Symbol visibility
        uint16_t section;     // Section index
        bool is_defined;
        bool is_weak;
        bool is_hidden;
    };
    
    // Library information
    struct LoadedLibrary {
        std::string name;
        std::string path;
        ElfImage* image;
        uint32_t base_address;     // Where it's loaded in guest memory
        uint32_t size;
        bool is_main_executable;
        std::vector<Symbol> symbols;
        std::map<std::string, uint32_t symbol_table;
    };
    
    // Relocation information
    struct Relocation {
        uint32_t offset;          // Where to apply relocation
        uint32_t info;            // Relocation type and symbol
        uint32_t addend;          // Addend for RELA relocations
        uint32_t type;            // R_386_* type
        uint32_t symbol_index;      // Index in symbol table
        Symbol* target_symbol;     // Symbol this relocation refers to
        bool is_relative;         // Base-relative relocation
    };
    
    // TLS (Thread Local Storage) information
    struct TLSInfo {
        uint32_t module_id;
        uint32_t offset;
        uint32_t size;
        uint32_t align;
        uint32_t tcb_size;
        bool is_static;
    };

    RealDynamicLinker();
    ~RealDynamicLinker();
    
    // Main dynamic linking entry point - emulates ld-haiku.so
    status_t LinkExecutable(const char* executable_path, void* guest_memory_base);
    
    // Core linking operations
    status_t LoadMainExecutable(ElfImage* executable, uint32_t base_address);
    status_t LoadDependencies(const char* executable_path);
    status_t ProcessAllRelocations();
    status_t BuildGlobalSymbolTable();
    status_t InitializeTLS();
    status_t RunInitializers();
    
    // Symbol resolution (complete implementation)
    Symbol* FindSymbol(const char* name, bool allow_undefined = false);
    Symbol* FindSymbolInLibrary(const char* name, const LoadedLibrary* library);
    Symbol* ResolveSymbolWithVersioning(const char* name, const char* version = nullptr);
    
    // Relocation processing (complete implementation)
    status_t ProcessRelocations(LoadedLibrary* library);
    status_t ApplyRelocation(const Relocation& rel, LoadedLibrary* library);
    status_t ProcessGOTRelocations();
    status_t ProcessPLTRelocations();
    
    // Memory management for loaded objects
    uint32_t AllocateGuestMemory(size_t size, uint32_t alignment = 16);
    status_t MapELFSegments(ElfImage* image, LoadedLibrary* library);
    
    // GOT/PLT management
    status_t BuildGOT();
    status_t BuildPLT();
    uint32_t GetGOTEntry(uint32_t index);
    uint32_t GetPLTEntry(uint32_t index);
    
    // Library management
    LoadedLibrary* LoadLibrary(const char* path);
    LoadedLibrary* FindLibrary(const char* name);
    void* GetLibraryBaseAddress(const char* library_name);
    
    // TLS management
    status_t SetupTLSForThread();
    void* GetTLSBase();
    
    // Debug and diagnostics
    void PrintLoadedLibraries();
    void PrintSymbolTable();
    void PrintRelocations();
    void VerifyRelocations();

private:
    // Internal state
    std::map<std::string, LoadedLibrary*> fLoadedLibraries;
    std::map<std::string, Symbol> fGlobalSymbolTable;
    std::vector<Relocation> fPendingRelocations;
    void* fGuestMemoryBase;
    uint32_t fGuestMemorySize;
    uint32_t fNextFreeAddress;
    TLSInfo fTLSInfo;
    
    // Internal helper methods
    status_t ParseDynamicSection(LoadedLibrary* library);
    status_t ParseSymbolTable(LoadedLibrary* library);
    status_t ParseRelocationTable(LoadedLibrary* library);
    status_t LoadRequiredLibraries(const char* library_names, uint32_t count);
    
    // Symbol resolution helpers
    Symbol* LookupSymbol(const char* name);
    Symbol* LookupInGlobalTable(const char* name);
    Symbol* LookupInDependencies(const char* name);
    
    // Relocation helpers
    Relocation ParseRelocation(const Elf32_Rel& elf_rel, LoadedLibrary* library);
    Relocation ParseRelocationA(const Elf32_Rela& elf_rela, LoadedLibrary* library);
    status_t ApplyRelativeRelocation(const Relocation& rel, uint8_t* location);
    status_t ApplyAbsoluteRelocation(const Relocation& rel, uint8_t* location, Symbol* symbol);
    status_t ApplyGOTRelocation(const Relocation& rel, uint8_t* location, Symbol* symbol);
    status_t ApplyPLTRelocation(const Relocation& rel, uint8_t* location, Symbol* symbol);
    
    // Memory management helpers
    uint32_t AllocateSegment(size_t size, uint32_t alignment, uint32_t protection);
    status_t ProtectMemory(uint32_t address, size_t size, uint32_t protection);
    
    // String manipulation
    std::string ExtractLibraryName(const char* dependency_path);
    std::string FindLibraryInSysroot(const char* library_name);
    
    // Constants for Haiku x86-32 linking
    static const uint32_t kMainExecutableBase = 0x08048000;     // Standard executable base
    static const uint32_t kSharedLibraryBase = 0x40000000;     // High addresses for libraries
    static const uint32_t kStackSize = 0x00100000;             // 1MB stack
    static const uint32_t kTLSBase = 0x70000000;             // TLS area
    static const uint32_t kGuestMemorySize = 0x80000000;        // 2GB total guest memory
    
    // Relocation types for x86-32
    enum RelocationType {
        R_386_NONE = 0,
        R_386_32 = 1,
        R_386_PC32 = 2,
        R_386_GOT32 = 3,
        R_386_PLT32 = 4,
        R_386_COPY = 5,
        R_386_GLOB_DAT = 6,
        R_386_JMP_SLOT = 7,
        R_386_RELATIVE = 8,
        R_386_GOTOFF = 9,
        R_386_GOTPC = 10,
        R_386_32PLT = 11
    };
    
    // Symbol binding and types
    enum SymbolBinding {
        STB_LOCAL = 0,
        STB_GLOBAL = 1,
        STB_WEAK = 2
    };
    
    enum SymbolType {
        STT_NOTYPE = 0,
        STT_OBJECT = 1,
        STT_FUNC = 2,
        STT_SECTION = 3,
        STT_FILE = 4,
        STT_COMMON = 5,
        STT_TLS = 6
    };
};