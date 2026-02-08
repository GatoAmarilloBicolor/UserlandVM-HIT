/*
 * Enhanced x86-32 Interpreter with Missing Opcodes and Improved Stack Initialization
 * Addresses the disconnection issues and missing implementations
 */

#pragma once

#include <SupportDefs.h>
#include <cstdint>
#include "ExecutionEngine.h"
#include "X86_32GuestContext.h"

class GuestContext;
class SyscallDispatcher;
class AddressSpace;
class OptimizedX86Executor;

// ModR/M Decoder Helper Struct
struct ModRM {
    uint8_t mod;              // 0-3 (addressing mode)
    uint8_t reg_op;           // 0-7 (register field or opcode extension)
    uint8_t rm;               // 0-7 (register or memory reference)
    int32_t displacement;     // Displacement value (-128 to +2GB)
    uint8_t bytes_used;       // Total bytes consumed (1-6)
};

// Enhanced x86-32 Interpreter with complete opcode support
class EnhancedInterpreterX86_32 : public ExecutionEngine {
public:
    EnhancedInterpreterX86_32(AddressSpace& addressSpace, SyscallDispatcher& dispatcher);
    virtual ~EnhancedInterpreterX86_32();

    virtual status_t Run(GuestContext& context) override;

    // Enhanced initialization for ET_DYN binaries with proper stack setup
    status_t InitializeStackWithArgv(GuestContext& context, int argc, char** argv, char** envp);

private:
    AddressSpace& fAddressSpace;
    SyscallDispatcher& fDispatcher;
    OptimizedX86Executor* fOptimizedExecutor;

    static const uint32_t MAX_INSTRUCTIONS = 10000000;

    // Core instruction execution
    status_t ExecuteInstruction(GuestContext& context, uint32_t& bytes_consumed);
    status_t DecodeModRM(const uint8_t* instr, ModRM& result);
    uint32_t GetEffectiveAddress(X86_32Registers& regs, const ModRM& modrm);

    // Basic instructions
    status_t Execute_MOV(GuestContext& context, const uint8_t* instr, uint32_t& len);
    status_t Execute_MOV_Load(GuestContext& context, const uint8_t* instr, uint32_t& len);
    status_t Execute_MOV_Load_FS(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_MOV_Store(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_INT(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_PUSH(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_PUSH_Imm(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_POP(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_ADD(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_SUB(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_CMP(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_XOR(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_JMP(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_RET(GuestContext& context, const uint8** instr, uint32_t& len);
    status_t Execute_CALL(GuestContext& context, const uint8** instr, uint32_t& len);

    // Enhanced conditional jumps with 32-bit support
    status_t Execute_JZ_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JNZ_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JZ_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JNZ_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JL_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JL_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JLE_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JLE_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JG_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JG_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JGE_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JGE_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JA_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JA_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JAE_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JAE_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JB_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JB_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JBE_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JBE_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JS_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JS_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JNS_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JNS_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JO_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JO_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JNO_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JNO_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JP_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JP_32(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JNP_8(GuestContext& context, const uint8** instr, uint32& len);
    status_t Execute_JNP_32(GuestContext& context, const uint8** instr, uint32& len);

    // Missing opcodes that were identified
    status_t Execute_GROUP_80(GuestContext& context, const uint8* instr, uint32& len);  // 0x80 - Group with 8-bit immediate
    status_t Execute_IN_AL_DX(GuestContext& context, const uint8* instr, uint32& len); // 0xEC - IN AL, DX
    status_t Execute_OUT_DX_AL(GuestContext& context, const uint8* instr, uint32& len); // 0xEE - OUT DX, AL
    status_t Execute_PUSHAD(GuestContext& context, const uint8* instr, uint32& len);  // 0x60 - Push all registers
    status_t Execute_POPAD(GuestContext& context, const uint8* instr, uint32& len);   // 0x61 - Pop all registers
    status_t Execute_ENTER(GuestContext& context, const uint8* instr, uint32& len);    // 0xC8 - ENTER stack frame
    status_t Execute_LEAVE(GuestContext& context, const uint8* instr, uint32& len);    // 0xC9 - LEAVE stack frame
    status_t Execute_LOOP(GuestContext& context, const uint8* instr, uint32& len);      // 0xE2 - LOOP
    status_t Execute_LOOPE(GuestContext& context, const uint8* instr, uint32& len);     // 0xE1 - LOOPE
    status_t Execute_LOOPNE(GuestContext& context, const uint8* instr, uint32& len);    // 0xE0 - LOOPNE
    status_t Execute_JECXZ(GuestContext& context, const uint8* instr, uint32& len);    // 0xE3 - JECXZ

    // Group opcodes
    status_t Execute_GROUP_83(GuestContext& context, const uint8* instr, uint32& len);
    status_t Execute_GROUP_81(GuestContext& context, const uint8* instr, uint32& len);
    status_t Execute_GROUP_C1(GuestContext& context, const uint8* instr, uint32& len);
    status_t Execute_GROUP_F6(GuestContext& context, const uint8* instr, uint32& len);  // 0xF6 - Group with r/m8
    status_t Execute_GROUP_F7(GuestContext& context, const uint8* instr, uint32& len);  // 0xF7 - Group with r/m32

    // String operations
    status_t Execute_MOVSB(GuestContext& context, const uint8* instr, uint32& len);    // 0xA4
    status_t Execute_MOVSW(GuestContext& context, const uint8* instr, uint32& len);    // 0xA5
    status_t Execute_STOSB(GuestContext& context, const uint8* instr, uint32& len);    // 0xAA
    status_t Execute_STOSW(GuestContext& context, const uint8* instr, uint32& len);    // 0xAB
    status_t Execute_CMPSB(GuestContext& context, const uint8* instr, uint32& len);    // 0xA6
    status_t Execute_CMPSW(GuestContext& context, const uint8* instr, uint32& len);    // 0xA7
    status_t Execute_SCASB(GuestContext& context, const uint8* instr, uint32& len);    // 0xAE
    status_t Execute_SCASW(GuestContext& context, const uint8* instr, uint32& len);    // 0xAF
    status_t Execute_LODSB(GuestContext& context, const uint8* instr, uint32& len);    // 0xAC
    status_t Execute_LODSW(GuestContext& context, const uint8* instr, uint32& len);    // 0xAD

    // Flag operations
    status_t Execute_CLC(GuestContext& context, const uint8* instr, uint32& len);     // 0xF8
    status_t Execute_STC(GuestContext& context, const uint8* instr, uint32& len);     // 0xF9
    status_t Execute_CLI(GuestContext& context, const uint8* instr, uint32& len);     // 0xFA
    status_t Execute_STI(GuestContext& context, const uint8* instr, uint32& len);     // 0xFB
    status_t Execute_CLD(GuestContext& context, const uint8* instr, uint32& len);     // 0xFC
    status_t Execute_STD(GuestContext& context, const uint8* instr, uint32& len);     // 0xFD
    status_t Execute_SAHF(GuestContext& context, const uint8* instr, uint32& len);    // 0x9E
    status_t Execute_LAHF(GuestContext& context, const uint8* instr, uint32& len);    // 0x9F

    // Bit operations
    status_t Execute_TEST(GuestContext& context, const uint8* instr, uint32& len);    // 0x84/0x85
    status_t Execute_NOT(GuestContext& context, const uint8* instr, uint32& len);     // 0xF6/2, 0xF7/2
    status_t Execute_NEG(GuestContext& context, const uint8* instr, uint32& len);     // 0xF6/3, 0xF7/3
    status_t Execute_MUL(GuestContext& context, const uint8* instr, uint32& len);     // 0xF6/4, 0xF7/4
    status_t Execute_IMUL(GuestContext& context, const uint8* instr, uint32& len);    // 0xF6/5, 0xF7/5
    status_t Execute_DIV(GuestContext& context, const uint8* instr, uint32& len);     // 0xF6/6, 0xF7/6
    status_t Execute_IDIV(GuestContext& context, const uint8* instr, uint32& len);    // 0xF6/7, 0xF7/7

    // Logical operations
    status_t Execute_OR(GuestContext& context, const uint8* instr, uint32& len);      // 0x08/0x09/0x0A/0x0B
    status_t Execute_AND(GuestContext& context, const uint8* instr, uint32& len);     // 0x20/0x21/0x22/0x23
    status_t Execute_SHL(GuestContext& context, const uint8* instr, uint32& len);     // 0xC0/4, 0xC1/4, 0xD0/4, 0xD1/4
    status_t Execute_SHR(GuestContext& context, const uint8* instr, uint32& len);     // 0xC0/5, 0xC1/5, 0xD0/5, 0xD1/5
    status_t Execute_SAR(GuestContext& context, const uint8* instr, uint32& len);     // 0xC0/7, 0xC1/7, 0xD0/7, 0xD1/7
    status_t Execute_ROL(GuestContext& context, const uint8* instr, uint32& len);     // 0xC0/0, 0xC1/0, 0xD0/0, 0xD1/0
    status_t Execute_ROR(GuestContext& context, const uint8* instr, uint32& len);     // 0xC0/1, 0xC1/1, 0xD0/1, 0xD1/1
    status_t Execute_RCL(GuestContext& context, const uint8* instr, uint32& len);     // 0xC0/2, 0xC1/2, 0xD0/2, 0xD1/2
    status_t Execute_RCR(GuestContext& context, const uint8* instr, uint32& len);     // 0xC0/3, 0xC1/3, 0xD0/3, 0xD1/3

    // Enhanced syscall logging for debugging write issues
    void LogSyscall(GuestContext& context, uint32_t syscall_num, const char* syscall_name);
    status_t Execute_Syscall_WithLogging(GuestContext& context);

    // Relocation support for ET_DYN binaries
    status_t ApplyRelocations(GuestContext& context);
    status_t ApplyRelocation_TYPE_RELATIVE(GuestContext& context, uint32_t reloc_addr, int32_t addend);

    // Enhanced flag handling
    template<typename T>
    void SetFlags_ADD(X86_32Registers& regs, T result, T op1, T op2, bool is_32bit);
    template<typename T>
    void SetFlags_SUB(X86_32Registers& regs, T result, T op1, T op2, bool is_32bit);
    template<typename T>
    void SetFlags_LOGICAL(X86_32Registers& regs, T result, bool is_32bit);

    // Stack helpers for enhanced initialization
    status_t PushStack32(GuestContext& context, uint32_t value);
    status_t PopStack32(GuestContext& context, uint32_t& value);
    status_t SetupLinuxStack(GuestContext& context, int argc, char** argv, char** envp);

    // Two-byte opcode support (0x0F prefix)
    status_t Execute_TwoByte_Opcode(GuestContext& context, const uint8* instr, uint32& len);
    
    // 0x0F 0x8x - Conditional jumps 32-bit (missing opcodes)
    status_t Execute_JO_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len);  // 0x0F 0x80
    status_t Execute_JNO_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len); // 0x0F 0x81
    status_t Execute_JB_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len);  // 0x0F 0x82
    status_t Execute_JAE_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len); // 0x0F 0x83
    status_t Execute_JE_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len);  // 0x0F 0x84
    status_t Execute_JNE_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len); // 0x0F 0x85
    status_t Execute_JBE_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len); // 0x0F 0x86
    status_t Execute_JA_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len);  // 0x0F 0x87
    status_t Execute_JS_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len);  // 0x0F 0x88
    status_t Execute_JNS_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len); // 0x0F 0x89
    status_t Execute_JP_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len);  // 0x0F 0x8A
    status_t Execute_JNP_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len); // 0x0F 0x8B
    status_t Execute_JL_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len);  // 0x0F 0x8C
    status_t Execute_JGE_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len); // 0x0F 0x8D
    status_t Execute_JLE_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len); // 0x0F 0x8E
    status_t Execute_JG_32_TwoByte(GuestContext& context, const uint8* instr, uint32& len);  // 0x0F 0x8F

    // Debug and verification
    void DebugPrintInstruction(GuestContext& context, const uint8* instr, uint32_t len, const char* opcode_name);
    void VerifyStackAlignment(GuestContext& context);
};