/*
 * OptimizedStringPool.h - Efficient string memory management
 * Reduces allocations and improves string comparison performance
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <unordered_set>

class OptimizedStringPool {
public:
    struct StringEntry {
        uint32_t hash;
        uint32_t offset;    // Offset in pool buffer
        uint16_t length;
        uint16_t ref_count;
    };
    
    OptimizedStringPool(size_t initial_capacity = 4096);
    ~OptimizedStringPool();
    
    // Add string to pool, returns interned pointer
    const char* Intern(const char* str);
    
    // Fast string comparison using pre-computed hashes
    bool Equals(const char* str1, const char* str2) const;
    
    // Memory usage statistics
    size_t GetTotalAllocated() const { return total_allocated; }
    size_t GetUniqueStrings() const { return string_entries.size(); }
    
private:
    uint32_t ComputeHash(const char* str) const;
    const char* FindInterned(const char* str) const;
    
    std::vector<StringEntry> string_entries;
    std::vector<char> pool_buffer;
    std::unordered_set<uint32_t> hash_set;
    
    size_t total_allocated;
    uint32_t next_offset;
};

// Global string pool instance
extern OptimizedStringPool* g_string_pool;

// Optimized string operations
#define STRING_INTERN(str) (g_string_pool ? g_string_pool->Intern(str) : (str))
#define STRING_EQUALS(s1, s2) (g_string_pool ? g_string_pool->Equals(s1, s2) : (strcmp(s1, s2) == 0))
