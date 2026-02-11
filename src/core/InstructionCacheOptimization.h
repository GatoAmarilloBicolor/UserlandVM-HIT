/*
 * InstructionCacheOptimization.h - Advanced instruction cache optimizations
 * Improves interpreter performance with better caching strategies
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <array>

class InstructionCacheOptimization {
public:
    struct CacheEntry {
        uint32_t address;
        uint32_t opcode;
        uint32_t arg1;        // Pre-decoded argument 1
        uint32_t arg2;        // Pre-decoded argument 2
        uint16_t instruction_len; // Instruction length
        uint16_t flags;         // Optimization flags
        uint32_t jump_target;    // For jump instructions
        uint32_t access_count;   // Access frequency
    };
    
    // Optimization flags
    static constexpr uint16_t FLAG_IS_JUMP = 0x0001;
    static constexpr uint16_t FLAG_IS_CALL = 0x0002;
    static constexpr uint16_t FLAG_IS_RET = 0x0004;
    static constexpr uint16_t FLAG_IS_MOV = 0x0008;
    static constexpr uint16_t FLAG_IS_ADD = 0x0010;
    static constexpr uint16_t FLAG_HOT_PATH = 0x0020;
    static constexpr uint32_t INVALID_TARGET = 0xFFFFFFFF;
    
    InstructionCacheOptimization();
    
    // Cache operations
    CacheEntry* Get(uint32_t address);
    void Put(uint32_t address, uint32_t opcode, uint32_t arg1 = 0, uint32_t arg2 = 0);
    void Invalidate(uint32_t address);
    void InvalidateRange(uint32_t start, uint32_t end);
    
    // Cache statistics
    uint32_t GetHitCount() const { return hit_count; }
    uint32_t GetMissCount() const { return miss_count; }
    double GetHitRate() const { 
        return (hit_count + miss_count) > 0 ? 
               (double)hit_count / (hit_count + miss_count) * 100.0 : 0.0;
    }
    
    // Performance tuning
    void SetHotPathThreshold(uint32_t threshold) { hot_path_threshold = threshold; }
    void EnablePrediction(bool enabled) { prediction_enabled = enabled; }
    
    // Advanced caching
    uint32_t PredictNextInstruction(uint32_t current_address);
    void RecordJumpTarget(uint32_t from_addr, uint32_t to_addr);
    bool IsInHotPath(uint32_t address);
    
    // Cache management
    void Flush() {
        std::fill(cache_entries.begin(), cache_entries.end(), CacheEntry{});
        hit_count = miss_count = 0;
    }
    
    void DumpStats() const;
    
private:
    static constexpr size_t CACHE_SIZE = 512;  // Increased from 256
    static constexpr uint32_t CACHE_INDEX_MASK = CACHE_SIZE - 1;
    
    std::array<CacheEntry, CACHE_SIZE> cache_entries;
    std::array<uint32_t, CACHE_SIZE> jump_targets;  // Jump prediction table
    
    uint32_t hit_count = 0;
    uint32_t miss_count = 0;
    uint32_t hot_path_threshold = 10;  // Accesses to consider hot
    bool prediction_enabled = true;
    
    // Hashing functions
    uint32_t HashAddress(uint32_t address) const;
    uint32_t JumpHash(uint32_t from_addr, uint32_t to_addr) const;
    
    // Cache replacement policy
    uint32_t GetReplacementIndex(uint32_t index);
    void UpdateAccessPattern(CacheEntry& entry);
};

// Global cache instance
extern InstructionCacheOptimization* g_instruction_cache;

// Cache optimization macros
#define CACHE_GET(addr) (g_instruction_cache ? g_instruction_cache->Get(addr) : nullptr)
#define CACHE_PUT(addr, opcode, arg1, arg2) \
    do { if (g_instruction_cache) g_instruction_cache->Put(addr, opcode, arg1, arg2); } while(0)

#define CACHE_PREDICT_NEXT(addr) \
    (g_instruction_cache ? g_instruction_cache->PredictNextInstruction(addr) : 0)

#define CACHE_IS_HOT(addr) \
    (g_instruction_cache && g_instruction_cache->IsInHotPath(addr))

#define CACHE_HIT_RATE() \
    (g_instruction_cache ? g_instruction_cache->GetHitRate() : 0.0)
