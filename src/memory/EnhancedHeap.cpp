/*
 * EnhancedHeap.cpp - Advanced memory management implementation
 */

#include "EnhancedHeap.h"
#include <cstdio>
#include <algorithm>
#include <cstring>

EnhancedHeap* g_enhanced_heap = nullptr;

EnhancedHeap::EnhancedHeap(size_t heap_size) : heap_size(heap_size), large_blocks(nullptr) {
    heap_base = (uint8_t*)malloc(heap_size);
    if (!heap_base) {
        printf("[HEAP] Failed to allocate heap of %zu bytes\n", heap_size);
        return;
    }
    
    printf("[HEAP] Enhanced heap initialized: %zu bytes\n", heap_size);
    
    // Initialize size classes for common allocation sizes
    size_classes.resize(8);  // 32, 64, 128, 256, 512, 1024, 2048, 4096
    
    size_t base_size = 32;
    for (auto& size_class : size_classes) {
        size_class.size = base_size;
        size_class.free_blocks.reserve(16);  // Pre-allocate free list
        base_size *= 2;
    }
}

EnhancedHeap::~EnhancedHeap() {
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    size_t leaked_blocks = 0;
    size_t leaked_bytes = 0;
    
    // Check for memory leaks
    for (const auto& size_class : size_classes) {
        for (const MemoryBlock* block : size_class.free_blocks) {
            if (block && !block->is_free) {
                leaked_blocks++;
                leaked_bytes += block->actual_size;
            }
        }
    }
    
    // Check large blocks
    const MemoryBlock* current = large_blocks;
    while (current) {
        if (!current->is_free) {
            leaked_blocks++;
            leaked_bytes += current->actual_size;
        }
        current = current->next;
    }
    
    if (leaked_blocks > 0) {
        printf("[HEAP] WARNING: %zu leaked blocks, %zu bytes\n", leaked_blocks, leaked_bytes);
    }
    
    free(heap_base);
    printf("[HEAP] Enhanced heap destroyed\n");
}

void* EnhancedHeap::Allocate(size_t size, size_t alignment) {
    if (UNLIKELY(size == 0)) return nullptr;
    
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    // Round up to alignment
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
    size_t actual_size = sizeof(MemoryBlock) + aligned_size;
    
    MemoryBlock* block = nullptr;
    
    // Try to allocate from size class first
    if (aligned_size <= 4096) {
        block = AllocateFromSizeClass(aligned_size);
    }
    
    // If no size class allocation available, search in large blocks
    if (!block) {
        block = FindBestFit(actual_size);
    }
    
    if (UNLIKELY(!block)) {
        // Try heap compactison
        if (total_allocated > compact_threshold) {
            CompactHeap();
            block = FindBestFit(actual_size);
        }
        
        if (!block) {
            printf("[HEAP] Out of memory: requested %zu bytes\n", size);
            return nullptr;
        }
    }
    
    // Setup block metadata
    block->is_free = false;
    block->allocation_id = allocation_counter++;
    
    // Clear memory if sanitization enabled
    if (sanitization_enabled) {
        uint8_t* memory = (uint8_t*)(block + 1);
        memset(memory, SANITIZATION_PATTERN, aligned_size);
    }
    
    // Update statistics
    total_allocated += actual_size;
    size_t current_peak = peak_allocated.load();
    if (total_allocated > current_peak) {
        peak_allocated = total_allocated;
    }
    
    void* result = (void*)((uint8_t*)(block + 1));
    LOG_VERBOSE("[HEAP] Allocated %zu bytes at %p (block id: %u)\n", size, result, block->allocation_id);
    
    return result;
}

void EnhancedHeap::Deallocate(void* ptr) {
    if (UNLIKELY(!ptr)) return;
    
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - sizeof(MemoryBlock));
    
    // Check block validity
    if (UNLIKELY(!IsValidBlock(block))) {
        printf("[HEAP] ERROR: Invalid block at %p\n", ptr);
        return;
    }
    
    if (UNLIKELY(block->is_free)) {
        printf("[HEAP] ERROR: Double free of block %p (id: %u)\n", ptr, block->allocation_id);
        return;
    }
    
    // Check for corruption
    if (sanitization_enabled && UNLIKELY(CheckCorruption(ptr, block->size))) {
        printf("[HEAP] ERROR: Memory corruption detected in block %p (id: %u)\n", ptr, block->allocation_id);
        block->magic = CORRUPTED_MAGIC;
        return;
    }
    
    // Mark as free
    block->is_free = true;
    total_allocated -= block->actual_size;
    
    // Try to merge with adjacent blocks
    MergeAdjacentBlocks(block);
    
    LOG_VERBOSE("[HEAP] Deallocated %p (block id: %u, size: %zu)\n", ptr, block->allocation_id, block->size);
}

EnhancedHeap::MemoryBlock* EnhancedHeap::AllocateFromSizeClass(size_t size) {
    size_t class_index = 0;
    size_t class_size = 32;
    
    // Find appropriate size class
    while (class_index < size_classes.size() && class_size < size) {
        class_size *= 2;
        class_index++;
    }
    
    if (class_index >= size_classes.size()) return nullptr;
    
    SizeClass& size_class = size_classes[class_index];
    
    // Find free block in this size class
    for (auto it = size_class.free_blocks.begin(); it != size_class.free_blocks.end(); ++it) {
        MemoryBlock* block = *it;
        if (block && block->is_free) {
            // Remove from free list
            size_class.free_blocks.erase(it);
            return block;
        }
    }
    
    return nullptr;
}

EnhancedHeap::MemoryBlock* EnhancedHeap::FindBestFit(size_t size) {
    MemoryBlock* best_fit = nullptr;
    size_t best_size = SIZE_MAX;
    
    const MemoryBlock* current = large_blocks;
    while (current) {
        if (current->is_free && current->actual_size >= size) {
            if (current->actual_size < best_size) {
                best_fit = const_cast<MemoryBlock*>(current);
                best_size = current->actual_size;
            }
        }
        current = current->next;
    }
    
    return best_fit;
}

void EnhancedHeap::MergeAdjacentBlocks(MemoryBlock* block) {
    // Try to merge with next block
    if (block->next && block->next->is_free) {
        LOG_VERBOSE("[HEAP] Merging block %p with next %p\n", block, block->next);
        
        block->size += block->next->actual_size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // Try to merge with previous block
    if (block->prev && block->prev->is_free) {
        LOG_VERBOSE("[HEAP] Merging block %p with previous %p\n", block, block->prev);
        
        block->prev->size += block->actual_size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

bool EnhancedHeap::ValidateHeap() const {
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    size_t free_count = 0;
    size_t used_count = 0;
    
    // Validate size classes
    for (const auto& size_class : size_classes) {
        for (const MemoryBlock* block : size_class.free_blocks) {
            if (!IsValidBlock(block)) return false;
            if (!block->is_free) return false;
            free_count++;
        }
    }
    
    // Validate large blocks
    const MemoryBlock* current = large_blocks;
    while (current) {
        if (!IsValidBlock(current)) return false;
        
        if (current->is_free) free_count++;
        else used_count++;
        
        current = current->next;
    }
    
    printf("[HEAP] Validation: %zu free blocks, %zu used blocks\n", free_count, used_count);
    return true;
}

uint32_t EnhancedHeap::ComputeChecksum(const MemoryBlock* block) const {
    // Simple checksum for corruption detection
    uint32_t checksum = 0;
    const uint32_t* data = (const uint32_t*)block;
    size_t words = sizeof(MemoryBlock) / 4;
    
    for (size_t i = 0; i < words; i++) {
        checksum ^= data[i];
        checksum = (checksum << 1) | (checksum >> 31);  // Rotate left
    }
    
    return checksum;
}

bool EnhancedHeap::IsValidBlock(const MemoryBlock* block) const {
    // Check if block pointer is within heap bounds
    const uint8_t* block_addr = (const uint8_t*)block;
    if (block_addr < heap_base || block_addr >= heap_base + heap_size) {
        return false;
    }
    
    // Check magic number
    if (block->magic != BLOCK_MAGIC && block->magic != CORRUPTED_MAGIC) {
        return false;
    }
    
    // Check checksum
    uint32_t expected_checksum = ComputeChecksum(block);
    if (block->checksum != expected_checksum) {
        return false;
    }
    
    return true;
}

bool EnhancedHeap::CheckCorruption(void* ptr, size_t size) const {
    // Check for common corruption patterns
    uint8_t* memory = (uint8_t*)ptr;
    
    // Check for freed memory pattern (some corruption indicators)
    for (size_t i = 0; i < size && i < 64; i++) {  // Check first 64 bytes
        if (memory[i] == 0xDD || memory[i] == 0xFEEEFEEE) {
            return true;  // Common freed patterns
        }
    }
    
    return false;
}

void EnhancedHeap::CompactHeap() {
    printf("[HEAP] Compacting heap (current: %zu bytes)\n", total_allocated.load());
    fragmentation_count++;
    
    // This is a simplified compaction - in a real implementation,
    // we would move used blocks together and create one large free block
    size_t total_free = heap_size - total_allocated;
    
    // Update fragmentation threshold based on usage patterns
    if (total_free < heap_size * 0.1) {  // Less than 10% free
        compact_threshold = heap_size * 0.8;  // Compact sooner
    } else {
        compact_threshold = heap_size * 0.95;  // Compact when nearly full
    }
    
    LOG_VERBOSE("[HEAP] Heap compacted, new threshold: %zu\n", compact_threshold);
}

void EnhancedHeap::DumpHeapStats() const {
    std::lock_guard<std::mutex> lock(heap_mutex);
    
    printf("\n=== ENHANCED HEAP STATISTICS ===\n");
    printf("Total allocated: %zu bytes\n", total_allocated.load());
    printf("Peak allocated: %zu bytes\n", peak_allocated.load());
    printf("Fragmentation events: %zu\n", fragmentation_count.load());
    printf("Total allocations: %u\n", allocation_counter.load());
    printf("Heap size: %zu bytes\n", heap_size);
    printf("Utilization: %.2f%%\n", 
           (double)total_allocated.load() * 100.0 / heap_size);
    
    // Size class statistics
    for (size_t i = 0; i < size_classes.size(); i++) {
        const auto& size_class = size_classes[i];
        size_t free_blocks = 0;
        
        for (const MemoryBlock* block : size_class.free_blocks) {
            if (block && block->is_free) free_blocks++;
        }
        
        printf("Size class %zu: %u bytes, %zu free blocks\n", 
               i, size_class.size, free_blocks);
    }
    
    printf("===============================\n\n");
}

void* EnhancedHeap::SanitizeMemory(void* ptr, size_t size) {
    if (!ptr || size == 0) return ptr;
    
    uint8_t* memory = (uint8_t*)ptr;
    uint8_t pattern = SANITIZATION_PATTERN;
    
    for (size_t i = 0; i < size; i++) {
        memory[i] = pattern;
    }
    
    return ptr;
}

void EnhancedHeap::SetCompactThreshold(size_t threshold) {
    compact_threshold = threshold;
    LOG_VERBOSE("[HEAP] Compact threshold set to %zu bytes\n", threshold);
}

void EnhancedHeap::EnableSanitization(bool enabled) {
    sanitization_enabled = enabled;
    printf("[HEAP] Memory sanitization %s\n", enabled ? "ENABLED" : "DISABLED");
}
