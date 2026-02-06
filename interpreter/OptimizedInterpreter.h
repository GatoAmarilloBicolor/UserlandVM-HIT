/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * OptimizedInterpreter.h - High-performance x86-32 interpreter with dispatch tables
 */

#ifndef OPTIMIZED_INTERPRETER_H
#define OPTIMIZED_INTERPRETER_H

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <vector>
#include <memory>

// Forward declarations
class X86_32GuestContext;
class AddressSpace;
class RecycledSyscalls;

class OptimizedInterpreter {
public:
    // Instruction handler function signature
    using InstructionHandler = void(*)(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    
    // Syscall handler function signature
    using SyscallHandler = int(*)(X86_32GuestContext& ctx);
    
    // Instruction categories for optimized dispatch
    enum InstructionCategory {
        CATEGORY_ARITHMETIC = 0,
        CATEGORY_LOGIC = 1,
        CATEGORY_MEMORY = 2,
        CATEGORY_JUMP = 3,
        CATEGORY_STACK = 4,
        CATEGORY_STRING = 5,
        CATEGORY_SYSTEM = 6,
        CATEGORY_FPU = 7,
        CATEGORY_SIMD = 8,
        CATEGORY_PRIVILEGED = 9,
        CATEGORY_EXTENDED = 10,
        CATEGORY_COUNT = 11
    };

    // Performance metrics for interpreter
    struct InterpreterMetrics {
        uint64_t total_instructions_executed;
        uint64_t total_execution_time_us;
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t fast_path_instructions;
        uint64_t slow_path_instructions;
        uint64_t syscall_count;
        uint64_t memory_access_count;
        uint64_t branch_taken_count;
        uint64_t branch_not_taken_count;
        double avg_instructions_per_second;
        double avg_cache_hit_rate;
        std::unordered_map<uint8_t, uint64_t> instruction_frequency;
        
        InterpreterMetrics() : total_instructions_executed(0), total_execution_time_us(0),
                           cache_hits(0), cache_misses(0), fast_path_instructions(0),
                           slow_path_instructions(0), syscall_count(0), memory_access_count(0),
                           branch_taken_count(0), branch_not_taken_count(0),
                           avg_instructions_per_second(0.0), avg_cache_hit_rate(0.0) {}
    };

    // Instruction cache entry
    struct CacheEntry {
        uint32_t guest_address;
        uint8_t opcode;
        InstructionHandler handler;
        uint32_t instruction_length;
        bool is_valid;
        uint64_t execution_count;
        
        CacheEntry() : guest_address(0), opcode(0), handler(nullptr), 
                       instruction_length(0), is_valid(false), execution_count(0) {}
    };

    // Block cache for super-fast execution
    struct BasicBlock {
        uint32_t start_address;
        uint32_t end_address;
        std::vector<uint8_t> instructions;
        std::vector<InstructionHandler> handlers;
        uint32_t execution_count;
        bool is_optimized;
        
        BasicBlock() : start_address(0), end_address(0), execution_count(0), is_optimized(false) {}
    };

private:
    // Main dispatch table - O(1) instruction lookup
    static constexpr size_t DISPATCH_TABLE_SIZE = 256;
    InstructionHandler fDispatchTable[DISPATCH_TABLE_SIZE];
    
    // Secondary dispatch table for prefix bytes
    static constexpr size_t PREFIX_DISPATCH_SIZE = 256;
    InstructionHandler fPrefixDispatchTable[PREFIX_DISPATCH_SIZE];
    
    // Instruction cache for frequently executed instructions
    std::unordered_map<uint32_t, CacheEntry> fInstructionCache;
    static constexpr size_t MAX_CACHE_SIZE = 4096;
    
    // Basic block cache for optimized execution
    std::unordered_map<uint32_t, BasicBlock> fBasicBlockCache;
    static constexpr size_t MAX_BASIC_BLOCKS = 1024;
    
    // Syscall dispatcher for fast system calls
    std::unique_ptr<RecycledSyscalls> fSyscallDispatcher;
    std::unordered_map<uint32_t, SyscallHandler> fSyscallHandlers;
    
    // Current execution state
    X86_32GuestContext* fCurrentContext;
    AddressSpace* fAddressSpace;
    uint8_t* fCurrentInstructionPointer;
    bool fIsRunning;
    bool fSingleStepMode;
    
    // Performance metrics
    InterpreterMetrics fMetrics;
    
    // Optimization flags
    bool fBlockCacheEnabled;
    bool fInstructionCacheEnabled;
    bool fProfileMode;
    bool fOptimizationEnabled;

public:
    OptimizedInterpreter();
    ~OptimizedInterpreter();
    
    // Core execution methods
    bool Initialize(X86_32GuestContext* context, AddressSpace* address_space);
    void Execute();
    void ExecuteSingleInstruction();
    void ExecuteBasicBlock(uint32_t start_address);
    
    // Execution control
    void Start();
    void Stop();
    void Pause();
    void Resume();
    void SingleStep(bool enable) { fSingleStepMode = enable; }
    
    // Cache management
    void ClearInstructionCache();
    void ClearBasicBlockCache();
    void WarmupCache(uint32_t start_address, size_t count);
    
    // Optimization
    void EnableBlockCache(bool enable) { fBlockCacheEnabled = enable; }
    void EnableInstructionCache(bool enable) { fInstructionCacheEnabled = enable; }
    void EnableProfiling(bool enable) { fProfileMode = enable; }
    void EnableOptimization(bool enable) { fOptimizationEnabled = enable; }
    
    // Performance analysis
    InterpreterMetrics GetMetrics() const { return fMetrics; }
    void ResetMetrics();
    void PrintMetrics() const;
    void DumpInstructionCache() const;
    void DumpBasicBlockCache() const;
    
    // Debug and inspection
    void SetBreakpoint(uint32_t address);
    void ClearBreakpoint(uint32_t address);
    void ClearAllBreakpoints();
    bool HasBreakpoint(uint32_t address) const;
    
    // Memory and register access
    bool ReadGuestMemory(uint32_t address, void* buffer, size_t size);
    bool WriteGuestMemory(uint32_t address, const void* data, size_t size);
    uint32_t ReadGuestUint32(uint32_t address);
    uint16_t ReadGuestUint16(uint32_t address);
    uint8_t ReadGuestUint8(uint32_t address);
    bool WriteGuestUint32(uint32_t address, uint32_t value);
    bool WriteGuestUint16(uint32_t address, uint16_t value);
    bool WriteGuestUint8(uint32_t address, uint8_t value);

private:
    // Setup and initialization
    bool SetupDispatchTable();
    bool SetupPrefixDispatchTable();
    bool SetupSyscallHandlers();
    bool InitializeCaches();
    
    // Main instruction fetch and decode
    uint8_t FetchByte();
    uint16_t FetchWord();
    uint32_t FetchDword();
    uint32_t GetCurrentInstructionAddress() const;
    
    // Dispatch and execution
    InstructionHandler LookupHandler(uint8_t opcode);
    InstructionHandler LookupPrefixHandler(uint8_t prefix, uint8_t opcode);
    CacheEntry* LookupInstructionCache(uint32_t address);
    BasicBlock* LookupBasicBlock(uint32_t address);
    
    // Cache management
    void CacheInstruction(uint32_t address, uint8_t opcode, InstructionHandler handler, uint32_t length);
    void CacheBasicBlock(uint32_t start_address, const BasicBlock& block);
    void EvictOldestCacheEntries();
    void OptimizeBasicBlock(BasicBlock& block);
    
    // Instruction handlers - optimized implementations
    static void HandleNOP(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleADD_R32_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleSUB_R32_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleMOV_R32_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleJMP_REL32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleJCC_REL32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleCALL_REL32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleRET(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandlePUSH_R32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandlePOP_R32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleSYSCALL(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleINT3(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    
    // Memory operation handlers
    static void HandleLEA(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleMOVSX(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleMOVZX(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    
    // Arithmetic handlers
    static void HandleIMUL_R32_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleIDIV_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleSHL_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleSHR_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    
    // Logic handlers
    static void HandleAND_R32_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleOR_R32_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleXOR_R32_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleNOT_RM32(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    
    // String handlers
    static void HandleMOVSB(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleCMPSB(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleSCASB(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleLODSB(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleSTOSB(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    
    // Flag operations
    static void HandleCLD(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleSTD(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleCLC(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    static void HandleSTC(X86_32GuestContext& ctx, uint8_t* instruction_ptr);
    
    // Advanced instruction helpers
    void ExecuteComplexInstruction(uint8_t* instruction_ptr);
    void HandleModRM(uint8_t** instruction_ptr, uint32_t* effective_address, uint32_t* register_operand);
    void UpdateEFLAGS_Arithmetic(uint32_t result, uint32_t operand1, uint32_t operand2, bool is_32bit);
    void UpdateEFLAGS_Logic(uint32_t result);
    void UpdateEFLAGS_Shift(uint32_t result, uint32_t shift_count, bool is_arithmetic);
    
    // Memory access optimization
    bool FastMemoryRead(uint32_t address, void* buffer, size_t size);
    bool FastMemoryWrite(uint32_t address, const void* data, size_t size);
    void AlignMemoryAccess(uint32_t address, size_t size, uint32_t* aligned_addr, size_t* aligned_size);
    
    // Branch prediction
    bool PredictBranch(uint32_t address, bool is_conditional);
    void UpdateBranchPrediction(uint32_t address, bool taken, bool predicted);
    
    // Debugging helpers
    void LogInstruction(const char* mnemonic, uint8_t* instruction_ptr, uint32_t length);
    void LogRegisters() const;
    void LogMemoryAccess(uint32_t address, size_t size, bool is_write);
    
    // Performance tracking
    void RecordInstructionExecution(uint8_t opcode, uint32_t cycles);
    void RecordCacheAccess(uint32_t address, bool hit);
    void RecordBranch(bool taken, bool predicted);
    void RecordMemoryAccess(uint32_t address, size_t size, bool is_write);
    
    // Breakpoint management
    std::unordered_set<uint32_t> fBreakpoints;
    
    // Internal state
    uint32_t fExecutionCounter;
    uint64_t fStartTime;
    
    // Constants
    static constexpr uint32_t CACHE_LINE_SIZE = 64;
    static constexpr uint32_t BASIC_BLOCK_MAX_SIZE = 32;
    static constexpr uint32_t PROFILESAMPLE_INTERVAL = 10000;
    
    // Execution modes
    enum ExecutionMode {
        MODE_NORMAL = 0,
        MODE_SINGLE_STEP = 1,
        MODE_DEBUG = 2,
        MODE_PROFILE = 3
    };
    
    ExecutionMode fExecutionMode;
};

#endif // OPTIMIZED_INTERPRETER_H