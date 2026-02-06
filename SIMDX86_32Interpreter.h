/*
 * SIMD-Optimized X86-32 Interpreter for HaikuOS
 * Hardware-accelerated instruction execution using SIMD
 */

#ifndef _SIMD_X86_INTERPRETER_H
#define _SIMD_X86_INTERPRETER_H

#include "InterpreterX86_32.h"
#include "SIMDDirectAddressSpace.h"
#include "SupportDefs.h"

#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __AVX2__
#include <immintrin.h>
#endif

class SIMDX86_32Interpreter : public InterpreterX86_32 {
public:
    SIMDX86_32Interpreter(SIMDDirectAddressSpace& addressSpace, 
                        Haiku32SyscallDispatcher& dispatcher);
    virtual ~SIMDX86_32Interpreter();

    // SIMD-optimized execution
    virtual status_t Run(X86_32GuestContext& context) override;
    
    // Bulk instruction execution with SIMD
    status_t ExecuteBlock(X86_32GuestContext& context, uint32 instructionCount);
    
    // Vectorized string operations
    status_t SIMD_StringCompare(const uint8_t* src1, const uint8_t* src2, size_t len, uint32& result);
    status_t SIMD_StringCopy(uint8_t* dst, const uint8_t* src, size_t len);
    status_t SIMD_StringMove(uint8_t* dst, const uint8_t* src, size_t len);
    
    // SIMD-optimized arithmetic operations
    status_t SIMD_VectorAdd(uint32* dst, const uint32* src1, const uint32* src2, size_t count);
    status_t SIMD_VectorMul(uint32* dst, const uint32* src1, const uint32* src2, size_t count);
    
    // Hardware-accelerated memory operations
    status_t SIMD_MemSet(uint8_t* dst, uint8_t value, size_t size);
    status_t SIMD_MemCmp(const uint8_t* src1, const uint8_t* src2, size_t size, int32& result);

private:
    SIMDDirectAddressSpace& fSIMDAddressSpace;
    
    // SIMD instruction cache
    struct SIMDInstructionCache {
        uint32 address;
        uint8* opcode_data;
        uint32 length;
        bool is_vectorizable;
    };
    
    static const uint32 kCacheSize = 1024;
    SIMDInstructionCache fInstructionCache[kCacheSize];
    uint32 fCacheIndex;
    
    // SIMD detection and optimization
    bool fHasSSE2;
    bool fHasAVX2;
    bool fHasAVX512;
    
    // Performance counters
    uint64 fSIMDInstructions;
    uint64 fVectorizedOps;
    uint64 fCacheHits;
    
    // SIMD helper methods
    void InitSIMDCache();
    bool IsInstructionVectorizable(uint8 opcode);
    status_t ExecuteVectorizableInstruction(X86_32GuestContext& context, uint8* instr);
    
    // SIMD-optimized instruction handlers
    status_t HandleMOVSB_SIMD(X86_32GuestContext& context);
    status_t HandleMOVSW_SIMD(X86_32GuestContext& context);
    status_t HandleMOVSD_SIMD(X86_32GuestContext& context);
    status_t HandleCMPSB_SIMD(X86_32GuestContext& context);
    status_t HandleREP_SCASB_SIMD(X86_32GuestContext& context);
    status_t HandleSTOSB_SIMD(X86_32GuestContext& context);
    
    // Batch memory operations
    status_t BatchMemoryRead(uintptr_t guestAddr, void* buffer, size_t size);
    status_t BatchMemoryWrite(uintptr_t guestAddr, const void* buffer, size_t size);
    
    // JIT compilation hints
    struct JITBlock {
        uint32 start_address;
        uint32 instruction_count;
        uint8* native_code;
        bool is_hot;
    };
    
    static const uint32 kJITBlocks = 256;
    JITBlock fJITBlocks[kJITBlocks];
    uint32 fJITBlockCount;
    
    // Hot path detection
    void DetectHotPath(uint32 address);
    bool ShouldJITCompile(uint32 address);
    
    // SIMD register optimization
    struct SIMDRegisterFile {
        __m128i xmm[8];  // XMM registers for SIMD operations
        bool xmm_used[8];
    };
    
    SIMDRegisterFile fSIMDRegs;
    
    // Prefetching optimization
    void PrefetchInstructionStream(uintptr_t eip, uint32 count);
    void PrefetchDataMemory(uintptr_t addr, size_t size);
};

#endif