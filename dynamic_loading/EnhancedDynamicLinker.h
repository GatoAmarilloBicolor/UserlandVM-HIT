/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * EnhancedDynamicLinker.h - High-performance dynamic linker with dependency graph and hash tables
 */

#ifndef ENHANCED_DYNAMIC_LINKER_H
#define ENHANCED_DYNAMIC_LINKER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cstdint>

// Forward declarations
class ElfImage;
class SymbolResolver;

class EnhancedDynamicLinker {
public:
    // Library load status for tracking
    enum LoadStatus {
        UNLOADED = 0,
        LOADING = 1,
        LOADED = 2,
        RELOCATED = 3,
        INITIALIZED = 4,
        FAILED = 5
    };

    // Library information structure
    struct LibraryInfo {
        std::string name;
        std::string path;
        std::string soname;
        LoadStatus status;
        uint32_t load_time_ms;
        uint32_t ref_count;
        std::shared_ptr<ElfImage> image;
        std::vector<std::string> dependencies;
        std::vector<std::string> dependents;
        
        LibraryInfo() : status(UNLOADED), load_time_ms(0), ref_count(0) {}
    };

    // Dependency graph for topological sorting
    class DependencyGraph {
    private:
        std::unordered_map<std::string, std::vector<std::string>> fEdges;
        std::unordered_map<std::string, int> fInDegree;
        std::vector<std::string> fTopologicalOrder;
        bool fHasCycles;
        
    public:
        DependencyGraph() : fHasCycles(false) {}
        
        void AddDependency(const std::string& dependent, const std::string& dependency);
        bool BuildTopologicalOrder();
        bool HasCycles() const { return fHasCycles; }
        std::vector<std::string> GetLoadOrder() const { return fTopologicalOrder; }
        void Clear();
        void Dump() const;
    };

    // Performance tracking
    struct PerformanceMetrics {
        uint64_t total_libraries_loaded;
        uint64_t total_symbols_resolved;
        uint64_t total_relocations_processed;
        double avg_load_time_ms;
        double avg_resolve_time_us;
        uint64_t cache_hits;
        uint64_t cache_misses;
        
        PerformanceMetrics() : total_libraries_loaded(0), total_symbols_resolved(0),
                            total_relocations_processed(0), avg_load_time_ms(0.0),
                            avg_resolve_time_us(0.0), cache_hits(0), cache_misses(0) {}
    };

private:
    // Registry of loaded libraries
    std::unordered_map<std::string, std::shared_ptr<LibraryInfo>> fLibraries;
    
    // Dependency graph
    DependencyGraph fDependencyGraph;
    
    // Symbol resolver for fast lookups
    std::unique_ptr<SymbolResolver> fSymbolResolver;
    
    // Search paths for library resolution
    std::vector<std::string> fSearchPaths;
    
    // Critical libraries that must be loaded first
    std::unordered_set<std::string> fCriticalLibraries;
    
    // Performance metrics
    PerformanceMetrics fMetrics;
    
    // State tracking
    bool fInitialized;
    std::string fWorkingDirectory;

public:
    EnhancedDynamicLinker();
    ~EnhancedDynamicLinker();
    
    // Initialization and configuration
    bool Initialize();
    void AddSearchPath(const std::string& path);
    void SetWorkingDirectory(const std::string& path);
    void AddCriticalLibrary(const std::string& lib_name);
    
    // Core loading functionality
    std::shared_ptr<ElfImage> LoadLibrary(const std::string& path);
    std::shared_ptr<ElfImage> LoadLibraryWithDependencies(const std::string& path);
    bool LoadCriticalLibraries();
    
    // Dependency management
    bool ResolveDependencies(const std::string& main_binary);
    std::vector<std::string> GetLoadOrder(const std::string& library);
    bool CheckDependencies(const std::string& library);
    
    // Symbol resolution
    bool FindSymbol(const std::string& name, void** address, size_t* size = nullptr);
    bool FindSymbolInLibrary(const std::string& library_name, const std::string& name, 
                           void** address, size_t* size = nullptr);
    
    // Library management
    bool UnloadLibrary(const std::string& name);
    bool IsLibraryLoaded(const std::string& name) const;
    std::vector<std::string> GetLoadedLibraries() const;
    LibraryInfo* GetLibraryInfo(const std::string& name);
    
    // Relocation processing
    bool ProcessRelocations();
    bool ProcessLibraryRelocations(const std::string& library_name);
    
    // Initialization and cleanup
    bool RunInitializers();
    bool RunInitializersForLibrary(const std::string& library_name);
    bool RunFinalizers();
    
    // Cache management
    void ClearSymbolCache();
    void PreloadCommonSymbols();
    
    // Performance and debugging
    PerformanceMetrics GetMetrics() const { return fMetrics; }
    void ResetMetrics();
    void PrintLibraryStatus() const;
    void DumpDependencyGraph() const;

private:
    // Internal helper methods
    std::string ResolveLibraryPath(const std::string& name);
    bool LoadLibraryInternal(const std::string& path, std::shared_ptr<LibraryInfo>& info);
    bool ParseLibraryDependencies(std::shared_ptr<LibraryInfo> info);
    bool ApplyRelocations(std::shared_ptr<LibraryInfo> info);
    
    // Symbol resolution helpers
    bool FindSymbolInDependencies(const std::string& name, void** address, size_t* size);
    bool FindSymbolInSingleLibrary(const std::string& library_name, const std::string& name, 
                                void** address, size_t* size);
    
    // Path resolution helpers
    bool FileExists(const std::string& path) const;
    std::string FindLibraryInPaths(const std::string& name) const;
    
    // Reference counting
    void IncrementRefCount(const std::string& library_name);
    void DecrementRefCount(const std::string& library_name);
    
    // Critical library loading
    bool LoadLibC();
    bool LoadLibRoot();
    bool LoadLdHaiku();
    bool LoadBe();
    bool LoadBsd();
    
    // Error handling and logging
    void LogError(const std::string& message) const;
    void LogInfo(const std::string& message) const;
    void LogPerformance(const std::string& operation, double time_ms) const;
    
    // Utility methods
    std::string GetLibraryName(const std::string& path) const;
    std::string GetDirectory(const std::string& path) const;
    uint64_t GetTimestamp() const;
};

#endif // ENHANCED_DYNAMIC_LINKER_H