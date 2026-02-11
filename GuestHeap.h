/*
 * GuestHeap.h - Guest Memory Heap Management
 * 
 * Provides malloc/free implementation for guest programs running in the VM
 * Uses host memory with proper alignment and bookkeeping
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <vector>
#include <unordered_map>

// Memory block header for tracking allocations
struct MemoryBlock {
    void* ptr;           // Pointer to allocated memory
    size_t size;         // Size of allocation
    bool free;           // Whether block is free
    MemoryBlock* next;    // Next block in list
    MemoryBlock* prev;    // Previous block in list
    
    // Magic number for corruption detection
    static constexpr uint32_t MAGIC = 0xDEADBEEF;
    uint32_t magic;
    
    MemoryBlock(size_t s, void* p) : ptr(p), size(s), free(false), next(nullptr), prev(nullptr), magic(MAGIC) {}
    ~MemoryBlock() = default;
};

class GuestHeap {
public:
    GuestHeap();
    ~GuestHeap();
    
    // Initialize heap with given capacity
    bool Initialize(size_t capacity);
    
    // malloc/free interface
    void* malloc(size_t size);
    void* calloc(size_t count, size_t size);
    void* realloc(void* ptr, size_t new_size);
    void  free(void* ptr);
    
    // Heap information
    size_t GetTotalSize() const { return total_size; }
    size_t GetUsedSize() const { return used_size; }
    size_t GetFreeSize() const { return total_size - used_size; }
    size_t GetAllocationCount() const { return allocation_count; }
    
    // Memory validation
    bool ValidatePointer(const void* ptr) const;
    void DumpHeapInfo() const;
    
private:
    // Memory management
    MemoryBlock* FindFreeBlock(size_t size);
    MemoryBlock* SplitBlock(MemoryBlock* block, size_t size);
    void MergeFreeBlocks();
    void CoalesceWithNext(MemoryBlock* block);
    void CoalesceWithPrevious(MemoryBlock* block);
    
    // Memory block bookkeeping
    std::unordered_map<void*, MemoryBlock*> allocations; // ptr -> block mapping
    MemoryBlock* free_list;                            // Free blocks list
    uint8_t* heap_base;                              // Base of heap memory
    size_t total_size;                               // Total heap capacity
    size_t used_size;                                // Currently used memory
    size_t allocation_count;                          // Number of active allocations
    
    // Thread safety
    mutable std::mutex heap_mutex;
    
    // Constants
    static constexpr size_t ALIGNMENT = 16;        // 16-byte alignment
    static constexpr size_t MIN_BLOCK_SIZE = 32;     // Minimum block size
    static constexpr size_t HEADER_SIZE = sizeof(MemoryBlock);
    
    // Helper methods
    size_t AlignSize(size_t size) const;
    MemoryBlock* GetBlockFromPtr(void* ptr) const;
    void* GetPtrFromBlock(MemoryBlock* block) const;
    bool IsValidBlock(MemoryBlock* block) const;
};