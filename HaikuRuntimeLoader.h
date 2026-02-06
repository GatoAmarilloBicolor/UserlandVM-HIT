/*
 * Haiku PT_INTERP Runtime Loader
 * Intelligently loads runtime_loader and its dependencies
 */

#pragma once

#include "SupportDefs.h"
#include <string>
#include <vector>
#include <memory>

class GuestMemory;

// Runtime loader information
struct RuntimeLoaderInfo {
    std::string path;
    uint32_t load_address;
    uint32_t entry_point;
    bool is_loaded;
    
    RuntimeLoaderInfo() : load_address(0), entry_point(0), is_loaded(false) {}
};

// Library information for dynamic linking
struct LibraryInfo {
    std::string name;
    std::string path;
    uint32_t base_address;
    std::vector<uint32_t> symbols;
    bool is_loaded;
    
    LibraryInfo() : base_address(0), is_loaded(false) {}
};

class HaikuRuntimeLoader {
private:
    GuestMemory& fMemory;
    RuntimeLoaderInfo fRuntimeLoader;
    std::vector<LibraryInfo> fLoadedLibraries;
    uint32_t fNextLoadAddress;
    
public:
    HaikuRuntimeLoader(GuestMemory& memory);
    ~HaikuRuntimeLoader() = default;
    
    // Main runtime loader operations
    status_t LoadRuntimeLoader(const char* interpreter_path);
    status_t ExecuteRuntimeLoader();
    
    // Library operations
    status_t LoadLibrary(const char* lib_name);
    status_t ResolveSymbol(const char* symbol_name, uint32_t& address);
    
    // Dynamic linking operations
    status_t ApplyRelocations(uint32_t rel_addr, uint32_t rel_count, uint32_t base_addr);
    status_t ProcessDynamicSegment(uint32_t dynamic_addr, uint32_t base_addr);
    
    // Status information
    bool IsRuntimeLoaderLoaded() const { return fRuntimeLoader.is_loaded; }
    uint32_t GetRuntimeLoaderEntry() const { return fRuntimeLoader.entry_point; }
    
private:
    // Internal helper methods
    status_t LoadELFSegment(const char* file_path, uint32_t& base_addr, uint32_t& entry_point);
    status_t ParseELFHeader(const char* file_path, uint32_t& entry_point, bool& is_dynamic);
    status_t CopyRuntimeLoaderToGuest(const char* file_path, uint32_t& guest_addr);
    
    // Symbol resolution helpers
    status_t FindSymbolInLibrary(const char* symbol_name, LibraryInfo& lib, uint32_t& address);
    status_t FindSymbolInRuntimeLoader(const char* symbol_name, uint32_t& address);
    
    // Memory management helpers
    uint32_t AllocateGuestMemory(size_t size);
    bool WriteGuestMemory(uint32_t addr, const void* data, size_t size);
    bool ReadGuestMemory(uint32_t addr, void* data, size_t size);
    
    // Library management
    LibraryInfo* FindLoadedLibrary(const char* lib_name);
    status_t LoadStandardLibrary(const char* lib_name);
    
    // Haiku-specific library paths
    std::string FindLibraryPath(const char* lib_name);
    const char* GetStandardLibraryPath(const char* lib_name);
};