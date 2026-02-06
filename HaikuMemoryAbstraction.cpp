/*
 * Haiku Memory Abstraction Implementation
 * Cross-platform memory management solving type conflicts
 */

#include "HaikuMemoryAbstraction.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Platform detection
#ifdef __HAIKU__
#include <OS.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// Forward declarations for SSE intrinsics
#ifdef __SSE2__
#include <emmintrin.h>
#endif

HaikuMemoryAbstraction& HaikuMemoryAbstraction::GetInstance()
{
    static HaikuMemoryAbstraction instance;
    return instance;
}

status_t HaikuMemoryAbstraction::AllocateArea(const char* name, void** address, addr_t spec,
                                             vm_size_t size, uint32_t lock, uint32_t protection,
                                             HaikuMemoryArea** area)
{
    if (!name || !address || !area) {
        return HAIKU_BAD_VALUE;
    }
    
    // Platform-specific allocation
    if (IsHaikuOS() && SupportsNativeAreas()) {
        return CreateHaikuArea(name, address, spec, size, lock, protection, area);
    } else {
        return CreatePOSIXArea(name, address, spec, size, lock, protection, area);
    }
}

status_t HaikuMemoryAbstraction::AllocateSimple(void** memory, size_t size)
{
    if (!memory) {
        return HAIKU_BAD_VALUE;
    }
    
    *memory = malloc(size);
    return *memory ? HAIKU_OK : HAIKU_NO_MEMORY;
}

status_t HaikuMemoryAbstraction::FreeMemory(void* memory)
{
    if (memory) {
        free(memory);
    }
    return HAIKU_OK;
}

status_t HaikuMemoryAbstraction::ProtectMemory(void* address, size_t size, uint32_t protection)
{
    if (!address) {
        return HAIKU_BAD_VALUE;
    }
    
#ifdef __HAIKU__
    // Use Haiku's memory protection
    int prot = 0;
    if (protection & MEMORY_READ) prot |= PROT_READ;
    if (protection & MEMORY_WRITE) prot |= PROT_WRITE;
    if (protection & MEMORY_EXECUTE) prot |= PROT_EXEC;
    
    if (mprotect(address, size, prot) != 0) {
        return HAIKU_ERROR;
    }
#else
    // Use POSIX memory protection
    int prot = 0;
    if (protection & MEMORY_READ) prot |= PROT_READ;
    if (protection & MEMORY_WRITE) prot |= PROT_WRITE;
    if (protection & MEMORY_EXECUTE) prot |= PROT_EXEC;
    
    if (mprotect(address, size, prot) != 0) {
        return HAIKU_ERROR;
    }
#endif
    
    return HAIKU_OK;
}

status_t HaikuMemoryAbstraction::UnprotectMemory(void* address, size_t size)
{
    return ProtectMemory(address, size, MEMORY_READ | MEMORY_WRITE | MEMORY_EXECUTE);
}

status_t HaikuMemoryAbstraction::MapFile(const char* path, void** address, size_t* size)
{
    if (!path || !address || !size) {
        return HAIKU_BAD_VALUE;
    }
    
    FILE* file = fopen(path, "rb");
    if (!file) {
        return HAIKU_ERROR;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    fclose(file);
    
    // Allocate memory and map file
    *address = malloc(*size);
    if (!*address) {
        return HAIKU_NO_MEMORY;
    }
    
    // Read file into memory
    file = fopen(path, "rb");
    if (!file || fread(*address, *size, 1, file) != 1) {
        free(*address);
        *address = nullptr;
        fclose(file);
        return HAIKU_ERROR;
    }
    fclose(file);
    
    return HAIKU_OK;
}

status_t HaikuMemoryAbstraction::UnmapFile(void* address, size_t size)
{
    if (address) {
        free(address);
    }
    return HAIKU_OK;
}

bool HaikuMemoryAbstraction::IsHaikuOS() const
{
#ifdef __HAIKU__
    return true;
#else
    return false;
#endif
}

bool HaikuMemoryAbstraction::SupportsNativeAreas() const
{
#ifdef __HAIKU__
    return true;
#else
    return false;
#endif
}

status_t HaikuMemoryAbstraction::PrefetchMemory(void* address, size_t size)
{
    if (!address) {
        return HAIKU_BAD_VALUE;
    }
    
    // Platform-specific prefetch
#ifdef __SSE2__
    #include <emmintrin.h>
    const uint8_t* p = (const uint8_t*)address;
    size_t prefetchSize = (size + 63) & ~63;  // Round to 64-byte boundaries
    
    for (size_t i = 0; i < prefetchSize; i += 64) {
        _mm_prefetch((const char*)(p + i), 0);  // _MM_HINT_T0 = 0
    }
#endif
    
    return HAIKU_OK;
}

status_t HaikuMemoryAbstraction::FlushCache(void* address, size_t size)
{
    if (!address) {
        return HAIKU_BAD_VALUE;
    }
    
#ifdef __SSE2__
    // Use simpler cache flush
    for (size_t i = 0; i < size; i += 64) {
        _mm_clflush((void*)((uint8_t*)address + i));
    }
    _mm_sfence();
#endif
    
    return HAIKU_OK;
}

// Platform-specific implementations
status_t HaikuMemoryAbstraction::CreateHaikuArea(const char* name, void** address, addr_t spec,
                                                        vm_size_t size, uint32_t lock, uint32_t protection,
                                                        HaikuMemoryArea** area)
{
#ifdef __HAIKU__
    HaikuNativeArea* haikuArea = new HaikuNativeArea();
    status_t result = haikuArea->Create(name, address, spec, size, lock, protection);
    
    if (result == HAIKU_OK) {
        *area = haikuArea;
    } else {
        delete haikuArea;
        *area = nullptr;
    }
    
    return result;
#else
    return HAIKU_ERROR;
#endif
}

status_t HaikuMemoryAbstraction::CreatePOSIXArea(const char* name, void** address, addr_t spec,
                                                       vm_size_t size, uint32_t lock, uint32_t protection,
                                                       HaikuMemoryArea** area)
{
    POSIXArea* posixArea = new POSIXArea();
    status_t result = posixArea->Create(name, address, spec, size, lock, protection);
    
    if (result == HAIKU_OK) {
        *area = posixArea;
    } else {
        delete posixArea;
        *area = nullptr;
    }
    
    return result;
}

// HaikuNativeArea implementation
HaikuNativeArea::HaikuNativeArea()
    : fAreaID(-1), fAddress(nullptr), fSize(0), fCreated(false)
{
    memset(fName, 0, sizeof(fName));
}

HaikuNativeArea::~HaikuNativeArea()
{
    if (fCreated) {
        Delete();
    }
}

status_t HaikuNativeArea::Create(const char* name, void** address, addr_t spec,
                                vm_size_t size, uint32_t lock, uint32_t protection)
{
#ifdef __HAIKU__
    strncpy(fName, name, sizeof(fName) - 1);
    fSize = size;
    
    // Convert protection flags
    uint32_t haikuProtection = 0;
    if (protection & MEMORY_READ) haikuProtection |= B_READ_AREA;
    if (protection & MEMORY_WRITE) haikuProtection |= B_WRITE_AREA;
    
    // Create Haiku area
    fAreaID = create_area(name, address, spec, size, lock, haikuProtection);
    
    if (fAreaID < 0) {
        return HAIKU_ERROR;
    }
    
    fAddress = *address;
    fCreated = true;
    
    printf("[HAIKU_MEMORY] Created Haiku area: %s, id=%d, addr=%p, size=%zu\n",
           name, fAreaID, fAddress, size);
    
    return HAIKU_OK;
#else
    return HAIKU_ERROR;
#endif
}

status_t HaikuNativeArea::Delete()
{
    if (fCreated && fAreaID >= 0) {
#ifdef __HAIKU__
        delete_area(fAreaID);
        printf("[HAIKU_MEMORY] Deleted Haiku area: %s\n", fName);
#endif
        fCreated = false;
        fAreaID = -1;
        fAddress = nullptr;
        fSize = 0;
    }
    return HAIKU_OK;
}

status_t HaikuNativeArea::Resize(vm_size_t newSize)
{
    if (!fCreated) {
        return HAIKU_ERROR;
    }
    
#ifdef __HAIKU__
    status_t result = resize_area(fAreaID, newSize);
    if (result == HAIKU_OK) {
        fSize = newSize;
        printf("[HAIKU_MEMORY] Resized Haiku area: %s to %zu bytes\n", fName, newSize);
    }
    
    return result;
#else
    return HAIKU_ERROR;
#endif
}

status_t HaikuNativeArea::GetInfo(void* info)
{
    // Implementation depends on Haiku's area_info structure
    return HAIKU_OK;
}

void* HaikuNativeArea::GetAddress() const
{
    return fAddress;
}

vm_size_t HaikuNativeArea::GetSize() const
{
    return fSize;
}

area_id HaikuNativeArea::GetID() const
{
    return fAreaID;
}

const char* HaikuNativeArea::GetName() const
{
    return fName;
}

// POSIXArea implementation
POSIXArea::POSIXArea()
    : fAddress(nullptr), fSize(0), fCreated(false)
{
    memset(fName, 0, sizeof(fName));
}

POSIXArea::~POSIXArea()
{
    if (fCreated) {
        Delete();
    }
}

status_t POSIXArea::Create(const char* name, void** address, addr_t spec,
                              vm_size_t size, uint32_t lock, uint32_t protection)
{
    strncpy(fName, name, sizeof(fName) - 1);
    fSize = size;
    
    // Allocate memory for POSIX area
    fAddress = malloc(size);
    if (!fAddress) {
        return HAIKU_NO_MEMORY;
    }
    
    // Set protection if specified
    if (protection != MEMORY_ALL) {
        // Use POSIX memory protection
        int prot = 0;
        if (protection & MEMORY_READ) prot |= PROT_READ;
        if (protection & MEMORY_WRITE) prot |= PROT_WRITE;
        if (protection & MEMORY_EXECUTE) prot |= PROT_EXEC;
        
        mprotect(fAddress, size, prot);
    }
    
    fCreated = true;
    *address = fAddress;
    
    printf("[POSIX_MEMORY] Created area: %s, addr=%p, size=%zu\n", name, fAddress, size);
    
    return HAIKU_OK;
}

status_t POSIXArea::Delete()
{
    if (fCreated && fAddress) {
        printf("[POSIX_MEMORY] Deleted area: %s\n", fName);
        free(fAddress);
        fCreated = false;
        fAddress = nullptr;
        fSize = 0;
    }
    return HAIKU_OK;
}

status_t POSIXArea::Resize(vm_size_t newSize)
{
    if (!fCreated) {
        return HAIKU_ERROR;
    }
    
    void* newAddress = realloc(fAddress, newSize);
    if (!newAddress) {
        return HAIKU_NO_MEMORY;
    }
    
    fAddress = newAddress;
    fSize = newSize;
    
    printf("[POSIX_MEMORY] Resized area: %s to %zu bytes\n", fName, newSize);
    
    return HAIKU_OK;
}

status_t POSIXArea::GetInfo(void* info)
{
    return HAIKU_OK;
}

void* POSIXArea::GetAddress() const
{
    return fAddress;
}

vm_size_t POSIXArea::GetSize() const
{
    return fSize;
}

area_id POSIXArea::GetID() const
{
    return 0;  // POSIX doesn't have area IDs
}

const char* POSIXArea::GetName() const
{
    return fName;
}