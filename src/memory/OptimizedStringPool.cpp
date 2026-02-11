/*
 * OptimizedStringPool.cpp - Implementation
 */

#include "OptimizedStringPool.h"
#include <cstdio>

OptimizedStringPool* g_string_pool = nullptr;

OptimizedStringPool::OptimizedStringPool(size_t initial_capacity) 
    : total_allocated(0), next_offset(0) {
    pool_buffer.reserve(initial_capacity);
    string_entries.reserve(256);  // Pre-allocate for common strings
}

OptimizedStringPool::~OptimizedStringPool() {
    printf("[STRING_POOL] Final stats: %zu unique strings, %zu bytes allocated\n",
           GetUniqueStrings(), GetTotalAllocated());
}

const char* OptimizedStringPool::Intern(const char* str) {
    if (!str) return nullptr;
    
    // Check if already interned
    const char* existing = FindInterned(str);
    if (existing) {
        return existing;
    }
    
    uint16_t length = strlen(str);
    uint32_t hash = ComputeHash(str);
    
    // Check hash collision
    if (hash_set.find(hash) != hash_set.end()) {
        // Linear search for collision (rare)
        for (const auto& entry : string_entries) {
            if (entry.hash == hash && 
                entry.length == length &&
                strncmp(pool_buffer.data() + entry.offset, str, length) == 0) {
                return pool_buffer.data() + entry.offset;
            }
        }
    }
    
    // Add new string
    StringEntry entry;
    entry.hash = hash;
    entry.offset = next_offset;
    entry.length = length;
    entry.ref_count = 1;
    
    string_entries.push_back(entry);
    hash_set.insert(hash);
    
    // Copy string to pool
    pool_buffer.resize(next_offset + length + 1);  // +1 for null terminator
    memcpy(pool_buffer.data() + next_offset, str, length);
    pool_buffer[next_offset + length] = '\0';
    
    const char* result = pool_buffer.data() + next_offset;
    next_offset += length + 1;
    total_allocated += length + 1;
    
    return result;
}

bool OptimizedStringPool::Equals(const char* str1, const char* str2) const {
    if (str1 == str2) return true;
    if (!str1 || !str2) return false;
    
    // Fast path: compare hashes first
    uint32_t hash1 = ComputeHash(str1);
    uint32_t hash2 = ComputeHash(str2);
    
    if (hash1 != hash2) return false;
    
    // Full comparison only if hashes match
    return strcmp(str1, str2) == 0;
}

uint32_t OptimizedStringPool::ComputeHash(const char* str) const {
    // FNV-1a hash - fast and good distribution
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint32_t)(*str++);
        hash *= 16777619u;
    }
    return hash;
}

const char* OptimizedStringPool::FindInterned(const char* str) const {
    uint32_t hash = ComputeHash(str);
    uint16_t length = strlen(str);
    
    for (const auto& entry : string_entries) {
        if (entry.hash == hash && 
            entry.length == length &&
            strncmp(pool_buffer.data() + entry.offset, str, length) == 0) {
            return pool_buffer.data() + entry.offset;
        }
    }
    
    return nullptr;
}
