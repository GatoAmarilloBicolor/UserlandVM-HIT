/*
 * SIMD-Optimized DirectAddressSpace for HaikuOS
 * Uses SSE2/AVX2 for bulk memory operations and hardware acceleration
 */

#ifndef _SIMD_DIRECT_ADDRESS_SPACE_H
#define _SIMD_DIRECT_ADDRESS_SPACE_H

#include "DirectAddressSpace.h"
#include "SupportDefs.h"

// SIMD intrinsics for x86-64 (HaikuOS)
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __AVX2__
#include <immintrin.h>
#endif

class SIMDDirectAddressSpace : public DirectAddressSpace {
public:
    SIMDDirectAddressSpace();
    virtual ~SIMDDirectAddressSpace();

    // SIMD-optimized bulk operations
    virtual status_t Read(uintptr_t guestAddress, void* buffer, size_t size) override;
    virtual status_t Write(uintptr_t guestAddress, const void* buffer, size_t size) override;
    
    // HaikuOS Kit integration
    status_t InitWithHaikuArea(size_t size, const char* areaName = "userlandvm_simd");
    status_t MapWithHaikuVM();
    
    // SIMD-specific optimizations
    status_t ReadVector(uintptr_t guestAddress, void* buffer, size_t size);
    status_t WriteVector(uintptr_t guestAddress, const void* buffer, size_t size);
    
    // Hardware-accelerated memory clearing
    status_t ClearMemory(uintptr_t guestAddress, size_t size);
    
    // Prefetch optimization for instruction fetching
    status_t PrefetchInstructions(uintptr_t guestAddress, size_t size);

private:
    // SIMD capabilities detection
    bool fHasSSE2;
    bool fHasAVX2;
    bool fHasAVX512;
    
    // HaikuOS-specific optimizations
    area_id fSIMDArea;
    void* fAlignedBase;
    size_t fAlignment;
    
    // SIMD helper methods
    void DetectSIMDCapabilities();
    status_t ReadSSE2(uintptr_t guestAddress, void* buffer, size_t size);
    status_t ReadAVX2(uintptr_t guestAddress, void* buffer, size_t size);
    status_t WriteSSE2(uintptr_t guestAddress, const void* buffer, size_t size);
    status_t WriteAVX2(uintptr_t guestAddress, const void* buffer, size_t size);
    
    // Alignment helpers
    inline bool IsAligned(const void* ptr, size_t alignment) {
        return ((uintptr_t)ptr & (alignment - 1)) == 0;
    }
    
    inline void* AlignPointer(void* ptr, size_t alignment) {
        return (void*)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
    }
};

#endif