/**
 * @file EnhancedDirectAddressSpace.h
 * @brief Enhanced Address Space with 4GB support and proper ET_DYN relocation
 */

#pragma once

#include "UnifiedDefinitionsCorrected.h"
#include "AddressSpace.h"
#include <cstdint>
#include <vector>
#include <map>
#include <string>

// Enhanced memory management for 4GB address space
class EnhancedDirectAddressSpace : public AddressSpace {
public:
    // 4GB address space for x86-32
    static const size_t GUEST_MEMORY_SIZE_4GB = 0x100000000ULL; // 4GB
    static const uint32_t STANDARD_CODE_BASE = 0x08048000;     // Traditional executable base
    static const uint32_t ET_DYN_BASE = 0x08000000;            // ET_DYN (PIE) base
    static const uint32_t HEAP_BASE = 0x40000000;              // Heap base
    static const uint32_t STACK_BASE = 0xC0000000;            // Stack base (grows down)
    static const size_t STACK_SIZE = 0x10000000;              // 256MB stack
    
    // Memory region types
    enum MemoryType {
        MEMORY_TYPE_CODE = 1,
        MEMORY_TYPE_DATA = 2,
        MEMORY_TYPE_HEAP = 3,
        MEMORY_TYPE_STACK = 4,
        MEMORY_TYPE_MMAP = 5,
        MEMORY_TYPE_SHARED = 6
    };
    
    struct MemoryRegion {
        uint32_t start;
        uint32_t end;
        uint32_t size;
        MemoryType type;
        uint32_t protection;  // Read/Write/Execute flags
        uint8_t* host_ptr;
        std::string name;
    };
    
    EnhancedDirectAddressSpace();
    virtual ~EnhancedDirectAddressSpace();
    
    // Initialize with 4GB address space
    virtual status_t Init(size_t size = GUEST_MEMORY_SIZE_4GB);
    
    // Enhanced memory management with proper 4GB support
    virtual status_t Read(uintptr_t guestAddress, void* buffer, size_t size) override;
    virtual status_t Write(uintptr_t guestAddress, const void* buffer, size_t size) override;
    virtual status_t ReadString(uintptr_t guestAddress, char* buffer, size_t bufferSize) override;
    
    // Enhanced virtual address mapping for ET_DYN binaries
    virtual status_t RegisterMapping(uintptr_t guest_vaddr, uintptr_t guest_offset, 
                                   size_t size, MemoryType type = MEMORY_TYPE_CODE,
                                   const std::string& name = "anonymous");
    virtual uintptr_t TranslateAddress(uintptr_t guest_vaddr) const override;
    
    // ET_DYN specific functions
    virtual status_t LoadETDynBinary(const void* binary_data, size_t binary_size, 
                                    uint32_t* load_base, uint32_t* entry_point);
    virtual status_t ApplyRelocations(uint32_t load_base, const void* relocations, 
                                    size_t rel_count);
    
    // Enhanced heap management
    virtual status_t AllocateHeap(uint32_t* heap_base, size_t initial_size);
    virtual status_t ExpandHeap(size_t additional_size);
    
    // Stack management
    virtual status_t AllocateStack(uint32_t* stack_base, size_t stack_size = STACK_SIZE);
    
    // Memory protection
    virtual status_t ProtectMemory(uintptr_t address, size_t size, uint32_t protection);
    virtual status_t CheckMemoryAccess(uintptr_t address, size_t size, bool is_write) const;
    
    // Memory debugging
    virtual void DumpMemoryMap();
    virtual void DumpMemoryRegion(uint32_t address);
    virtual bool IsValidAddress(uintptr_t address) const;
    
    // Direct memory access (for performance)
    virtual uint8_t* GetHostPointer(uintptr_t guest_address);
    virtual uintptr_t GetGuestBase() const { return fGuestBaseAddress; }
    virtual size_t GetGuestSize() const { return fGuestSize; }
    
private:
    uint8_t* fMemory;                    // 4GB memory backing
    size_t fGuestSize;                   // Total guest memory size
    uintptr_t fGuestBaseAddress;         // Base address in host memory
    
    // Memory management
    std::vector<MemoryRegion> fRegions;   // Memory regions
    std::map<uint32_t, size_t> fAddressToRegion; // Address -> region index mapping
    
    // Heap management
    uint32_t fHeapBase;
    uint32_t fHeapSize;
    uint32_t fHeapNext;
    
    // Stack management  
    uint32_t fStackBase;
    uint32_t fStackSize;
    
    // ET_DYN specific
    uint32_t fLoadBias;
    bool fETDynLoaded;
    
    // Utility functions
    status_t AllocateHostMemory(size_t size);
    void FreeHostMemory();
    MemoryRegion* FindRegion(uintptr_t address);
    const MemoryRegion* FindRegion(uintptr_t address) const;
    status_t AddRegion(uint32_t start, uint32_t size, MemoryType type, 
                      const std::string& name = "anonymous");
    void RemoveRegion(uint32_t start);
    
    // Memory protection constants - now using definitions from UnifiedDefinitionsCorrected.h
};