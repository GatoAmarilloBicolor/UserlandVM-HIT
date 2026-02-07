// UserlandVM-HIT Memory Protection Implementation
// Enhanced memory protection enforcement for guest memory
// Author: Memory Protection Implementation 2026-02-07

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>

// Memory protection namespace for enhanced security
namespace MemoryProtection {
    
    // Protection flags definition
    enum ProtectionFlags {
        PROT_NONE = 0x0,
        PROT_READ = 0x1,
        PROT_WRITE = 0x2,
        PROT_EXEC = 0x4,
        PROT_RWX = PROT_READ | PROT_WRITE | PROT_EXEC
    };
    
    // Memory region structure for protection tracking
    struct MemoryRegion {
        uintptr_t start_addr;
        uintptr_t end_addr;
        uint32_t protection;
        bool is_mmaped;
        bool is_stack;
        bool is_heap;
        bool is_code;
        
        MemoryRegion() : start_addr(0), end_addr(0), protection(PROT_NONE), 
                      is_mmaped(false), is_stack(false), is_heap(false), is_code(false) {}
        
        bool Contains(uintptr_t addr) const {
            return addr >= start_addr && addr < end_addr;
        }
        
        bool Overlaps(uintptr_t addr, size_t size) const {
            uintptr_t addr_end = addr + size;
            return !(addr_end <= start_addr || addr >= end_addr);
        }
        
        bool HasPermission(uint32_t required_prot) const {
            return (protection & required_prot) == required_prot;
        }
        
        const char* GetTypeName() const {
            if (is_stack) return "STACK";
            if (is_heap) return "HEAP";
            if (is_code) return "CODE";
            if (is_mmaped) return "MMAPED";
            return "UNKNOWN";
        }
    };
    
    // Memory protection manager
    class MemoryProtectionManager {
    private:
        std::unordered_map<uintptr_t, MemoryRegion> memory_regions;
        std::unordered_map<uintptr_t, uint32_t> mprot_cache;
        
        // Helper function to find containing region
        typename std::unordered_map<uintptr_t, MemoryRegion>::iterator FindRegion(uintptr_t addr) {
            for (auto it = memory_regions.begin(); it != memory_regions.end(); ++it) {
                if (it->second.Contains(addr)) {
                    return it;
                }
            }
            return memory_regions.end();
        }
        
    public:
        // Register memory region with protection
        bool RegisterRegion(uintptr_t start_addr, size_t size, uint32_t protection, 
                         const char* type_name = "UNKNOWN") {
            printf("[MEMORY_PROT] Registering region: 0x%lx-0x%lx, prot=0x%x, type=%s\n",
                   start_addr, start_addr + size, protection, type_name);
            
            MemoryRegion region;
            region.start_addr = start_addr;
            region.end_addr = start_addr + size;
            region.protection = protection;
            region.is_mmaped = (strcmp(type_name, "MMAPED") == 0);
            region.is_stack = (strcmp(type_name, "STACK") == 0);
            region.is_heap = (strcmp(type_name, "HEAP") == 0);
            region.is_code = (strcmp(type_name, "CODE") == 0);
            
            memory_regions[start_addr] = region;
            printf("[MEMORY_PROT] Region registered: %s\n", region.GetTypeName());
            return true;
        }
        
        // Check if memory access is allowed
        bool CheckAccess(uintptr_t addr, size_t size, uint32_t required_prot, 
                      const char* operation_name = "unknown") {
            // Check protection cache first (fast path)
            auto cache_it = mprot_cache.find(addr);
            if (cache_it != mprot_cache.end()) {
                if ((cache_it->second & required_prot) == required_prot) {
                    return true;
                }
            }
            
            // Find the memory region
            auto it = FindRegion(addr);
            
            // If region not found, it's a violation
            if (it == memory_regions.end()) {
                printf("[MEMORY_PROT] VIOLATION: Address 0x%lx not in any mapped region\n", addr);
                return false;
            }
            
            const MemoryRegion& region = it->second;
            
            // Check if entire access range fits within region
            if (!region.Contains(addr) || !region.Contains(addr + size - 1)) {
                printf("[MEMORY_PROT] ERROR: Access range 0x%lx-0x%lx exceeds region bounds\n",
                       addr, addr + size);
                return false;
            }
            
            // Check protection permissions
            if (!region.HasPermission(required_prot)) {
                printf("[MEMORY_PROT] VIOLATION: %s access (0x%x) not allowed for region %s (0x%x)\n",
                       operation_name, required_prot, region.GetTypeName(), region.protection);
                printf("[MEMORY_PROT] Region has: READ=%s, WRITE=%s, EXEC=%s\n",
                       (region.protection & PROT_READ) ? "YES" : "NO",
                       (region.protection & PROT_WRITE) ? "YES" : "NO",
                       (region.protection & PROT_EXEC) ? "YES" : "NO");
                return false;
            }
            
            // Cache the successful permission check
            mprot_cache[addr] = region.protection;
            
            printf("[MEMORY_PROT] ACCESS GRANTED: %s to %s region at 0x%lx\n",
                   operation_name, region.GetTypeName(), addr);
            return true;
        }
        
        // Update memory protection (mprotect-like)
        bool UpdateProtection(uintptr_t addr, size_t size, uint32_t new_prot) {
            printf("[MEMORY_PROT] Updating protection: 0x%lx-0x%lx, new_prot=0x%x\n",
                   addr, addr + size, new_prot);
            
            auto it = memory_regions.lower_bound(addr);
            if (it == memory_regions.end() || !it->second.Contains(addr)) {
                printf("[MEMORY_PROT] ERROR: Cannot update protection for unmapped region\n");
                return false;
            }
            
        // Update region protection
        if (it != memory_regions.end()) {
            it->second.protection = new_prot;
        }
            
            // Clear cache for this address range
            for (uintptr_t clear_addr = addr; clear_addr < addr + size; clear_addr += 4096) {
                mprot_cache.erase(clear_addr);
            }
            
            printf("[MEMORY_PROT] Protection updated successfully\n");
            return true;
        }
        
        // Unregister memory region
        bool UnregisterRegion(uintptr_t addr) {
            printf("[MEMORY_PROT] Unregistering region at 0x%lx\n", addr);
            
            auto it = FindRegion(addr);
            if (it != memory_regions.end()) {
                printf("[MEMORY_PROT] Region %s unregistered\n", it->second.GetTypeName());
                memory_regions.erase(it);
                
                // Clear cache
                for (uintptr_t clear_addr = it->second.start_addr; 
                     clear_addr < it->second.end_addr; clear_addr += 4096) {
                    mprot_cache.erase(clear_addr);
                }
                
                return true;
            }
            
            printf("[MEMORY_PROT] ERROR: Region at 0x%lx not found\n", addr);
            return false;
        }
        
        // Print protection status
        void PrintStatus() const {
            printf("[MEMORY_PROT] Memory Protection Status:\n");
            printf("  Total regions: %zu\n", memory_regions.size());
            printf("  Cache entries: %zu\n", mprot_cache.size());
            
            int code_regions = 0, stack_regions = 0, heap_regions = 0, mmap_regions = 0;
            for (const auto& pair : memory_regions) {
                const MemoryRegion& region = pair.second;
                if (region.is_code) code_regions++;
                if (region.is_stack) stack_regions++;
                if (region.is_heap) heap_regions++;
                if (region.is_mmaped) mmap_regions++;
            }
            
            printf("  Code regions: %d\n", code_regions);
            printf("  Stack regions: %d\n", stack_regions);
            printf("  Heap regions: %d\n", heap_regions);
            printf("  Mmaped regions: %d\n", mmap_regions);
        }
        
        // Initialize memory protection system
        void Initialize() {
            printf("[MEMORY_PROT] Initializing memory protection system...\n");
            
            // Register default memory layout regions
            RegisterRegion(0x08000000, 0x78000000, PROT_RWX, "CODE");   // Standard executable
            RegisterRegion(0xC0000000, 0x40000000, PROT_RW, "STACK");  // Stack
            RegisterRegion(0x80000000, 0x40000000, PROT_RW, "LIBRARY"); // Libraries
            
            printf("[MEMORY_PROT] Memory protection system initialized\n");
        }
    };
    
    // Global memory protection manager instance
    static MemoryProtectionManager g_protection_manager;
    
    // Convenience functions for common protection checks
    inline bool CheckReadAccess(uintptr_t addr, size_t size) {
        return g_protection_manager.CheckAccess(addr, size, PROT_READ, "READ");
    }
    
    inline bool CheckWriteAccess(uintptr_t addr, size_t size) {
        return g_protection_manager.CheckAccess(addr, size, PROT_WRITE, "WRITE");
    }
    
    inline bool CheckExecuteAccess(uintptr_t addr, size_t size) {
        return g_protection_manager.CheckAccess(addr, size, PROT_EXEC, "EXECUTE");
    }
    
    inline bool CheckReadWriteAccess(uintptr_t addr, size_t size) {
        return g_protection_manager.CheckAccess(addr, size, PROT_READ | PROT_WRITE, "READ_WRITE");
    }
    
    // Memory protection operations
    inline bool ProtectMemory(uintptr_t addr, size_t size, uint32_t protection) {
        return g_protection_manager.UpdateProtection(addr, size, protection);
    }
    
    inline void InitializeProtection() {
        g_protection_manager.Initialize();
    }
    
    inline void PrintProtectionStatus() {
        g_protection_manager.PrintStatus();
    }
}

// Apply memory protection globally
void ApplyMemoryProtection() {
    printf("[GLOBAL_MEMORY_PROT] Applying enhanced memory protection...\n");
    
    MemoryProtection::InitializeProtection();
    MemoryProtection::PrintProtectionStatus();
    
    printf("[GLOBAL_MEMORY_PROT] Memory protection system ready!\n");
    printf("[GLOBAL_MEMORY_PROT] UserlandVM-HIT now has enhanced memory security!\n");
}