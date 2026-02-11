/*
 * EnhancedHeap.h - Advanced memory management with performance optimizations
 * Reduces fragmentation and improves allocation patterns
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>

class EnhancedHeap {
public:
    // Memory allocation with size class optimization
    void* Allocate(size_t size, size_t alignment = 16);
    void Deallocate(void* ptr);
    void* Reallocate(void* ptr, size_t new_size);
    
    // Memory statistics and debugging
    size_t GetTotalAllocated() const { return total_allocated.load(); }
    size_t GetPeakAllocated() const { return peak_allocated.load(); }
    size_t GetFragmentationCount() const { return fragmentation_count.load(); }
    
    // Heap validation and corruption detection
    bool ValidateHeap() const;
    void DumpHeapStats() const;
    
    // Memory sanitization
    static void* SanitizeMemory(void* ptr, size_t size);
    static bool CheckCorruption(void* ptr, size_t size);
    
    // Performance tuning
    void SetCompactThreshold(size_t threshold) { compact_threshold = threshold; }
    void EnableSanitization(bool enabled) { sanitization_enabled = enabled; }
    
private:
    struct MemoryBlock {
        size_t size;
        size_t actual_size;
        uint32_t magic;
        uint32_t checksum;
        bool is_free;
        MemoryBlock* prev;
        MemoryBlock* next;
        size_t allocation_id;
    };
    
    // Size class optimization
    struct SizeClass {
        size_t size;
        std::vector<MemoryBlock*> free_blocks;
    };
    
    // Allocation methods
    MemoryBlock* FindBestFit(size_t size);
    MemoryBlock* AllocateFromSizeClass(size_t size);
    void MergeAdjacentBlocks(MemoryBlock* block);
    void CompactHeap();
    
    // Corruption detection
    uint32_t ComputeChecksum(const MemoryBlock* block) const;
    bool IsValidBlock(const MemoryBlock* block) const;
    
    std::vector<SizeClass> size_classes;
    MemoryBlock* large_blocks;
    
    std::atomic<size_t> total_allocated{0};
    std::atomic<size_t> peak_allocated{0};
    std::atomic<size_t> fragmentation_count{0};
    std::atomic<uint32_t> allocation_counter{0};
    
    size_t compact_threshold = 64 * 1024;  // 64KB
    bool sanitization_enabled = false;
    
    mutable std::mutex heap_mutex;
    uint8_t* heap_base;
    size_t heap_size;
    
    static constexpr uint32_t BLOCK_MAGIC = 0xDEADBEEF;
    static constexpr uint32_t CORRUPTED_MAGIC = 0xBADC0FFE;
    static constexpr size_t ALIGNMENT = 16;
    static constexpr size_t MIN_BLOCK_SIZE = 64;
    static constexpr size_t SANITIZATION_PATTERN = 0xCD;
};

// Global enhanced heap instance
extern EnhancedHeap* g_enhanced_heap;

// Enhanced memory macros with corruption detection
#define ENHANCED_MALLOC(size) g_enhanced_heap->Allocate(size)
#define ENHANCED_FREE(ptr) g_enhanced_heap->Deallocate(ptr)
#define ENHANCED_REALLOC(ptr, size) g_enhanced_heap->Reallocate(ptr, size)
