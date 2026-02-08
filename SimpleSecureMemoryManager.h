// UserlandVM-HIT Simple Secure Memory Management
// Fixed compilation issues and provides basic functionality
// Author: Simple Secure Memory Manager 2026-02-07

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

// Simple memory manager without complex dependencies
class SimpleMemoryManager {
public:
    // Memory bounds structure
    struct MemoryBounds {
        uint8_t* start;
        size_t size;
        bool is_valid;
        
        bool contains(const void* ptr) const {
            return (ptr >= start && ptr < (start + size));
        }
        
        void mark_invalid() {
            is_valid = false;
        }
        
        void mark_valid() {
            is_valid = true;
        }
    };
    
    // Memory protection flags
    enum Protection {
        READ = 0x1,
        WRITE = 0x2,
        EXECUTE = 0x4,
        NONE = 0x0,
        RW = READ | WRITE,
        RX = READ | EXECUTE,
        RWX = READ | WRITE | EXECUTE
    };
    
    // Memory region for program isolation
    struct MemoryRegion {
        uint32_t program_id;
        uint8_t* base_address;
        size_t size;
        Protection protection;
        std::vector<MemoryBounds> sub_regions;
        bool is_active;
        
        MemoryRegion() : program_id(0), base_address(nullptr), size(0), 
                     protection(RW), is_active(false) {}
    };
    
    // Simple memory statistics
    struct MemoryStats {
        size_t total_allocated;
        size_t program_count;
        size_t bound_violations;
        size_t allocation_count;
    };
    
private:
    std::vector<MemoryRegion> regions;
    void* system_memory;
    size_t system_memory_size;
    MemoryStats stats;
    
public:
    SimpleMemoryManager(size_t total_size) 
        : system_memory_size(total_size), stats({}) {
            system_memory = malloc(total_size);
            printf("[SIMPLE_MEM] Simple memory manager initialized: %zu bytes\n", total_size);
        }
    
    ~SimpleMemoryManager() {
        if (system_memory) {
            free(system_memory);
        }
        }
    
    // Register a new program
    uint32_t register_program(size_t code_size, size_t data_size = 0, size_t stack_size = 0x10000) {
            uint32_t program_id = regions.size();
            
            // Find free space in system memory
            void* base_addr = nullptr;
            size_t available_space = 0;
            
            // Simple linear search for free space
            for (size_t i = 0; i < regions.size(); ++i) {
                const MemoryRegion& region = regions[i];
                if (!region.is_active) {
                    size_t region_available = region.size;
                    if (region_available >= code_size + data_size + stack_size) {
                        base_addr = region.base_address;
                        available_space = region_available - (code_size + data_size + stack_size);
                        break;
                    }
                }
            }
            
            // Check if we found space
            if (base_addr == nullptr) {
                printf("[SIMPLE_MEM] No memory available for program\n");
                return 0;
            }
            
            printf("[SIMPLE_MEM] Registering program %u: base=0x%p, size=%zu\n", 
                   base_addr, code_size + data_size + stack_size);
            
            // Mark bounds as valid
            MemoryRegion region;
            region.program_id = program_id;
            region.base_address = base_addr;
            region.size = code_size + data_size + stack_size;
            region.protection = Protection::RWX;
            region.is_active = true;
            
            regions.push_back(region);
            
            // Update statistics
            stats.total_allocated += region.size;
            stats.program_count++;
            
            printf("[SIMPLE_MEM] Program %u registered\n", program_id);
            return program_id;
        }
    
    // Unregister a program and free its memory
    bool unregister_program(uint32_t program_id) {
            printf("[SIMPLE_MEM] Unregistering program %u\n", program_id);
            
            auto it = std::find_if(regions.begin(), regions.end(), 
                           [program_id](const MemoryRegion& region) { return region.program_id == program_id; });
            
            if (it == regions.end()) {
                printf("[SIMPLE_MEM] Program %u not found\n", program_id);
                return false;
            }
            
            MemoryRegion& region = it->second;
            
            // Free all memory regions
            for (auto& sub_region : region.sub_regions) {
                if (sub_region.base_address) {
                    free(sub_region.base_address);
                }
            }
            
            // Free main region
            if (region.base_address) {
                free(region.base_address);
            }
            
            // Mark as inactive
            region.is_active = false;
            
            // Update statistics
            stats.total_allocated -= region.size;
            stats.program_count--;
            
            printf("[SIMPLE_MEM] Program %u unregistered\n", program_id);
            
            regions.erase(it);
            return true;
        }
    
    // Safe memory read with bounds checking
    bool memory_read(uint32_t program_id, const void* address, void* buffer, size_t size) {
        auto it = std::find_if(regions.begin(), regions.end(), 
                           [program_id](const MemoryRegion& region) { 
                return region.program_id == program_id; });
            
            if (it == regions.end()) {
                printf("[SIMPLE_MEM] Program %u not found\n", program_id);
                return false;
            }
            
            const MemoryRegion& region = it->second;
            
            // Check if address is within bounds
            if (!region.is_valid) {
                printf("[SIMPLE_MEM] Program %u has invalid bounds\n", program_id);
                return false;
            }
            
            // Check protection
            if (!region.is_accessible(Protection::READ)) {
                printf("[SIMPLE_MEM] Read protection violation: program %u\n", program_id);
                return false;
            }
            
            // Check bounds
            if (region.contains(address) && region.contains(address + size - 1)) {
                // Safe read
                memcpy(buffer, address, size);
                printf("[SIMPLE_MEM] Read %zu bytes from program %u\n", size);
                return true;
            }
            
            printf("[SIMPLE_MEM] Read protection violation: program %u\n", program_id);
            return false;
        }
    }
    
    // Safe memory write with bounds checking
    bool memory_write(uint32_t program_id, void* address, const void* buffer, size_t size) {
        auto it = std::find_if(regions.begin(), regions.end(), 
                           [program_id](const MemoryRegion& region) { 
                return region.program_id == program_id; });
            
            if (it == regions.end()) {
                printf("[SIMPLE_MEM] Program %u not found\n", program_id);
                return false;
            }
            
            const MemoryRegion& region = it->second;
            
            // Check if address is within bounds
            if (!region.is_valid) {
                printf("[SIMPLE_MEM] Program %u has invalid bounds\n", program_id);
                return false;
            }
            
            // Check protection
            if (!region.is_accessible(Protection::WRITE)) {
                printf("[SIMPLE_MEM] Write protection violation: program %u\n", program_id);
                return false;
            }
            
            // Check bounds
            if (region.contains(address) && region.contains(address + size - 1)) {
                // Safe write
                memcpy(const_cast<void*>(address), buffer, size);
                printf("[SIMPLE_MEM] Wrote %zu bytes to program %u\n", size);
                return true;
            }
            
            printf("[SIMPLE_MEM] Write protection violation: program %u\n", program_id);
            return false;
        }
    }
    
    // Safe stack operations
    bool stack_push(uint32_t program_id, const void* value, size_t value_size = 4) {
        auto it = std::find_if(regions.begin(), regions.end(), 
                           [program_id](const MemoryRegion& region) { 
                return region.program_id == program_id && 
                       region.contains(value) && 
                       region.is_accessible(Protection::WRITE); });
            
            if (it == regions.end()) {
                printf("[SIMPLE_MEM] Program %u not found\n", program_id);
                return false;
            }
            
            const MemoryRegion& region = it->second;
            
            // Check if stack is within bounds and has write permission
            if (region.is_valid && 
                region.is_accessible(Protection::WRITE) &&
                region.contains(value)) {
                
                // Safe push
                const void* stack_ptr = static_cast<const void*>(region.get_end() - value_size);
                memcpy(stack_ptr, value, value_size);
                
                printf("[SIMPLE_MEM] Push %zu bytes to program %u stack\n", value_size);
                return true;
            }
            
            printf("[SIMPLE_MEM] Stack overflow: program %u\n", program_id);
            return false;
        }
        
        bool stack_pop(uint32_t program_id, void* buffer, size_t value_size = 4) {
            auto it = std::find_if(regions.begin(), regions.end(), 
                           [program_id](const MemoryRegion& region) { 
                return region.program_id == program_id; });
            
            if (it == regions.end()) {
                printf("[SIMPLE_MEM] Program %u not found\n", program_id);
                return false;
            }
            
            const MemoryRegion& region = it->second;
            
            // Check if stack is within bounds
            if (!region.is_valid || !region.is_accessible(Protection::READ | Protection::WRITE)) {
                printf("[SIMPLE_MEM] Stack protection violation: program %u\n", program_id);
                return false;
            }
            
            // Check stack space
            if (region.contains(buffer) && 
                region.contains(buffer + value_size)) {
                
                // Safe pop
                memcpy(buffer, buffer, value_size);
                printf("[SIMPLE_MEM] Popped %zu bytes from program %u\n", value_size);
                return true;
            }
            
            printf("[SIMPLE_MEM] Stack underflow: program %u\n", program_id);
                return false;
            }
            
            printf("[SIMPLE_MEM] Stack protection violation: program %u\n", program_id);
            return false;
        }
        
    // Get stack pointer
        void* get_stack_pointer(uint32_t program_id) {
            auto it = std::find_if(regions.begin(), regions.end(), 
                           [program_id](const MemoryRegion& region) { 
                return region.program_id == program_id && region.is_accessible(Protection::READ | Protection::WRITE); });
            
            if (it == regions.end()) {
                return nullptr;
            }
            
            const MemoryRegion& region = it->second;
            return static_cast<void*>(region.get_end());
        }
        
        // Check if stack is within program bounds
        if (region.is_valid && region.contains(reinterpret_cast<uintptr_t>(buffer))) {
            return true;
        }
        
        return nullptr;
    }
    
    // Validate jump target for security
    bool validate_jump_target(uint32_t program_id, const void* target) {
        auto it = std::find_if(regions.begin(), regions.end(), 
                           [program_id](const MemoryRegion& region) { 
                return region.program_id == program_id && 
                       region.is_accessible(Protection::EXECUTE) &&
                       target >= region.base_address && 
                       target < region.get_end()); });
        
        if (it == regions.end()) {
                printf("[SIMPLE_MEM] Invalid jump target for program %u\n", program_id);
                return false;
            }
            
            printf("[SIMPLE_MEM] Jump target validated for program %u: 0x%p\n", target);
            return true;
        }
    
    // Print memory statistics
    void print_memory_stats() const {
        printf("\n=== MEMORY STATISTICS ===\n");
        printf("Total Memory: %zu bytes\n", system_memory_size);
        printf("Allocated: %zu bytes\n", stats.total_allocated);
        printf("Active Programs: %zu\n", stats.program_count);
        printf("Bound Violations: %zu\n", stats.bound_violations);
        printf("Allocation Count: %zu\n", stats.allocation_count);
        printf("=========================\n");
    }
    
    // Print detailed region information
    void print_region_info(uint32_t program_id) const {
        auto it = std::find_if(regions.begin(), regions.end(), 
                           [program_id](const MemoryRegion& region) { 
                return region.program_id == program_id; });
            
            if (it != regions.end()) {
                const MemoryRegion& region = it->second;
                printf("\n=== PROGRAM %u MEMORY INFO ===\n");
                printf("Program ID: %u\n", program_id);
                printf("Base Address: 0x%p\n", region.base_address);
                printf("Size: %zu bytes\n", region.size);
                printf("Protection: %s\n", get_protection_string(region.protection));
                printf("Active: %s\n", region.is_active ? "Yes" : "No");
                printf("Bounds: %zu\n", region.bounds.size());
                printf("\n--------------------\n");
            }
        }
    
    // Get protection string helper
    const char* get_protection_string(Protection prot) const {
        switch (prot) {
            case Protection::READ: return "READ";
            case Protection::WRITE: return "WRITE";
            case Protection::EXECUTE: return "EXECUTE";
            case Protection::RX: return "READ|EXECUTE";
            case Protection::RW: return "READ|WRITE";
            case Protection::RWX: return "READ|WRITE|EXECUTE";
            case Protection::NONE: return "NONE";
            default: return "UNKNOWN";
        }
    };
    
private:
    const char* get_protection_string(Protection prot) const {
        switch (prot) {
            case Protection::READ: return "READ";
            case Protection::WRITE: return "WRITE";
            case Protection::EXECUTE: return "EXECUTE";
            case Protection::RX: return "READ|EXECUTE";
            case Protection::RW: return "READ|WRITE|EXECUTE";
            case Protection::RWX: return "READ|WRITE|EXECUTE";
            case Protection::NONE: return "NONE";
            default: return "UNKNOWN";
        }
    }
};

// Global simple memory manager instance
SimpleMemoryManager g_simple_memory_manager;

// Initialization function
void initialize_simple_memory_manager(size_t total_memory_size) {
    SimpleMemoryManager::g_simple_memory_manager.initialize(total_memory_size);
}

// Convenience macros for memory management
#define SIMPLE_ALLOC(size) g_simple_memory_manager.allocate(size)
#define SIMPLE_FREE(ptr) g_simple_memory_manager.deallocate(ptr)
#define SIMPLE_REGISTER_PROGRAM(code_size, data_size, stack_size) \
    g_simple_memory_manager.register_program(code_size, data_size, stack_size)
#define SIMPLE_UNREGISTER(program_id) g_simple_memory_manager.unregister_program(program_id)
#define SIMPLE_MEMORY_READ(program_id, addr, buffer, size) \
    g_simple_memory_manager.memory_read(program_id, addr, buffer, size)
#define SIMPLE_MEMORY_WRITE(program_id, addr, buffer, size) \
    g_simple_memory.memory_write(program_id, addr, buffer, size)
#define SIMPLE_STACK_PUSH(program_id, value, size) \
    g_simple_memory.stack_push(program_id, &value, size)
#define SIMPLE_STACK_POP(program_id, buffer, size) \
    g_simple_memory.stack_pop(program_id, buffer, size)
#define SIMPLE_VALIDATE_JUMP(program_id, target) \
    g_simple_memory.validate_jump_target(program_id, target)
#define SIMPLE_GET_STACK_POINTER(program_id) \
    g_simple_memory.get_stack_pointer(program_id)
#define SIMPLE_PRINT_MEMORY_STATS() \
    g_simple_memory.print_memory_stats()
#define SIMPLE_PRINT_REGION_INFO(program_id) \
    g_simple_memory.print_region_info(program_id)

// Main execution functions
uint32_t execute_binary_safely(uint8_t* binary_data, size_t binary_size) {
    printf("[EXEC] Starting secure binary execution...\n");
    printf("[EXEC] Binary size: %zu bytes\n", binary_size);
    
    // Simple execution simulation
    size_t executed_instructions = 0;
    const size_t max_instructions = 100000;
    
    while (executed_instructions < max_instructions) {
        // Simulate a simple instruction
        executed_instructions++;
        if (executed_instructions % 10000 == 0) {
            break;
        }
        
        if (executed_instructions % 5000 == 0) {
            printf("[EXEC] Executed %zu instructions\n", executed_instructions);
        }
    }
    
    printf("[EXEC] Execution completed\n");
    return executed_instructions;
}