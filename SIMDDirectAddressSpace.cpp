/*
 * SIMD-Optimized DirectAddressSpace Implementation for HaikuOS
 * Hardware-accelerated memory operations using SSE2/AVX2
 */

#include "SIMDDirectAddressSpace.h"
#include <ApplicationKit.h>
#include <StorageKit.h>
#include <InterfaceKit.h>

SIMDDirectAddressSpace::SIMDDirectAddressSpace()
    : DirectAddressSpace(),
      fHasSSE2(false),
      fHasAVX2(false),
      fHasAVX512(false),
      fSIMDArea(-1),
      fAlignedBase(nullptr),
      fAlignment(64)  // AVX-512 alignment
{
    DetectSIMDCapabilities();
    printf("[SIMD] Initialized - SSE2: %s, AVX2: %s, AVX512: %s\n",
           fHasSSE2 ? "Yes" : "No",
           fHasAVX2 ? "Yes" : "No", 
           fHasAVX512 ? "Yes" : "No");
}

SIMDDirectAddressSpace::~SIMDDirectAddressSpace()
{
    if (fSIMDArea >= 0) {
        delete_area(fSIMDArea);
    }
    if (fAlignedBase) {
        free(fAlignedBase);
    }
}

void SIMDDirectAddressSpace::DetectSIMDCapabilities()
{
    // CPUID detection for SIMD capabilities
    uint32 eax, ebx, ecx, edx;
    
    // Basic CPUID - check for SSE2
    __cpuid(1, eax, ebx, ecx, edx);
    fHasSSE2 = (edx & (1 << 26)) != 0;
    
    // Check for AVX2
    if (ecx & (1 << 27)) {  // AVX supported
        __cpuid(7, eax, ebx, ecx, edx);
        fHasAVX2 = (ebx & (1 << 5)) != 0;
    }
    
    // Check for AVX-512F
    if (fHasAVX2) {
        fHasAVX512 = (ebx & (1 << 16)) != 0;
    }
}

status_t SIMDDirectAddressSpace::InitWithHaikuArea(size_t size, const char* areaName)
{
    // Use HaikuOS create_area with optimal alignment for SIMD
    size = (size + fAlignment - 1) & ~(fAlignment - 1);
    
    void* memoryBase = nullptr;
    fSIMDArea = create_area(areaName, &memoryBase, 
                           B_ANY_ADDRESS, size, 
                           B_NO_LOCK, 
                           B_READ_AREA | B_WRITE_AREA);
    
    if (fSIMDArea < B_OK) {
        printf("[SIMD] Failed to create aligned area: %s\n", strerror(fSIMDArea));
        return fSIMDArea;
    }
    
    fGuestBaseAddress = (addr_t)memoryBase;
    fGuestSize = size;
    fAlignedBase = memoryBase;
    
    printf("[SIMD] Created Haiku area: id=%d, base=%p, size=%zu, alignment=%zu\n",
           fSIMDArea, memoryBase, size, fAlignment);
    
    return B_OK;
}

status_t SIMDDirectAddressSpace::Read(uintptr_t guestAddress, void* buffer, size_t size)
{
    if (!buffer || size == 0)
        return B_BAD_VALUE;
    
    // Use SIMD-optimized path for large transfers
    if (size >= 64 && fHasSSE2) {
        return ReadVector(guestAddress, buffer, size);
    }
    
    // Fall back to original implementation for small transfers
    return DirectAddressSpace::Read(guestAddress, buffer, size);
}

status_t SIMDDirectAddressSpace::Write(uintptr_t guestAddress, const void* buffer, size_t size)
{
    if (!buffer || size == 0)
        return B_BAD_VALUE;
    
    // Use SIMD-optimized path for large transfers
    if (size >= 64 && fHasSSE2) {
        return WriteVector(guestAddress, buffer, size);
    }
    
    // Fall back to original implementation for small transfers
    return DirectAddressSpace::Write(guestAddress, buffer, size);
}

status_t SIMDDirectAddressSpace::ReadVector(uintptr_t guestAddress, void* buffer, size_t size)
{
    if (fHasAVX2) {
        return ReadAVX2(guestAddress, buffer, size);
    } else if (fHasSSE2) {
        return ReadSSE2(guestAddress, buffer, size);
    }
    
    return DirectAddressSpace::Read(guestAddress, buffer, size);
}

status_t SIMDDirectAddressSpace::WriteVector(uintptr_t guestAddress, const void* buffer, size_t size)
{
    if (fHasAVX2) {
        return WriteAVX2(guestAddress, buffer, size);
    } else if (fHasSSE2) {
        return WriteSSE2(guestAddress, buffer, size);
    }
    
    return DirectAddressSpace::Write(guestAddress, buffer, size);
}

status_t SIMDDirectAddressSpace::ReadSSE2(uintptr_t guestAddress, void* buffer, size_t size)
{
    // Calculate guest offset using same logic as parent
    uintptr_t offset;
    if (guestAddress >= 0x08000000 && guestAddress < 0x80000000) {
        offset = guestAddress - 0x08000000;
    } else if (guestAddress < 0x08000000) {
        offset = guestAddress;
    } else {
        return B_BAD_VALUE;
    }
    
    if (offset + size > fGuestSize) {
        return B_BAD_VALUE;
    }
    
    uint8_t* src = (uint8_t*)fGuestBaseAddress + offset;
    uint8_t* dst = (uint8_t*)buffer;
    
    // Use SSE2 for aligned bulk copies
    size_t aligned_size = size & ~15;  // Round down to 16-byte boundary
    size_t remainder = size & 15;
    
    if (aligned_size >= 16 && IsAligned(src, 16) && IsAligned(dst, 16)) {
        __m128i* sse_src = (__m128i*)src;
        __m128i* sse_dst = (__m128i*)dst;
        
        for (size_t i = 0; i < aligned_size / 16; i++) {
            _mm_store_si128(&sse_dst[i], _mm_load_si128(&sse_src[i]));
        }
        
        // Copy remaining bytes
        memcpy(dst + aligned_size, src + aligned_size, remainder);
    } else {
        // Fall back to memcpy for unaligned data
        memcpy(dst, src, size);
    }
    
    return B_OK;
}

status_t SIMDDirectAddressSpace::ReadAVX2(uintptr_t guestAddress, void* buffer, size_t size)
{
    // Calculate guest offset using same logic as parent
    uintptr_t offset;
    if (guestAddress >= 0x08000000 && guestAddress < 0x80000000) {
        offset = guestAddress - 0x08000000;
    } else if (guestAddress < 0x08000000) {
        offset = guestAddress;
    } else {
        return B_BAD_VALUE;
    }
    
    if (offset + size > fGuestSize) {
        return B_BAD_VALUE;
    }
    
    uint8_t* src = (uint8_t*)fGuestBaseAddress + offset;
    uint8_t* dst = (uint8_t*)buffer;
    
    // Use AVX2 for aligned bulk copies
    size_t aligned_size = size & ~31;  // Round down to 32-byte boundary
    size_t remainder = size & 31;
    
    if (aligned_size >= 32 && IsAligned(src, 32) && IsAligned(dst, 32)) {
        __m256i* avx_src = (__m256i*)src;
        __m256i* avx_dst = (__m256i*)dst;
        
        for (size_t i = 0; i < aligned_size / 32; i++) {
            _mm256_store_si256(&avx_dst[i], _mm256_load_si256(&avx_src[i]));
        }
        
        // Copy remaining bytes
        memcpy(dst + aligned_size, src + aligned_size, remainder);
    } else {
        // Fall back to SSE2 for unaligned data
        return ReadSSE2(guestAddress, buffer, size);
    }
    
    return B_OK;
}

status_t SIMDDirectAddressSpace::WriteSSE2(uintptr_t guestAddress, const void* buffer, size_t size)
{
    // Similar to ReadSSE2 but for writing
    uintptr_t offset;
    if (guestAddress >= 0x08000000 && guestAddress < 0x80000000) {
        offset = guestAddress - 0x08000000;
    } else if (guestAddress < 0x08000000) {
        offset = guestAddress;
    } else {
        return B_BAD_VALUE;
    }
    
    if (offset + size > fGuestSize) {
        return B_BAD_VALUE;
    }
    
    const uint8_t* src = (const uint8_t*)buffer;
    uint8_t* dst = (uint8_t*)fGuestBaseAddress + offset;
    
    size_t aligned_size = size & ~15;
    size_t remainder = size & 15;
    
    if (aligned_size >= 16 && IsAligned(src, 16) && IsAligned(dst, 16)) {
        const __m128i* sse_src = (const __m128i*)src;
        __m128i* sse_dst = (__m128i*)dst;
        
        for (size_t i = 0; i < aligned_size / 16; i++) {
            _mm_store_si128(&sse_dst[i], _mm_load_si128(&sse_src[i]));
        }
        
        memcpy(dst + aligned_size, src + aligned_size, remainder);
    } else {
        memcpy(dst, src, size);
    }
    
    return B_OK;
}

status_t SIMDDirectAddressSpace::WriteAVX2(uintptr_t guestAddress, const void* buffer, size_t size)
{
    uintptr_t offset;
    if (guestAddress >= 0x08000000 && guestAddress < 0x80000000) {
        offset = guestAddress - 0x08000000;
    } else if (guestAddress < 0x08000000) {
        offset = guestAddress;
    } else {
        return B_BAD_VALUE;
    }
    
    if (offset + size > fGuestSize) {
        return B_BAD_VALUE;
    }
    
    const uint8_t* src = (const uint8_t*)buffer;
    uint8_t* dst = (uint8_t*)fGuestBaseAddress + offset;
    
    size_t aligned_size = size & ~31;
    size_t remainder = size & 31;
    
    if (aligned_size >= 32 && IsAligned(src, 32) && IsAligned(dst, 32)) {
        const __m256i* avx_src = (const __m256i*)src;
        __m256i* avx_dst = (__m256i*)dst;
        
        for (size_t i = 0; i < aligned_size / 32; i++) {
            _mm256_store_si256(&avx_dst[i], _mm256_load_si256(&avx_src[i]));
        }
        
        memcpy(dst + aligned_size, src + aligned_size, remainder);
    } else {
        return WriteSSE2(guestAddress, buffer, size);
    }
    
    return B_OK;
}

status_t SIMDDirectAddressSpace::ClearMemory(uintptr_t guestAddress, size_t size)
{
    uintptr_t offset;
    if (guestAddress >= 0x08000000 && guestAddress < 0x80000000) {
        offset = guestAddress - 0x08000000;
    } else if (guestAddress < 0x08000000) {
        offset = guestAddress;
    } else {
        return B_BAD_VALUE;
    }
    
    if (offset + size > fGuestSize) {
        return B_BAD_VALUE;
    }
    
    uint8_t* dst = (uint8_t*)fGuestBaseAddress + offset;
    
    if (fHasAVX2 && IsAligned(dst, 32)) {
        __m256i zero = _mm256_setzero_si256();
        __m256i* avx_dst = (__m256i*)dst;
        
        for (size_t i = 0; i < size / 32; i++) {
            _mm256_store_si256(&avx_dst[i], zero);
        }
        
        // Clear remaining bytes
        memset(dst + (size & ~31), 0, size & 31);
    } else if (fHasSSE2 && IsAligned(dst, 16)) {
        __m128i zero = _mm_setzero_si128();
        __m128i* sse_dst = (__m128i*)dst;
        
        for (size_t i = 0; i < size / 16; i++) {
            _mm_store_si128(&sse_dst[i], zero);
        }
        
        memset(dst + (size & ~15), 0, size & 15);
    } else {
        memset(dst, 0, size);
    }
    
    return B_OK;
}

status_t SIMDDirectAddressSpace::PrefetchInstructions(uintptr_t guestAddress, size_t size)
{
    uintptr_t offset;
    if (guestAddress >= 0x08000000 && guestAddress < 0x80000000) {
        offset = guestAddress - 0x08000000;
    } else if (guestAddress < 0x08000000) {
        offset = guestAddress;
    } else {
        return B_BAD_VALUE;
    }
    
    if (offset + size > fGuestSize) {
        return B_BAD_VALUE;
    }
    
    uint8_t* addr = (uint8_t*)fGuestBaseAddress + offset;
    
    // Use prefetch instructions to cache instruction data
    for (size_t i = 0; i < size; i += 64) {
        _mm_prefetch((const char*)(addr + i), _MM_HINT_T0);
    }
    
    return B_OK;
}