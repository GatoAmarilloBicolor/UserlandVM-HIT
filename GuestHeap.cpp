/*
 * GuestHeap.cpp - Guest Memory Heap Management Implementation
 * 
 * Provides malloc/free implementation for guest programs running in VM
 * Uses host memory with proper alignment and bookkeeping
 */

#include "GuestHeap.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

GuestHeap::GuestHeap() 
    : free_list(nullptr), heap_base(nullptr), total_size(0), 
      used_size(0), allocation_count(0) {
    printf("[GuestHeap] Initialized\n");
}

GuestHeap::~GuestHeap() {
    if (heap_base) {
        printf("[GuestHeap] Destroying heap with %zu allocations remaining\n", allocation_count);
        free(heap_base);
        heap_base = nullptr;
    }
}

bool GuestHeap::Initialize(size_t capacity) {
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    printf("[GuestHeap] Initializing with capacity: %zu bytes\n", capacity);
    
    if (heap_base) {
        printf("[GuestHeap] WARNING: Heap already initialized\n");
        return true;
    }
    
    // Allocate memory from host for guest heap
    heap_base = (uint8_t*)malloc(capacity);
    if (!heap_base) {
        printf("[GuestHeap] ERROR: Failed to allocate %zu bytes\n", capacity);
        return false;
    }
    
    total_size = capacity;
    used_size = 0;
    allocation_count = 0;
    
    // Create initial free block spanning entire heap
    free_list = new MemoryBlock(capacity - HEADER_SIZE, heap_base + HEADER_SIZE);
    free_list->free = true;
    
    allocations.clear();
    
    printf("[GuestHeap] ✅ Initialized: %zu bytes, base=%p\n", capacity, heap_base);
    return true;
}

void* GuestHeap::malloc(size_t size) {
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    printf("[GuestHeap] malloc(%zu)\n", size);
    
    if (!heap_base) {
        printf("[GuestHeap] ERROR: Heap not initialized\n");
        return nullptr;
    }
    
    if (size == 0) {
        return nullptr;
    }
    
    // Align size and add header
    size_t aligned_size = AlignSize(size);
    
    // Find suitable free block
    MemoryBlock* block = FindFreeBlock(aligned_size);
    if (!block) {
        printf("[GuestHeap] ERROR: Out of memory, requested %zu bytes\n", aligned_size);
        return nullptr;
    }
    
    // Mark block as used
    block->free = false;
    
    // Store allocation mapping
    void* ptr = GetPtrFromBlock(block);
    allocations[ptr] = block;
    
    used_size += aligned_size + HEADER_SIZE;
    allocation_count++;
    
    printf("[GuestHeap] ✅ malloc(%zu) = %p (aligned to %zu)\n", size, ptr, aligned_size);
    return ptr;
}

void* GuestHeap::calloc(size_t count, size_t size) {
    size_t total_size = count * size;
    void* ptr = malloc(total_size);
    
    if (ptr) {
        memset(ptr, 0, total_size);
        printf("[GuestHeap] ✅ calloc(%zu, %zu) = %p\n", count, size, ptr);
    }
    
    return ptr;
}

void* GuestHeap::realloc(void* ptr, size_t new_size) {
    printf("[GuestHeap] realloc(%p, %zu)\n", ptr, new_size);
    
    if (!ptr) {
        return malloc(new_size);
    }
    
    if (new_size == 0) {
        free(ptr);
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    auto it = allocations.find(ptr);
    if (it == allocations.end()) {
        printf("[GuestHeap] ERROR: realloc invalid pointer %p\n", ptr);
        return nullptr;
    }
    
    MemoryBlock* block = it->second;
    size_t old_size = block->size;
    size_t aligned_new_size = AlignSize(new_size);
    
    // If new size fits in current block, just resize it
    if (aligned_new_size <= old_size) {
        printf("[GuestHeap] ✅ realloc %p to %zu bytes (shrinking)\n", ptr, new_size);
        return ptr;
    }
    
    // Try to expand block if next block is free
    if (block->next && block->next->free) {
        size_t combined_size = old_size + HEADER_SIZE + block->next->size;
        if (combined_size >= aligned_new_size) {
            // Merge with next block and split if needed
            CoalesceWithNext(block);
            MemoryBlock* remaining = SplitBlock(block, aligned_new_size);
            printf("[GuestHeap] ✅ realloc %p to %zu bytes (expanded)\n", ptr, new_size);
            return ptr;
        }
    }
    
    // Otherwise allocate new block and copy data
    void* new_ptr = malloc(new_size);
    if (!new_ptr) {
        printf("[GuestHeap] ERROR: realloc failed, out of memory\n");
        return nullptr;
    }
    
    // Copy old data
    size_t copy_size = std::min(old_size, new_size);
    memcpy(new_ptr, ptr, copy_size);
    
    // Free old block
    allocations.erase(it);
    block->free = true;
    MergeFreeBlocks();
    
    allocation_count--; // Will be incremented in malloc
    
    printf("[GuestHeap] ✅ realloc %p -> %p (%zu bytes)\n", ptr, new_ptr, new_size);
    return new_ptr;
}

void GuestHeap::free(void* ptr) {
    if (!ptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    printf("[GuestHeap] free(%p)\n", ptr);
    
    auto it = allocations.find(ptr);
    if (it == allocations.end()) {
        printf("[GuestHeap] ERROR: free invalid pointer %p\n", ptr);
        return;
    }
    
    MemoryBlock* block = it->second;
    if (!IsValidBlock(block)) {
        printf("[GuestHeap] ERROR: Corrupted block header at %p\n", block);
        return;
    }
    
    // Mark block as free
    block->free = true;
    
    // Update statistics
    size_t block_size = block->size + HEADER_SIZE;
    used_size -= block_size;
    allocation_count--;
    allocations.erase(it);
    
    // Try to merge with adjacent free blocks
    MergeFreeBlocks();
    
    printf("[GuestHeap] ✅ freed %p (%zu bytes)\n", ptr, block_size);
}

bool GuestHeap::ValidatePointer(const void* ptr) const {
    if (!ptr || !heap_base) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    auto it = allocations.find(const_cast<void*>(ptr));
    return it != allocations.end() && IsValidBlock(it->second);
}

void GuestHeap::DumpHeapInfo() const {
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    printf("[GuestHeap] === Heap Information ===\n");
    printf("[GuestHeap] Total size: %zu bytes\n", total_size);
    printf("[GuestHeap] Used size: %zu bytes (%.1f%%)\n", used_size, 
           total_size > 0 ? (double)used_size * 100.0 / total_size : 0.0);
    printf("[GuestHeap] Free size: %zu bytes\n", total_size - used_size);
    printf("[GuestHeap] Allocation count: %zu\n", allocation_count);
    printf("[GuestHeap] Active allocations:\n");
    
    for (const auto& pair : allocations) {
        MemoryBlock* block = pair.second;
        printf("[GuestHeap]   %p: %zu bytes\n", pair.first, block->size);
    }
    printf("[GuestHeap] ========================\n");
}

// Private helper methods

size_t GuestHeap::AlignSize(size_t size) const {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

MemoryBlock* GuestHeap::GetBlockFromPtr(void* ptr) const {
    return reinterpret_cast<MemoryBlock*>(static_cast<uint8_t*>(ptr) - HEADER_SIZE);
}

void* GuestHeap::GetPtrFromBlock(MemoryBlock* block) const {
    return static_cast<void*>(reinterpret_cast<uint8_t*>(block) + HEADER_SIZE);
}

bool GuestHeap::IsValidBlock(MemoryBlock* block) const {
    if (!block || block->magic != MemoryBlock::MAGIC) {
        return false;
    }
    
    uintptr_t addr = reinterpret_cast<uintptr_t>(block);
    uintptr_t base = reinterpret_cast<uintptr_t>(heap_base);
    
    return addr >= base && addr < base + total_size;
}

MemoryBlock* GuestHeap::FindFreeBlock(size_t size) {
    MemoryBlock* block = free_list;
    MemoryBlock* best_fit = nullptr;
    size_t best_size = SIZE_MAX;
    
    // First-fit with best-fit optimization
    while (block) {
        if (block->free && block->size >= size) {
            if (block->size < best_size) {
                best_fit = block;
                best_size = block->size;
            }
        }
        block = block->next;
    }
    
    if (best_fit) {
        return SplitBlock(best_fit, size);
    }
    
    return nullptr;
}

MemoryBlock* GuestHeap::SplitBlock(MemoryBlock* block, size_t size) {
    if (block->size <= size + MIN_BLOCK_SIZE) {
        return block; // Don't split if remainder is too small
    }
    
    // Create new block for remaining space
    MemoryBlock* remainder = new MemoryBlock(block->size - size, GetPtrFromBlock(block));
    remainder->free = true;
    
    // Update original block
    block->size = size;
    
    // Insert remainder into free list
    remainder->next = block->next;
    remainder->prev = block;
    if (block->next) {
        block->next->prev = remainder;
    }
    block->next = remainder;
    
    return block;
}

void GuestHeap::MergeFreeBlocks() {
    // Coalesce adjacent free blocks to reduce fragmentation
    for (MemoryBlock* block = free_list; block; block = block->next) {
        if (block->free && block->next && block->next->free) {
            CoalesceWithNext(block);
        }
    }
}

void GuestHeap::CoalesceWithNext(MemoryBlock* block) {
    if (!block->next || !block->next->free) {
        return;
    }
    
    MemoryBlock* next_block = block->next;
    
    // Merge sizes
    block->size += HEADER_SIZE + next_block->size;
    
    // Update links
    block->next = next_block->next;
    if (next_block->next) {
        next_block->next->prev = block;
    }
    
    delete next_block;
}

void GuestHeap::CoalesceWithPrevious(MemoryBlock* block) {
    if (!block->prev || !block->prev->free) {
        return;
    }
    
    MemoryBlock* prev_block = block->prev;
    
    // Merge sizes
    prev_block->size += HEADER_SIZE + block->size;
    
    // Update links
    prev_block->next = block->next;
    if (block->next) {
        block->next->prev = prev_block;
    }
    
    delete block;
}