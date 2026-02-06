/*
 * Haiku Memory Abstraction Interface
 * Provides cross-platform memory management abstraction
 * Solves conflicts between Haiku and system types
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

// Forward declarations for cross-platform compatibility
#ifdef __HAIKU__
#include <OS.h>
#else
typedef int32_t status_t;
typedef int32_t area_id;
typedef uint32_t addr_t;
typedef uintptr_t phys_addr_t;
typedef uintptr_t vm_addr_t;
typedef size_t vm_size_t;
#endif

// Abstract memory area interface
class HaikuMemoryArea {
public:
    virtual ~HaikuMemoryArea() = default;
    
    virtual status_t Create(const char* name, void** address, addr_t spec, 
                           vm_size_t size, uint32_t lock, uint32_t protection) = 0;
    virtual status_t Delete() = 0;
    virtual status_t Resize(vm_size_t newSize) = 0;
    virtual status_t GetInfo(void* info) = 0;
    virtual void* GetAddress() const = 0;
    virtual vm_size_t GetSize() const = 0;
    virtual area_id GetID() const = 0;
    virtual const char* GetName() const = 0;
};

// Cross-platform memory management implementation
class HaikuMemoryAbstraction {
public:
    static HaikuMemoryAbstraction& GetInstance();
    
    // Memory allocation methods
    status_t AllocateArea(const char* name, void** address, addr_t spec,
                         vm_size_t size, uint32_t lock, uint32_t protection,
                         HaikuMemoryArea** area);
    
    status_t AllocateSimple(void** memory, size_t size);
    status_t FreeMemory(void* memory);
    
    // Memory protection
    status_t ProtectMemory(void* address, size_t size, uint32_t protection);
    status_t UnprotectMemory(void* address, size_t size);
    
    // Memory mapping
    status_t MapFile(const char* path, void** address, size_t* size);
    status_t UnmapFile(void* address, size_t size);
    
    // Platform detection
    bool IsHaikuOS() const;
    bool SupportsNativeAreas() const;
    
    // Performance optimization
    status_t PrefetchMemory(void* address, size_t size);
    status_t FlushCache(void* address, size_t size);

private:
    HaikuMemoryAbstraction() = default;
    ~HaikuMemoryAbstraction() = default;
    
    // Platform-specific implementations
    static status_t CreateHaikuArea(const char* name, void** address, addr_t spec,
                                   vm_size_t size, uint32_t lock, uint32_t protection,
                                   HaikuMemoryArea** area);
    
    static status_t CreatePOSIXArea(const char* name, void** address, addr_t spec,
                                  vm_size_t size, uint32_t lock, uint32_t protection,
                                  HaikuMemoryArea** area);
};

// Haiku-specific area implementation
class HaikuNativeArea : public HaikuMemoryArea {
public:
    HaikuNativeArea();
    virtual ~HaikuNativeArea();
    
    virtual status_t Create(const char* name, void** address, addr_t spec,
                           vm_size_t size, uint32_t lock, uint32_t protection) override;
    virtual status_t Delete() override;
    virtual status_t Resize(vm_size_t newSize) override;
    virtual status_t GetInfo(void* info) override;
    virtual void* GetAddress() const override;
    virtual vm_size_t GetSize() const override;
    virtual area_id GetID() const override;
    virtual const char* GetName() const override;

private:
    area_id fAreaID;
    void* fAddress;
    vm_size_t fSize;
    char fName[64];
    bool fCreated;
};

// POSIX fallback area implementation
class POSIXArea : public HaikuMemoryArea {
public:
    POSIXArea();
    virtual ~POSIXArea();
    
    virtual status_t Create(const char* name, void** address, addr_t spec,
                           vm_size_t size, uint32_t lock, uint32_t protection) override;
    virtual status_t Delete() override;
    virtual status_t Resize(vm_size_t newSize) override;
    virtual status_t GetInfo(void* info) override;
    virtual void* GetAddress() const override;
    virtual vm_size_t GetSize() const override;
    virtual area_id GetID() const override;
    virtual const char* GetName() const override;

private:
    void* fAddress;
    vm_size_t fSize;
    char fName[64];
    bool fCreated;
    uint32_t fProtection;
};

// Constants for memory protection
enum MemoryProtection {
    MEMORY_READ = 0x01,
    MEMORY_WRITE = 0x02,
    MEMORY_EXECUTE = 0x04,
    MEMORY_READ_WRITE = MEMORY_READ | MEMORY_WRITE,
    MEMORY_ALL = MEMORY_READ | MEMORY_WRITE | MEMORY_EXECUTE
};

// Constants for address specification
enum AddressSpecification {
    ADDRESS_ANY = 0x01,
    ADDRESS_SPECIFIC = 0x02,
    ADDRESS_BASE = 0x03
};

// Status codes (Haiku-compatible)
enum {
    HAIKU_OK = 0,
    HAIKU_ERROR = -1,
    HAIKU_NO_MEMORY = -2,
    HAIKU_BAD_VALUE = -3,
    HAIKU_ENTRY_NOT_FOUND = -6,
    HAIKU_NAME_IN_USE = -15,
    HAIKU_PERMISSION_DENIED = -13,
    HAIKU_WOULD_BLOCK = -7
};