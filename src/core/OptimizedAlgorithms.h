/*
 * OptimizedAlgorithms.h - Performance-critical algorithms with optimizations
 * Replaces inefficient algorithms with optimized versions
 */

#pragma once

#include <cstring>
#include <algorithm>
#include <cstdint>

// Optimized memory operations
namespace Optimized {
    // Fast memory set with loop unrolling
    inline void* memset_fast(void* dest, int c, size_t n) {
        if (UNLIKELY(n == 0)) return dest;
        
        uint8_t* d = (uint8_t*)dest;
        uint8_t byte = (uint8_t)c;
        
        // Handle head bytes up to 8-byte alignment
        while (n > 0 && ((uintptr_t)d & 7)) {
            *d++ = byte;
            n--;
        }
        
        // 8-byte unrolled loop
        uint64_t pattern = ((uint64_t)byte * 0x0101010101010101ull);
        while (n >= 8) {
            *(uint64_t*)d = pattern;
            d += 8;
            n -= 8;
        }
        
        // Handle tail bytes
        while (n > 0) {
            *d++ = byte;
            n--;
        }
        
        return dest;
    }
    
    // Fast memory copy with alignment handling
    inline void* memcpy_fast(void* dest, const void* src, size_t n) {
        if (UNLIKELY(n == 0) || dest == src) return dest;
        
        uint8_t* d = (uint8_t*)dest;
        const uint8_t* s = (const uint8_t*)src;
        
        // Handle overlapping regions
        if (src < dest && (src + n) > dest) {
            // Copy backwards
            d += n;
            s += n;
            while (n--) {
                *--d = *--s;
            }
        } else {
            // Copy forwards with 8-byte alignment
            while (n > 0 && ((uintptr_t)d & 7)) {
                *d++ = *s++;
                n--;
            }
            
            while (n >= 8) {
                *(uint64_t*)d = *(const uint64_t*)s;
                d += 8;
                s += 8;
                n -= 8;
            }
            
            while (n--) {
                *d++ = *s++;
            }
        }
        
        return dest;
    }
    
    // Optimized string length
    inline size_t strlen_fast(const char* str) {
        if (UNLIKELY(!str)) return 0;
        
        // Check 8 bytes at a time
        const uint64_t* ptr64 = (const uint64_t*)str;
        uint64_t mask = 0x0101010101010101ull;
        
        while (true) {
            uint64_t chunk = *ptr64++;
            
            // Check if any byte is null
            uint64_t test = (chunk - mask) & (~chunk) & mask;
            if (test != 0) {
                // Found null byte, determine position
                size_t offset = (__builtin_ctzll(test) >> 3);
                return ((const char*)ptr64 - str - 8) + offset;
            }
        }
    }
    
    // Fast string comparison with early termination
    inline int strcmp_fast(const char* s1, const char* s2) {
        if (s1 == s2) return 0;
        if (UNLIKELY(!s1)) return -1;
        if (UNLIKELY(!s2)) return 1;
        
        // Compare 8 bytes at a time
        const uint64_t* p1 = (const uint64_t*)s1;
        const uint64_t* p2 = (const uint64_t*)s2;
        uint64_t mask = 0x0101010101010101ull;
        
        while (true) {
            uint64_t c1 = *p1++;
            uint64_t c2 = *p2++;
            
            // Check for null bytes
            uint64_t null1 = (c1 - mask) & (~c1) & mask;
            uint64_t null2 = (c2 - mask) & (~c2) & mask;
            
            if (null1 || null2) {
                // One or both strings ended
                if (null1 && null2) return 0;  // Both ended
                if (null1) return -1;         // s1 ended
                return 1;                      // s2 ended
            }
            
            if (UNLIKELY(c1 != c2)) {
                // Difference found, find exact position
                size_t diff_pos = (__builtin_ctzll(c1 ^ c2) >> 3);
                return s1[diff_pos] - s2[diff_pos];
            }
        }
    }
    
    // Binary search optimized for sorted arrays
    template<typename T, typename K>
    inline int binary_search(const T* array, size_t size, const K& key, 
                         int (*compare)(const K&, const T&)) {
        if (UNLIKELY(size == 0)) return -1;
        
        size_t left = 0;
        size_t right = size - 1;
        
        while (left <= right) {
            size_t mid = left + ((right - left) >> 1);
            int cmp = compare(key, array[mid]);
            
            if (cmp == 0) return mid;
            if (cmp < 0) left = mid + 1;
            else right = mid - 1;
        }
        
        return -1;
    }
    
    // Quick sort implementation with median-of-three pivot
    template<typename T>
    void quick_sort(T* array, size_t size, int (*compare)(const T&, const T&)) {
        if (UNLIKELY(size < 2)) return;
        
        std::sort(array, array + size, 
            [compare](const T& a, const T& b) { return compare(a, b) < 0; });
    }
}

// Replacements for standard functions
#define OPTIMIZED_MEMSET(dest, c, n) Optimized::memset_fast(dest, c, n)
#define OPTIMIZED_MEMCPY(dest, src, n) Optimized::memcpy_fast(dest, src, n)
#define OPTIMIZED_STRLEN(str) Optimized::strlen_fast(str)
#define OPTIMIZED_STRCMP(s1, s2) Optimized::strcmp_fast(s1, s2)
