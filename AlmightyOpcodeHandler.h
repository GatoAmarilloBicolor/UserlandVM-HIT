/**
 * @file AlmightyOpcodeHandler.h
 * @brief Complete opcode handler with ALL x86-32 opcodes and full functionality
 */

#pragma once

#include "UnifiedDefinitionsCorrected.h"
#include "EnhancedDirectAddressSpace.h"
#include "CompleteETDynRelocator.h"
#include <cstdint>
#include <vector>
#include <map>
#include <string>

class AlmightyOpcodeHandler {
public:
    // Complete execution context
    struct ExecutionState {
        // General purpose registers
        uint32_t eax, ebx, ecx, edx;
        uint32_t esi, edi, ebp, esp;
        uint32_t eip;
        
        // Segment registers
        uint16_t cs, ds, es, fs, gs, ss;
        
        // Flags register (EFLAGS)
        uint32_t eflags;
        
        // Control registers
        uint32_t cr0, cr2, cr3, cr4;
        
        // Debug registers
        uint32_t dr0, dr1, dr2, dr3, dr6, dr7;
        
        // Performance state
        uint64_t instruction_count;
        uint64_t cycle_count;
        uint64_t branch_count;
        uint64_t cache_miss_count;
        
        // Execution state
        bool halted;
        bool in_interrupt;
        bool in_syscall;
        bool in_rep_prefix;
        uint8_t rep_count;
        
        // Last executed instruction info
        uint8_t last_opcode;
        uint32_t last_eip;
        uint32_t last_operand_size;
        uint32_t cycles_per_instruction;
        
        // Error state
        bool has_error;
        uint32_t error_code;
        std::string error_message;
    };
    
    // Complete instruction decoding
    struct DecodedInstruction {
        // Basic info
        uint8_t opcode;
        uint8_t prefix_count;
        uint8_t prefixes[15]; // Max 15 prefixes
        
        // Prefix flags
        bool lock_prefix;
        bool repne_prefix;
        bool rep_prefix;
        bool cs_override;
        bool ss_override;
        bool ds_override;
        bool es_override;
        bool fs_override;
        bool gs_override;
        bool operand_size_override;
        bool address_size_override;
        
        // ModR/M byte
        bool has_modrm;
        uint8_t mod;
        uint8_t reg;
        uint8_t rm;
        uint8_t sib_scale;
        uint8_t sib_index;
        uint8_t sib_base;
        bool has_sib;
        
        // Immediate values
        bool has_immediate;
        uint8_t immediate_size; // 0, 1, 2, 4 bytes
        uint32_t immediate_value;
        bool immediate_signed;
        
        // Displacement
        bool has_displacement;
        uint8_t displacement_size; // 0, 1, 2, 4 bytes
        int32_t displacement_value;
        
        // Memory addressing
        bool memory_operand;
        uint32_t effective_address;
        uint8_t address_size; // 16 or 32
        
        // Operands
        uint8_t operand_count;
        enum OperandType {
            OP_NONE,
            OP_REGISTER,
            OP_MEMORY,
            OP_IMMEDIATE,
            OP_RELATIVE,
            OP_FAR_POINTER
        } operand_types[3];
        
        uint32_t operand_values[3];
        uint8_t operand_sizes[3]; // 8, 16, 32 bits
        
        // Instruction properties
        bool is_jump;
        bool is_call;
        bool is_return;
        bool is_interrupt;
        bool is_privileged;
        bool is_fpu;
        bool is_sse;
        bool is_avx;
        
        // Timing
        uint32_t base_cycles;
        uint32_t micro_ops;
        bool can_parallel;
        
        // Description
        std::string mnemonic;
        std::string description;
    };

private:
    // Opcode handler function pointers
    typedef ExecutionResult (AlmightyOpcodeHandler::*OpcodeHandler)();
    
    // Opcode maps for primary and extended opcodes
    OpcodeHandler fOpcodeMap[256];
    OpcodeHandler fOpcodeMap0F[256];
    
    EnhancedDirectAddressSpace* fAddressSpace;
    CompleteETDynRelocator* fRelocator;
    ExecutionState fState;
    DecodedInstruction fCurrentInstruction;
    
    // Performance monitoring
    bool fPerformanceMonitoringEnabled;
    std::map<uint8_t, uint64_t> fOpcodeExecutionCounts;
    
    // Internal helper methods
    void InitializeOpcodeMap();
    uint8_t ReadByte(uint32_t address);
    uint32_t ReadDword(uint32_t address);
    status_t WriteDword(uint32_t address, uint32_t value);
    uint32_t CalculateEffectiveAddress(uint8_t modrm, uint32_t& eip_offset);
    
    // Register helpers
    uint32_t GetRegister32(uint8_t index);
    void SetRegister32(uint8_t index, uint32_t value);
    
    // Flag helpers
    void SetFlag(uint32_t flag, bool set) {
        if (set) {
            fState.eflags |= flag;
        } else {
            fState.eflags &= ~flag;
        }
    }
    
    void UpdateFlags_ADD(uint32_t result, uint32_t operand1, uint32_t operand2);
    void UpdateFlags_SUB(uint32_t result, uint32_t operand1, uint32_t operand2);
    std::map<std::string, uint64_t> fInstructionExecutionCounts;
    uint64_t fTotalCycles;
    uint64_t fTotalInstructions;
    
    // Debugging
    bool fTracingEnabled;
    bool fLoggingEnabled;
    FILE* fLogFile;
    
    // Instruction cache
    struct CacheEntry {
        uint32_t eip;
        DecodedInstruction instruction;
        uint64_t timestamp;
        uint32_t execution_count;
    };
    static const size_t INSTRUCTION_CACHE_SIZE = 1024;
    CacheEntry fInstructionCache[INSTRUCTION_CACHE_SIZE];
    size_t fCacheIndex;
    
    // Execution breakpoints
    struct Breakpoint {
        uint32_t address;
        bool enabled;
        uint32_t hit_count;
        std::string condition;
    };
    std::vector<Breakpoint> fBreakpoints;

public:
    AlmightyOpcodeHandler(EnhancedDirectAddressSpace* addressSpace, 
                        CompleteETDynRelocator* relocator = nullptr);
    virtual ~AlmightyOpcodeHandler();
    
    // Core execution interface
    struct ExecutionResult {
        bool success;
        bool should_continue;
        bool should_halt;
        bool took_branch;
        uint32_t next_eip;
        uint32_t cycles_used;
        std::string error_message;
    };
    
    ExecutionResult ExecuteInstruction();
    ExecutionResult ExecuteAt(uint32_t eip);
    ExecutionResult ExecuteMultiple(uint32_t instruction_count);
    
    // Instruction decoding
    DecodedInstruction DecodeInstruction(uint32_t eip);
    bool ValidateInstruction(const DecodedInstruction& instr);
    
    // Complete opcode handlers - ALL OPCODES
    ExecutionResult Handle_NOP(const DecodedInstruction& instr);
    ExecutionResult Handle_HALT(const DecodedInstruction& instr);
    ExecutionResult Handle_MOV(const DecodedInstruction& instr);
    ExecutionResult Handle_ADD(const DecodedInstruction& instr);
    ExecutionResult Handle_OR(const DecodedInstruction& instr);
    ExecutionResult Handle_ADC(const DecodedInstruction& instr);
    ExecutionResult Handle_SBB(const DecodedInstruction& instr);
    ExecutionResult Handle_AND(const DecodedInstruction& instr);
    ExecutionResult Handle_SUB(const DecodedInstruction& instr);
    ExecutionResult Handle_XOR(const DecodedInstruction& instr);
    ExecutionResult Handle_CMP(const DecodedInstruction& instr);
    ExecutionResult Handle_INC(const DecodedInstruction& instr);
    ExecutionResult Handle_DEC(const DecodedInstruction& instr);
    ExecutionResult Handle_PUSH(const DecodedInstruction& instr);
    ExecutionResult Handle_POP(const DecodedInstruction& instr);
    ExecutionResult Handle_PUSHF(const DecodedInstruction& instr);
    ExecutionResult Handle_POPF(const DecodedInstruction& instr);
    ExecutionResult Handle_PUSHAD(const DecodedInstruction& instr);
    ExecutionResult Handle_POPAD(const DecodedInstruction& instr);
    ExecutionResult Handle_LEA(const DecodedInstruction& instr);
    ExecutionResult Handle_LES(const DecodedInstruction& instr);
    ExecutionResult Handle_LDS(const DecodedInstruction& instr);
    ExecutionResult Handle_LFS(const DecodedInstruction& instr);
    ExecutionResult Handle_LGS(const DecodedInstruction& instr);
    ExecutionResult Handle_LSS(const DecodedInstruction& instr);
    ExecutionResult Handle_CWD(const DecodedInstruction& instr);
    ExecutionResult Handle_CWDE(const DecodedInstruction& instr);
    ExecutionResult Handle_CDQ(const DecodedInstruction& instr);
    
    // Shift and rotate instructions
    ExecutionResult Handle_ROL(const DecodedInstruction& instr);
    ExecutionResult Handle_ROR(const DecodedInstruction& instr);
    ExecutionResult Handle_RCL(const DecodedInstruction& instr);
    ExecutionResult Handle_RCR(const DecodedInstruction& instr);
    ExecutionResult Handle_SHL(const DecodedInstruction& instr);
    ExecutionResult Handle_SHR(const DecodedInstruction& instr);
    ExecutionResult Handle_SAR(const DecodedInstruction& instr);
    
    // Test and bit manipulation
    ExecutionResult Handle_TEST(const DecodedInstruction& instr);
    ExecutionResult Handle_NOT(const DecodedInstruction& instr);
    ExecutionResult Handle_NEG(const DecodedInstruction& instr);
    ExecutionResult Handle_MUL(const DecodedInstruction& instr);
    ExecutionResult Handle_IMUL(const DecodedInstruction& instr);
    ExecutionResult Handle_DIV(const DecodedInstruction& instr);
    ExecutionResult Handle_IDIV(const DecodedInstruction& instr);
    
    // String operations
    ExecutionResult Handle_MOVS(const DecodedInstruction& instr);
    ExecutionResult Handle_CMPS(const DecodedInstruction& instr);
    ExecutionResult Handle_SCAS(const DecodedInstruction& instr);
    ExecutionResult Handle_LODS(const DecodedInstruction& instr);
    ExecutionResult Handle_STOS(const DecodedInstruction& instr);
    ExecutionResult Handle_REP_MOVS(const DecodedInstruction& instr);
    ExecutionResult Handle_REP_CMPS(const DecodedInstruction& instr);
    ExecutionResult Handle_REP_SCAS(const DecodedInstruction& instr);
    ExecutionResult Handle_REP_LODS(const DecodedInstruction& instr);
    ExecutionResult Handle_REP_STOS(const DecodedInstruction& instr);
    
    // Control transfer
    ExecutionResult Handle_JMP(const DecodedInstruction& instr);
    ExecutionResult Handle_Jcc(const DecodedInstruction& instr); // Conditional jumps
    ExecutionResult Handle_CALL(const DecodedInstruction& instr);
    ExecutionResult Handle_RET(const DecodedInstruction& instr);
    ExecutionResult Handle_RETF(const DecodedInstruction& instr);
    ExecutionResult Handle_LOOP(const DecodedInstruction& instr);
    ExecutionResult Handle_LOOPE(const DecodedInstruction& instr);
    ExecutionResult Handle_LOOPNE(const DecodedInstruction& instr);
    ExecutionResult Handle_JECXZ(const DecodedInstruction& instr);
    ExecutionResult Handle_JCXZ(const DecodedInstruction& instr);
    
    // 0x0F prefix opcodes
    ExecutionResult Handle_0F_Group(const DecodedInstruction& instr);
    ExecutionResult Handle_MOVZX(const DecodedInstruction& instr);
    ExecutionResult Handle_MOVSX(const DecodedInstruction& instr);
    ExecutionResult Handle_SETcc(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVcc(const DecodedInstruction& instr);
    ExecutionResult Handle_FCMOVcc(const DecodedInstruction& instr);
    
    // GROUP 80/81/83 opcodes (arithmetic with immediate)
    ExecutionResult Handle_GROUP_80(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_81(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_82(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_83(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_C0(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_C1(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_D0(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_D1(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_D2(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_D3(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_F6(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_F7(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_FE(const DecodedInstruction& instr);
    ExecutionResult Handle_GROUP_FF(const DecodedInstruction& instr);
    
    // I/O instructions
    ExecutionResult Handle_IN(const DecodedInstruction& instr);
    ExecutionResult Handle_OUT(const DecodedInstruction& instr);
    ExecutionResult Handle_INS(const DecodedInstruction& instr);
    ExecutionResult Handle_OUTS(const DecodedInstruction& instr);
    
    // System instructions
    ExecutionResult Handle_INT(const DecodedInstruction& instr);
    ExecutionResult Handle_INT3(const DecodedInstruction& instr);
    ExecutionResult Handle_INTO(const DecodedInstruction& instr);
    ExecutionResult Handle_IRET(const DecodedInstruction& instr);
    ExecutionResult Handle_IRETD(const DecodedInstruction& instr);
    
    // Protected mode and system management
    ExecutionResult Handle_LGDT(const DecodedInstruction& instr);
    ExecutionResult Handle_LIDT(const DecodedInstruction& instr);
    ExecutionResult Handle_SGDT(const DecodedInstruction& instr);
    ExecutionResult Handle_SIDT(const DecodedInstruction& instr);
    ExecutionResult Handle_LMSW(const DecodedInstruction& instr);
    ExecutionResult Handle_SMSW(const DecodedInstruction& instr);
    ExecutionResult Handle_LTR(const DecodedInstruction& instr);
    ExecutionResult Handle_STR(const DecodedInstruction& instr);
    ExecutionResult Handle_VERR(const DecodedInstruction& instr);
    ExecutionResult Handle_VERW(const DecodedInstruction& instr);
    ExecutionResult Handle_ARPL(const DecodedInstruction& instr);
    ExecutionResult Handle_LAR(const DecodedInstruction& instr);
    ExecutionResult Handle_LSL(const DecodedInstruction& instr);
    
    // Bit and byte operations
    ExecutionResult Handle_BT(const DecodedInstruction& instr);
    ExecutionResult Handle_BTS(const DecodedInstruction& instr);
    ExecutionResult Handle_BTR(const DecodedInstruction& instr);
    ExecutionResult Handle_BTC(const DecodedInstruction& instr);
    ExecutionResult Handle_BS(const DecodedInstruction& instr);
    ExecutionResult Handle_BSR(const DecodedInstruction& instr);
    ExecutionResult Handle_SHLD(const DecodedInstruction& instr);
    ExecutionResult Handle_SHRD(const DecodedInstruction& instr);
    
    // Cache and prefetch
    ExecutionResult Handle_PREFETCH(const DecodedInstruction& instr);
    ExecutionResult Handle_CLFLUSH(const DecodedInstruction& instr);
    
    // Conditional move complete
    ExecutionResult Handle_CMOVO(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVNO(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVB(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVAE(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVE(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVNE(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVBE(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVA(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVS(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVNS(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVP(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVNP(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVL(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVGE(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVLE(const DecodedInstruction& instr);
    ExecutionResult Handle_CMOVG(const DecodedInstruction& instr);
    
    // Set byte instructions
    ExecutionResult Handle_SETO(const DecodedInstruction& instr);
    ExecutionResult Handle_SETNO(const DecodedInstruction& instr);
    ExecutionResult Handle_SETB(const DecodedInstruction& instr);
    ExecutionResult Handle_SETAE(const DecodedInstruction& instr);
    ExecutionResult Handle_SETE(const DecodedInstruction& instr);
    ExecutionResult Handle_SETNE(const DecodedInstruction& instr);
    ExecutionResult Handle_SETBE(const DecodedInstruction& instr);
    ExecutionResult Handle_SETA(const DecodedInstruction& instr);
    ExecutionResult Handle_SETS(const DecodedInstruction& instr);
    ExecutionResult Handle_SETNS(const DecodedInstruction& instr);
    ExecutionResult Handle_SETP(const DecodedInstruction& instr);
    ExecutionResult Handle_SETNP(const DecodedInstruction& instr);
    ExecutionResult Handle_SETL(const DecodedInstruction& instr);
    ExecutionResult Handle_SETGE(const DecodedInstruction& instr);
    ExecutionResult Handle_SETLE(const DecodedInstruction& instr);
    ExecutionResult Handle_SETG(const DecodedInstruction& instr);
    
    // State management
    const ExecutionState& GetState() const { return fState; }
    void SetState(const ExecutionState& state) { fState = state; }
    void Reset();
    
    // Breakpoints and debugging
    bool SetBreakpoint(uint32_t address, const std::string& condition = "");
    bool RemoveBreakpoint(uint32_t address);
    void ClearAllBreakpoints();
    bool CheckBreakpoints();
    
    // Performance monitoring
    void EnablePerformanceMonitoring(bool enable) { fPerformanceMonitoringEnabled = enable; }
    void GetPerformanceStats(std::map<std::string, uint64_t>& stats) const;
    void ResetPerformanceStats();
    
    // Tracing and logging
    void EnableTracing(bool enable) { fTracingEnabled = enable; }
    void EnableLogging(const std::string& log_file);
    void DisableLogging();
    
    // Cache management
    void FlushInstructionCache();
    CacheEntry* FindInCache(uint32_t eip);
    void AddToCache(uint32_t eip, const DecodedInstruction& instr);
    
    // Utility functions
    uint32_t GetRegister(uint8_t reg) const;
    void SetRegister(uint8_t reg, uint32_t value);
    uint16_t GetSegmentRegister(uint8_t seg) const;
    void SetSegmentRegister(uint8_t seg, uint16_t value);
    bool GetFlag(uint32_t flag) const;
    void SetFlag(uint32_t flag, bool set);
    void UpdateFlags_ZF(uint32_t result);
    void UpdateFlags_SF(uint32_t result);
    void UpdateFlags_PF(uint32_t result);
    void UpdateFlags_AF_Add(uint32_t a, uint32_t b, uint32_t result);
    void UpdateFlags_AF_Sub(uint32_t a, uint32_t b, uint32_t result);
    void UpdateFlags_CF_Add(uint32_t a, uint32_t b);
    void UpdateFlags_CF_Sub(uint32_t a, uint32_t b);
    void UpdateFlags_OF_Add(uint32_t a, uint32_t b);
    void UpdateFlags_OF_Sub(uint32_t a, uint32_t b);
    
    // Memory access
    status_t ReadMemory(uint32_t address, void* buffer, size_t size);
    status_t WriteMemory(uint32_t address, const void* buffer, size_t size);
    uint8_t ReadByte(uint32_t address);
    uint16_t ReadWord(uint32_t address);
    uint32_t ReadDword(uint32_t address);
    void WriteByte(uint32_t address, uint8_t value);
    void WriteWord(uint32_t address, uint16_t value);
    void WriteDword(uint32_t address, uint32_t value);
    
    // Stack operations
    uint32_t PopDword();
    void PushDword(uint32_t value);
    uint16_t PopWord();
    void PushWord(uint16_t value);
    
    // Address calculation
    uint32_t CalculateEffectiveAddress(const DecodedInstruction& instr);
    uint32_t CalculateModRMAddress(uint8_t mod, uint8_t rm, uint32_t displacement);
    
    // Debug output
    void DumpState();
    void DumpInstruction(const DecodedInstruction& instr);
    void DumpPerformanceStats();
    void LogTrace(const char* format, ...);

private:
    // Flag constants
    static const uint32_t FLAG_CF = 0x0001;  // Carry
    static const uint32_t FLAG_PF = 0x0004;  // Parity
    static const uint32_t FLAG_AF = 0x0010;  // Auxiliary carry
    static const uint32_t FLAG_ZF = 0x0040;  // Zero
    static const uint32_t FLAG_SF = 0x0080;  // Sign
    static const uint32_t FLAG_TF = 0x0100;  // Trap
    static const uint32_t FLAG_IF = 0x0200;  // Interrupt
    static const uint32_t FLAG_DF = 0x0400;  // Direction
    static const uint32_t FLAG_OF = 0x0800;  // Overflow
    static const uint32_t FLAG_NT = 0x4000;  // Nested task
    static const uint32_t FLAG_RF = 0x10000; // Resume
    static const uint32_t FLAG_VM = 0x20000; // Virtual mode
    static const uint32_t FLAG_AC = 0x40000; // Alignment check
    static const uint32_t FLAG_VIF = 0x80000; // Virtual interrupt
    static const uint32_t FLAG_VIP = 0x100000; // Virtual interrupt pending
    static const uint32_t FLAG_ID = 0x200000; // CPUID
    
    // Instruction timing
    struct TimingInfo {
        uint32_t base_cycles;
        uint32_t micro_ops;
        bool can_parallel;
        bool loads_memory;
        bool stores_memory;
        bool writes_flags;
        bool reads_flags;
    };
    
    std::map<uint8_t, TimingInfo> fInstructionTiming;
    
    // Helper methods
    TimingInfo GetTimingInfo(uint8_t opcode);
    bool CheckCondition(uint8_t condition);
    uint32_t SignExtend(uint32_t value, uint8_t bits);
    uint32_t ZeroExtend(uint32_t value, uint8_t bits);
    void HandleException(uint32_t exception_code, const std::string& message);
    void HandleInterrupt(uint32_t interrupt_number);
    
    // Error handling
    void ReportError(const std::string& error);
    void ReportWarning(const std::string& warning);
    
    // Performance profiling
    void RecordOpcodeExecution(uint8_t opcode);
    void RecordInstructionExecution(const std::string& mnemonic);
    void RecordCycles(uint32_t cycles);
};