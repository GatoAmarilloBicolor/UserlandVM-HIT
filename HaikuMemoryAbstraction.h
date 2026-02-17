/*
 * HaikuMemoryAbstraction.h - Memory abstraction layer for Haiku
 * Provides memory management interface for the VM
 */

#ifndef HAIKU_MEMORY_ABSTRACTION_H
#define HAIKU_MEMORY_ABSTRACTION_H

#include <cstdint>
#include <cstddef>
#include <memory>

namespace HaikuVM {

class MemoryRegion {
public:
    MemoryRegion(uintptr_t base, size_t size, bool writable = true, bool executable = false)
        : base_address(base), size(size), writable(writable), executable(executable) {}
    
    uintptr_t Base() const { return base_address; }
    size_t Size() const { return size; }
    bool IsWritable() const { return writable; }
    bool IsExecutable() const { return executable; }
    
private:
    uintptr_t base_address;
    size_t size;
    bool writable;
    bool executable;
};

class HaikuMemoryAbstraction {
public:
    HaikuMemoryAbstraction();
    ~HaikuMemoryAbstraction();
    
    bool Initialize(size_t total_memory);
    
    void* Allocate(size_t size, uintptr_t* out_address);
    bool Free(void* ptr);
    bool Protect(void* address, size_t size, bool readable, bool writable, bool executable);
    
    bool MapMemory(uintptr_t guest_address, void* host_address, size_t size, bool writable);
    bool UnmapMemory(uintptr_t guest_address, size_t size);
    
    bool ReadMemory(uintptr_t guest_address, void* buffer, size_t size);
    bool WriteMemory(uintptr_t guest_address, const void* buffer, size_t size);
    
    size_t GetTotalMemory() const { return total_memory; }
    size_t GetUsedMemory() const { return used_memory; }
    size_t GetFreeMemory() const { return total_memory - used_memory; }
    
private:
    size_t total_memory;
    size_t used_memory;
};

} // namespace HaikuVM

#endif // HAIKU_MEMORY_ABSTRACTION_H
