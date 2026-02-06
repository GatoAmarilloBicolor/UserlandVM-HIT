/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * SymbolResolver.h - High-performance symbol resolution with GNU hash and classic hash tables
 */

#ifndef SYMBOL_RESOLVER_H
#define SYMBOL_RESOLVER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

// Forward declarations
class ElfImage;

class SymbolResolver {
public:
    // Symbol information structure
    struct SymbolInfo {
        uint32_t address;
        uint32_t size;
        uint8_t type;      // STT_* type
        uint8_t binding;    // STB_* binding
        uint16_t section;    // SHN_* section index
        std::string name;
        
        SymbolInfo() : address(0), size(0), type(0), binding(0), section(0) {}
    };

    // Symbol lookup context
    struct LookupContext {
        const char* symbol_name;
        uint32_t symbol_hash;
        bool require_defined;
        bool allow_weak;
        
        LookupContext(const char* name, bool def = true, bool weak = true)
            : symbol_name(name), require_defined(def), allow_weak(weak) {
            symbol_hash = HashSymbolName(name);
        }
    };

private:
    // Classic ELF hash table structure
    struct ClassicHashTable {
        uint32_t nbuckets;
        uint32_t nchains;
        uint32_t* buckets;
        uint32_t* chains;
        const char* strings;
        
        ClassicHashTable() : nbuckets(0), nchains(0), buckets(nullptr), chains(nullptr), strings(nullptr) {}
        
        bool IsInitialized() const { return buckets && chains && strings; }
    };

    // GNU hash table structure (efficient)
    struct GnuHashTable {
        uint32_t nbuckets;
        uint32_t symoffset;
        uint32_t bloom_size;
        uint32_t bloom_shift;
        uint32_t* bloom_filter;
        uint32_t* buckets;
        uint32_t* chain;
        const char* strings;
        
        GnuHashTable() : nbuckets(0), symoffset(0), bloom_size(0), bloom_shift(0),
                         bloom_filter(nullptr), buckets(nullptr), chain(nullptr), strings(nullptr) {}
        
        bool IsInitialized() const { return bloom_filter && buckets && chain && strings; }
    };

    // Symbol cache for O(1) repeated lookups
    struct SymbolCache {
        std::unordered_map<std::string, SymbolInfo> cache;
        uint64_t hits;
        uint64_t misses;
        
        SymbolCache() : hits(0), misses(0) {}
        
        void RecordHit() { hits++; }
        void RecordMiss() { misses++; }
        double GetHitRate() const { return hits / (double)(hits + misses); }
    };

public:
    SymbolResolver();
    ~SymbolResolver();

    // Initialize hash tables for an ELF image
    bool InitializeClassicHash(ElfImage* image);
    bool InitializeGnuHash(ElfImage* image);
    
    // Primary symbol lookup methods
    bool FindSymbol(const std::string& name, SymbolInfo& info, ElfImage* image);
    bool FindSymbolOptimized(const LookupContext& context, SymbolInfo& info, ElfImage* image);
    
    // Batch symbol lookup for dependency resolution
    bool FindMultipleSymbols(const std::vector<std::string>& names, 
                           std::vector<SymbolInfo>& results, ElfImage* image);
    
    // Cache management
    void ClearCache();
    void PreloadCommonSymbols(ElfImage* image);
    SymbolCache* GetCacheStats() { return &fCache; }
    
    // Performance metrics
    struct PerformanceMetrics {
        uint64_t total_lookups;
        uint64_t classic_hash_lookups;
        uint64_t gnu_hash_lookups;
        uint64_t linear_searches;
        double avg_lookup_time_us;
        
        PerformanceMetrics() : total_lookups(0), classic_hash_lookups(0), 
                           gnu_hash_lookups(0), linear_searches(0), avg_lookup_time_us(0.0) {}
    };
    
    PerformanceMetrics GetMetrics() const { return fMetrics; }
    void ResetMetrics();

private:
    // Hash functions
    static uint32_t HashSymbolName(const char* name);
    static uint32_t GnuHashSymbolName(const char* name);
    static uint32_t BloomFilterHash(uint32_t hash1, uint32_t hash2);
    
    // Classic ELF hash lookup
    SymbolInfo* FindSymbolClassicHash(const std::string& name, ElfImage* image);
    SymbolInfo* FindSymbolClassicHash(const LookupContext& context, ElfImage* image);
    
    // GNU hash lookup (O(1) average case)
    SymbolInfo* FindSymbolGnuHash(const std::string& name, ElfImage* image);
    SymbolInfo* FindSymbolGnuHash(const LookupContext& context, ElfImage* image);
    
    // Fallback linear search (slow but complete)
    SymbolInfo* FindSymbolLinear(const std::string& name, ElfImage* image);
    
    // Bloom filter test for GNU hash (quick rejection)
    bool TestBloomFilter(uint32_t hash1, uint32_t hash2);
    
    // Symbol validation
    bool IsValidSymbol(const SymbolInfo& info, const LookupContext& context);
    bool IsSymbolVisible(const SymbolInfo& info);
    
    // Cache operations
    SymbolInfo* LookupCache(const std::string& name);
    void CacheSymbol(const std::string& name, const SymbolInfo& info);
    
    // Helper methods
    bool LoadSymbolTable(ElfImage* image);
    bool LoadStringTable(ElfImage* image);
    const char* GetString(uint32_t offset);
    SymbolInfo* GetSymbol(uint32_t index);
    
    // Data members
    ClassicHashTable fClassicHash;
    GnuHashTable fGnuHash;
    SymbolCache fCache;
    PerformanceMetrics fMetrics;
    
    // Symbol table cache
    std::vector<SymbolInfo> fSymbols;
    bool fSymbolsLoaded;
    
    // String table cache
    const char* fStringTable;
    size_t fStringTableSize;
    
    // Current image context
    ElfImage* fCurrentImage;
};

#endif // SYMBOL_RESOLVER_H