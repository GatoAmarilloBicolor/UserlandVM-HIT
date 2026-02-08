// UserlandVM-HIT Secure Memory Management and Program Isolation
// Bounds checking and memory sandboxing for safe multi-program execution
// Author: Secure Memory Management 2026-02-07

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <SupportDefs.h>

namespace SecureMemoryManagement {
    
    // Memory protection flags
    enum MemoryProtection {
        PROT_READ    = 0x1,
        PROT_WRITE   = 0x2,
        PROT_EXEC    = 0x4,
        PROT_NONE     = 0x0,
        PROT_RW       = PROT_READ | PROT_WRITE,
        PROT_RX       = PROT_READ | PROT_EXEC,
        PROT_RWX      = PROT_READ | PROT_WRITE | PROT_EXEC
    };
    
    // Memory region types
    enum MemoryType {
        HEAP_MEMORY,
        STACK_MEMORY,
        CODE_MEMORY,
        DATA_MEMORY,
        MAPPED_MEMORY,
        GUARD_MEMORY
    };
    
    // Memory bounds information
    struct MemoryBounds {
        uint8_t* start_address;
        size_t size;
        bool is_valid;
        MemoryType type;
        MemoryProtection protection;
        
        MemoryBounds() : start_address(nullptr), size(0), is_valid(false), 
                       type(HEAP_MEMORY), protection(PROT_NONE) {}
        
        bool contains(const void* ptr) const {
            const uint8_t* addr = static_cast<const uint8_t*>(ptr);
            return (addr >= start_address && addr < start_address + size);
        }
        
        bool is_accessible(MemoryProtection required_prot) const {
            if (!is_valid) return false;
            return (protection & required_prot) == required_prot;
        }
        
        void* get_end() const {
            return start_address + size;
        }
    };
    
    // Memory region for program isolation
    struct MemoryRegion {
        uint32_t program_id;
        void* base_address;
        size_t size;
        std::vector<MemoryBounds> bounds;
        bool is_active;
        
        MemoryRegion() : program_id(0), base_address(nullptr), size(0), is_active(false) {}
        
        void* get_relative_address(size_t offset) const {
            if (!base_address || offset >= size) {
                return nullptr;
            }
            return static_cast<uint8_t*>(base_address) + offset;
        }
        
        bool is_offset_valid(size_t offset) const {
            return offset < size;
        }
    };
    
    // Secure memory manager with sandboxing
    class SecureMemoryManager {
    private:
        static constexpr size_t GUARD_SIZE = 4096;           // 4KB guard pages
        static constexpr size_t MIN_REGION_SIZE = 65536;         // 64KB minimum region size
        static constexpr size_t MAX_REGIONS = 1024;               // Maximum concurrent programs
        static constexpr uint32_t INVALID_PROGRAM_ID = 0xFFFFFFFF;
        
        std::unordered_map<uint32_t, MemoryRegion> regions;
        std::vector<MemoryBounds> guard_pages;
        void* system_memory_pool;
        size_t system_memory_size;
        uint32_t next_program_id;
        std::mutex manager_mutex;
        
        // Track memory statistics
        struct MemoryStats {
            size_t total_allocated;
            size_t total_guard_pages;
            size_t active_regions;
            size_t total_protected_memory;
            size_t guard_violations;
            size_t bound_violations;
        } stats;
        
        // Find if an address is in any region
        bool is_address_in_region(uint32_t program_id, const void* address) const {
            auto it = regions.find(program_id);
            if (it == regions.end()) {
                return false;
            }
            
            const MemoryRegion& region = it->second;
            if (!region.is_active) {
                return false;
            }
            
            return (address >= region.base_address && 
                    address < static_cast<uint8_t*>(region.base_address) + region.size);
        }
        
        // Get memory bounds for an address
        MemoryBounds get_bounds_for_address(uint32_t program_id, const void* address) const {
            auto it = regions.find(program_id);
            if (it == regions.end()) {
                return MemoryBounds();
            }
            
            const MemoryRegion& region = it->second;
            for (const auto& bound : region.bounds) {
                if (bound.contains(address)) {
                    return bound;
                }
            }
            
            return MemoryBounds();
        }
        
    public:
        SecureMemoryManager(size_t total_size) 
            : system_memory_size(total_size), next_program_id(1), manager_mutex() {
            stats = {};
            
            // Allocate system memory pool
            system_memory_pool = malloc(total_size);
            if (!system_memory_pool) {
                printf("[SECURE_MEM] Failed to allocate system memory pool: %zu bytes\n", total_size);
                return;
            }
            
            printf("[SECURE_MEM] Initialized secure memory manager: %zu bytes\n", total_size);
            stats.total_allocated = total_size;
        }
        
        ~SecureMemoryManager() {
            if (system_memory_pool) {
                free(system_memory_pool);
            }
        }
        
        // Register a new program with memory isolation
        uint32_t register_program(size_t code_size, size_t data_size, size_t stack_size = 0x100000) {
            std::lock_guard<std::mutex> lock(manager_mutex);
            
            printf("[SECURE_MEM] Registering new program: code=%zu, data=%zu, stack=%zu\n", 
                   code_size, data_size, stack_size);
            
            uint32_t program_id = next_program_id++;
            
            // Calculate total required size
            size_t total_size = code_size + data_size + stack_size;
            size_t aligned_size = (total_size + 4095) & ~4095; // Align to 4KB
            
            // Find free memory space
            size_t used_memory = 0;
            for (const auto& region : regions) {
                if (region.is_active) {
                    used_memory += region.size;
                }
            }
            
            if (used_memory + aligned_size > system_memory_size) {
                printf("[SECURE_MEM] Insufficient memory for program registration\n");
                return INVALID_PROGRAM_ID;
            }
            
            // Allocate memory for the program
            uint8_t* program_memory = static_cast<uint8_t*>(system_memory_pool) + used_memory;
            
            MemoryRegion region;
            region.program_id = program_id;
            region.base_address = program_memory;
            region.size = aligned_size;
            region.is_active = true;
            
            // Create memory bounds for different regions
            size_t code_offset = 0;
            
            // Code region (read+execute, no write)
            MemoryBounds code_bound;
            code_bound.start_address = program_memory + code_offset;
            code_bound.size = code_size;
            code_bound.type = CODE_MEMORY;
            code_bound.protection = PROT_RX;
            code_bound.is_valid = true;
            region.bounds.push_back(code_bound);
            code_offset += code_size;
            
            // Data region (read+write, no execute)
            MemoryBounds data_bound;
            data_bound.start_address = program_memory + code_offset;
            data_bound.size = data_size;
            data_bound.type = DATA_MEMORY;
            data_bound.protection = PROT_RW;
            data_bound.is_valid = true;
            region.bounds.push_back(data_bound);
            code_offset += data_size;
            
            // Stack region (read+write, no execute, grows down)
            MemoryBounds stack_bound;
            stack_bound.start_address = program_memory + aligned_size - stack_size;
            stack_bound.size = stack_size;
            stack_bound.type = STACK_MEMORY;
            stack_bound.protection = PROT_RW;
            stack_bound.is_valid = true;
            region.bounds.push_back(stack_bound);
            
            // Create guard pages (no access)
            size_t guard_count = 0;
            uint8_t* current_pos = program_memory + aligned_size;
            
            // Guard before region
            if (guard_count < guard_pages.size()) {
                MemoryBounds& guard = guard_pages[guard_count++];
                if (guard.is_valid) {
                    memset(guard.start_address, 0xCC, GUARD_SIZE);
                    current_pos += GUARD_SIZE;
                }
            }
            
            // Guard after region
            for (size_t i = 0; i < 4 && guard_count < guard_pages.size(); i++) {
                MemoryBounds& guard = guard_pages[guard_count++];
                if (guard.is_valid) {
                    memset(guard.start_address, 0xCC, GUARD_SIZE);
                    current_pos += GUARD_SIZE;
                }
            }
            
            regions[program_id] = region;
            stats.total_allocated += aligned_size;
            stats.active_regions++;
            stats.total_protected_memory += total_size;
            
            printf("[SECURE_MEM] Program %u registered: base=0x%p, size=0x%zx\n", 
                   program_id, region.base_address, (uint64_t)aligned_size);
            
            return program_id;
        }
        
        // Unregister a program and free its memory
        bool unregister_program(uint32_t program_id) {
            std::lock_guard<std::mutex> lock(manager_mutex);
            
            printf("[SECURE_MEM] Unregistering program %u\n", program_id);
            
            auto it = regions.find(program_id);
            if (it == regions.end()) {
                printf("[SECURE_MEM] Program %u not found\n", program_id);
                return false;
            }
            
            MemoryRegion& region = it->second;
            
            if (!region.is_active) {
                printf("[SECURE_MEM] Program %u already inactive\n", program_id);
                return true;
            }
            
            // Clear guard pages
            for (MemoryBounds& bound : region.bounds) {
                if (bound.is_valid) {
                    memset(bound.start_address, 0x00, bound.size);
                }
            }
            
            // Clear the memory region
            if (region.base_address) {
                memset(region.base_address, 0x00, region.size);
            }
            
            region.is_active = false;
            stats.active_regions--;
            stats.total_allocated -= region.size;
            stats.total_protected_memory -= region.size;
            
            return true;
        }
        
        // Get memory access with bounds checking
        bool memory_read(uint32_t program_id, const void* address, void* buffer, size_t size) {
            MemoryBounds bounds = get_bounds_for_address(program_id, address);
            
            if (!bounds.is_valid) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Read violation: program=%u, addr=0x%p, size=%zu\n", 
                       program_id, address, size);
                return false;
            }
            
            if (!bounds.is_accessible(PROT_READ)) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Read protection violation: program=%u, addr=0x%p\n", program_id, address);
                return false;
            }
            
            // Perform bounds checking
            if (bounds.start_address + size > bounds.get_end()) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Read overflow: program=%u, addr=0x%p, size=%zu\n", 
                       program_id, address, size);
                return false;
            }
            
            memcpy(buffer, address, size);
            return true;
        }
        
        bool memory_write(uint32_t program_id, const void* address, const void* buffer, size_t size) {
            MemoryBounds bounds = get_bounds_for_address(program_id, address);
            
            if (!bounds.is_valid) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Write violation: program=%u, addr=0x%p, size=%zu\n", 
                       program_id, address, size);
                return false;
            }
            
            if (!bounds.is_accessible(PROT_WRITE)) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Write protection violation: program=%u, addr=0x%p\n", program_id, address);
                return false;
            }
            
            // Perform bounds checking
            if (bounds.start_address + size > bounds.get_end()) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Write overflow: program=%u, addr=0x%p, size=%zu\n", 
                       program_id, address, size);
                return false;
            }
            
            memcpy(const_cast<void*>(address), buffer, size);
            return true;
        }
        
        // Safe stack operations with bounds checking
        bool stack_push(uint32_t program_id, void** stack_ptr, const void* value, size_t value_size) {
            MemoryBounds bounds = get_bounds_for_address(program_id, *stack_ptr);
            
            if (!bounds.is_valid || bounds.type != STACK_MEMORY) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Stack operation on invalid region: program=%u\n", program_id);
                return false;
            }
            
            if (!bounds.is_accessible(PROT_WRITE)) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Stack write protection violation: program=%u\n", program_id);
                return false;
            }
            
            // Check stack space
            if (static_cast<uint8_t*>(*stack_ptr) - value_size < bounds.start_address) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Stack underflow: program=%u, sp=0x%p, size=%zu\n", 
                       program_id, *stack_ptr, value_size);
                return false;
            }
            
            // Perform safe push
            if (static_cast<uint8_t*>(*stack_ptr) - value_size >= bounds.start_address + bounds.size) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Stack overflow: program=%u, sp=0x%p, size=%zu\n", 
                       program_id, *stack_ptr, value_size);
                return false;
            }
            
            memcpy(*stack_ptr - value_size, value, value_size);
            *stack_ptr = static_cast<uint8_t*>(*stack_ptr) - value_size;
            
            return true;
        }
        
        bool stack_pop(uint32_t program_id, void** stack_ptr, void* buffer, size_t value_size) {
            MemoryBounds bounds = get_bounds_for_address(program_id, *stack_ptr);
            
            if (!bounds.is_valid || bounds.type != STACK_MEMORY) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Stack operation on invalid region: program=%u\n", program_id);
                return false;
            }
            
            if (!bounds.is_accessible(PROT_READ | PROT_WRITE)) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Stack read/write protection violation: program=%u\n", program_id);
                return false;
            }
            
            // Check stack bounds
            if (*stack_ptr + value_size > bounds.get_end()) {
                stats.bound_violations++;
                printf("[SECURE_MEM] Stack overflow during pop: program=%u, sp=0x%p, size=%zu\n", 
                       program_id, *stack_ptr, value_size);
                return false;
            }
            
            // Perform safe pop
            memcpy(buffer, *stack_ptr, value_size);
            *stack_ptr = static_cast<uint8_t*>(*stack_ptr) + value_size;
            
            return true;
        }
        
        // Get stack pointer with bounds checking
        void** get_stack_pointer(uint32_t program_id, const void* base_address, size_t stack_offset = 0) {
            auto it = regions.find(program_id);
            if (it == regions.end()) {
                return nullptr;
            }
            
            const MemoryRegion& region = it->second;
            if (!region.is_active) {
                return nullptr;
            }
            
            // Find stack region
            for (const auto& bound : region.bounds) {
                if (bound.type == STACK_MEMORY) {
                    void** stack_ptr = static_cast<void**>(const_cast<void*>(bound.start_address) + stack_offset);
                    return stack_ptr;
                }
            }
            
            return nullptr;
        }
        
        // Check jump target for security
        bool validate_jump_target(uint32_t program_id, const void* target) {
            MemoryBounds bounds = get_bounds_for_address(program_id, target);
            
            if (!bounds.is_valid) {
                printf("[SECURE_MEM] Jump to invalid address: program=%u, target=0x%p\n", program_id, target);
                return false;
            }
            
            if (!bounds.is_accessible(PROT_EXEC)) {
                printf("[SECURE_MEM] Jump to non-executable address: program=%u, target=0x%p\n", program_id, target);
                return false;
            }
            
            printf("[SECURE_MEM] Jump target validated: program=%u, target=0x%p\n", program_id, target);
            return true;
        }
        
        // Print memory statistics
        void print_statistics() const {
            printf("\n=== SECURE MEMORY STATISTICS ===\n");
            printf("Total System Memory: %zu bytes\n", system_memory_size);
            printf("Total Allocated: %zu bytes\n", stats.total_allocated);
            printf("Active Programs: %zu\n", stats.active_regions);
            printf("Protected Memory: %zu bytes\n", stats.total_protected_memory);
            printf("Guard Pages: %zu\n", stats.total_guard_pages);
            printf("Bound Violations: %zu\n", stats.bound_violations);
            printf("Guard Violations: %zu\n", stats.guard_violations);
            printf("Memory Utilization: %.2f%%\n", 
                   system_memory_size > 0 ? (double)stats.total_allocated / system_memory_size * 100.0 : 0.0);
            printf("=============================\n\n");
        }
        
        // Get region information for debugging
        void print_region_info(uint32_t program_id) const {
            auto it = regions.find(program_id);
            if (it == regions.end()) {
                printf("[SECURE_MEM] Program %u not found\n", program_id);
                return;
            }
            
            const MemoryRegion& region = it->second;
            printf("\n=== PROGRAM %u MEMORY INFO ===\n", program_id);
            printf("Base Address: 0x%p\n", region.base_address);
            printf("Size: 0x%zx bytes\n", (uint64_t)region.size);
            printf("Active: %s\n", region.is_active ? "Yes" : "No");
            printf("Memory Bounds: %zu regions\n", region.bounds.size());
            
            for (size_t i = 0; i < region.bounds.size(); i++) {
                const MemoryBounds& bound = region.bounds[i];
                printf("  [%zu] %s: 0x%p-0x%p (0x%zx bytes)\n", 
                       i, bound.type == CODE_MEMORY ? "CODE" :
                       bound.type == DATA_MEMORY ? "DATA" :
                       bound.type == STACK_MEMORY ? "STACK" : "UNKNOWN",
                       bound.start_address, bound.get_end(), (uint64_t)bound.size);
                printf("       Protection: %s\n", 
                       bound.protection == PROT_RX ? "RX" :
                       bound.protection == PROT_RW ? "RW" :
                       bound.protection == PROT_RWX ? "RWX" : "NONE");
            }
            printf("==========================\n\n");
        }
    };
    
    // Global secure memory manager
    static SecureMemoryManager* instance;
    
public:
    static void initialize(size_t total_memory_size) {
        if (instance) {
            delete instance;
        }
        instance = new SecureMemoryManager(total_memory_size);
    }
    
    static SecureMemoryManager& get_instance() {
        if (!instance) {
            // Initialize with default size if not already done
            static size_t default_size = 256 * 1024 * 1024; // 256MB default
            initialize(default_size);
        }
        return *instance;
    }
    
    // Deinitialize
    static void deinitialize() {
        if (instance) {
            delete instance;
            instance = nullptr;
        }
    }
};

// Initialize secure memory manager
SecureMemoryManagement::SecureMemoryManager* SecureMemoryManagement::instance = nullptr;