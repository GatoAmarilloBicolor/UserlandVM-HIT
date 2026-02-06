/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * HaikuDynamicLinker.h - Haiku-specific dynamic linking with system loader integration
 */

#ifndef HAIKU_DYNAMIC_LINKER_H
#define HAIKU_DYNAMIC_LINKER_H

#include <SupportDefs.h>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

class HaikuDynamicLinker {
public:
    // Enhanced library information for Haiku
    struct HaikuLibraryInfo {
        std::string library_name;
        std::string library_path;
        std::string soname;
        void* handle;              // dlopen handle
        void* base_address;         // Library base address
        size_t size;                // Library size
        uint32_t reference_count;   // Reference count
        bool is_system_library;       // Is this a system library
        bool is_loaded;              // Load status
        std::vector<std::string> dependencies; // DT_NEEDED entries
        
        HaikuLibraryInfo() : handle(nullptr), base_address(nullptr), size(0), 
                               reference_count(0), is_system_library(false), is_loaded(false) {}
    };

    // Haiku-specific dynamic section information
    struct HaikuDynamicSection {
        std::vector<std::string> needed_libraries;  // DT_NEEDED
        std::string soname;            // DT_SONAME
        std::string rpath;              // DT_RPATH
        std::string runpath;            // DT_RUNPATH
        uint32_t init_function;        // DT_INIT
        uint32_t fini_function;        // DT_FINI
        std::vector<uint32_t> init_array;  // DT_INIT_ARRAY
        std::vector<uint32_t> fini_array;  // DT_FINI_ARRAY
        uint32_t tls_module;          // DT_TLS
        uint32_t tls_offset;          // DT_TLS offset
        uint64_t tls_size;             // DT_TLS size
        uint32_t tls_align;             // DT_TLS align
        
        HaikuDynamicSection() : init_function(0), fini_function(0), tls_module(0),
                              tls_offset(0), tls_size(0), tls_align(0) {}
    };

    // Performance metrics for Haiku dynamic linking
    struct HaikuDynamicMetrics {
        uint64_t libraries_loaded;
        uint64_t symbols_resolved;
        uint64_t relocations_processed;
        uint64_t system_calls_bypassed;
        uint64_t cache_hits;
        uint64_t cache_misses;
        double avg_load_time_ms;
        double avg_resolve_time_us;
        
        HaikuDynamicMetrics() : libraries_loaded(0), symbols_resolved(0),
                                 relocations_processed(0), system_calls_bypassed(0),
                                 cache_hits(0), cache_misses(0),
                                 avg_load_time_ms(0.0), avg_resolve_time_us(0.0) {}
    };

private:
    // Library registry
    std::map<std::string, HaikuLibraryInfo> fLoadedLibraries;
    
    // Search paths specific to Haiku
    std::vector<std::string> fSearchPaths;
    
    // Symbol cache for performance
    struct SymbolCache {
        std::string symbol_name;
        void* symbol_address;
        size_t symbol_size;
        std::string library_name;
        uint64_t access_count;
        
        SymbolCache() : symbol_address(nullptr), symbol_size(0), access_count(0) {}
    };
    
    static constexpr size_t SYMBOL_CACHE_SIZE = 1024;
    SymbolCache fSymbolCache[SYMBOL_CACHE_SIZE];
    size_t fCacheIndex;
    
    // Performance tracking
    HaikuDynamicMetrics fMetrics;
    
    // System integration
    bool fUseSystemLoader;         // Use dlopen/dlsym instead of custom ELF loader
    bool fLazyBindingEnabled;       // Enable lazy binding
    bool fDebugMode;               // Debug output

public:
    HaikuDynamicLinker();
    ~HaikuDynamicLinker();
    
    // Initialization
    status_t Initialize(bool use_system_loader = true, bool enable_lazy_binding = true, bool debug_mode = false);
    
    // Main loading interface
    HaikuLibraryInfo* LoadLibrary(const char* library_path);
    bool UnloadLibrary(const char* library_name);
    bool IsLibraryLoaded(const char* library_name) const;
    HaikuLibraryInfo* GetLibraryInfo(const char* library_name) const;
    
    // Dynamic section parsing
    HaikuDynamicSection ParseDynamicSection(const char* library_path);
    bool LoadDependencies(const char* executable_path);
    
    // Symbol resolution with caching
    bool ResolveSymbol(const char* symbol_name, void** address, size_t* size = nullptr);
    void ClearSymbolCache();
    
    // Haiku-specific operations
    status_t LoadHaikuSystemLibrary(const char* library_name);
    status_t LoadBeCompatibleLibrary(const char* library_name);
    status_t LoadNetworkLibrary(const char* library_name);
    
    // TLS handling for Haiku
    status_t InitializeTLS();
    status_t SetupTLSForThread();
    void* GetTLSBase();
    
    // Performance and debugging
    HaikuDynamicMetrics GetMetrics() const { return fMetrics; }
    void ResetMetrics();
    void PrintMetrics() const;
    void PrintLibraryStatus() const;
    
    // Search path management
    void AddSearchPath(const char* path);
    void SetHaikuSearchPaths();
    
    // Advanced operations
    status_t PreloadCommonLibraries();
    status_t OptimizeSymbolCache();
    status_t CreateLibrarySymlink(const char* library_name, const char* target);

private:
    // Internal helper methods
    HaikuLibraryInfo* CreateLibraryInfo(const char* path, void* handle);
    bool ParseELFDynamicSection(const char* library_path, HaikuDynamicSection& section);
    bool ProcessRelocations(const HaikuDynamicSection& section, HaikuLibraryInfo& info);
    
    // Symbol cache management
    uint32_t GetSymbolHash(const char* symbol_name);
    void CacheSymbol(const char* symbol_name, void* address, size_t size, const char* library_name);
    SymbolCache* LookupSymbolCache(const char* symbol_name);
    bool UpdateSymbolCache(const char* symbol_name, void* address, size_t size, const char* library_name);
    
    // Haiku-specific helpers
    bool IsHaikuSystemLibrary(const char* library_name);
    std::string GetHaikuLibraryPath(const char* library_name);
    bool ValidateHaikuLibrary(const void* handle);
    
    // Performance tracking
    void RecordLoadOperation(const char* operation, double time_ms);
    void RecordSymbolResolution(const char* symbol_name, double time_us);
    void RecordCacheHit(const char* symbol_name);
    void RecordCacheMiss(const char* symbol_name);
    
    // Utility methods
    const char* GetLibraryName(const char* path);
    std::string GetLibraryPath(const char* library_name);
    bool FileExists(const char* path);
    
    // Error handling
    void LogError(const char* operation, const char* details);
    void LogInfo(const char* operation, const char* details);
};

#endif // HAIKU_DYNAMIC_LINKER_H