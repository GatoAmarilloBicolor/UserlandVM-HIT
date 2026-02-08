/**
 * @file CompleteOpcodeHandler.h
 * @brief Complete opcode handler integration with proper calling conventions
 */

#pragma once

#include "UnifiedDefinitionsCorrected.h"
#include "EnhancedDirectAddressSpace.h"
#include "GuestContext.h"
#include <cstdint>

// Complete opcode handler with proper integration
class CompleteOpcodeHandler {
public:
    // Enhanced opcode handler return type
    struct HandlerResult {
        bool success;
        uint32_t next_eip;
        uint32_t cycles;
        const char* error_message;
    };
    
    // Complete register context
    struct RegisterContext {
        uint32_t eax, ebx, ecx, edx;
        uint32_t esi, edi, ebp, esp;
        uint32_t eip;
        uint32_t eflags;
        uint32_t cs, ds, es, fs, gs, ss;
    };
    
private:
    EnhancedDirectAddressSpace* fAddressSpace;
    RegisterContext fRegisters;
    bool fTraceEnabled;
    
public:
    CompleteOpcodeHandler(EnhancedDirectAddressSpace* addressSpace);
    virtual ~CompleteOpcodeHandler();
    
    // Core execution interface
    HandlerResult ExecuteInstruction(uint8_t* instruction, uint32_t instruction_length);
    HandlerResult ExecuteOpcode(uint8_t opcode, const uint8_t* operands);
    
    // Register access
    uint32_t GetRegister(int reg) const;
    void SetRegister(int reg, uint32_t value);
    uint32_t GetFlag(uint32_t flag) const;
    void SetFlag(uint32_t flag, bool set);
    void UpdateFlags(uint32_t result, bool is_arithmetic = true);
    
    // Memory access helpers
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
    
    // Complete opcode implementations
    HandlerResult Handle0F_Prefix(const uint8_t* operands);
    HandlerResult HandleGroup80(const uint8_t* operands); // ADD/OR/ADC/SBB/AND/SUB/XOR/CMP
    HandlerResult HandleGroup81(const uint8_t* operands); // Same as 80 but with 32-bit immediate
    HandlerResult HandleGroup83(const uint8_t* operands); // Same as 80 but with sign-extended 8-bit immediate
    HandlerResult HandleIN(const uint8_t* operands);     // 0xEC - IN AL, DX
    HandlerResult HandleOUT(const uint8_t* operands);    // 0xEE - OUT DX, AL
    
    // Arithmetic operations (complete implementations)
    uint32_t Add32(uint32_t a, uint32_t b);
    uint32_t Sub32(uint32_t a, uint32_t b);
    uint32_t And32(uint32_t a, uint32_t b);
    uint32_t Or32(uint32_t a, uint32_t b);
    uint32_t Xor32(uint32_t a, uint32_t b);
    uint32_t Cmp32(uint32_t a, uint32_t b);
    uint32_t Adc32(uint32_t a, uint32_t b);
    uint32_t Sbb32(uint32_t a, uint32_t b);
    
    // Jump operations (0x0F series)
    HandlerResult HandleConditionalJump(const uint8_t* operands, uint8_t condition);
    
    // ModR/M parsing (complete)
    struct ModRM {
        uint8_t mod;
        uint8_t reg;
        uint8_t rm;
        uint32_t address;
        bool has_displacement;
        int32_t displacement;
    };
    
    ModRM ParseModRM(const uint8_t* instruction, uint32_t& offset);
    uint32_t GetModRMAddress(const ModRM& modrm);
    
    // Debug and tracing
    void EnableTracing(bool enable) { fTraceEnabled = enable; }
    void DumpRegisters();
    void DumpInstruction(const uint8_t* instruction, uint32_t length);
    
    // Execution state
    void Reset();
    void SetContext(const RegisterContext& context);
    RegisterContext GetContext() const { return fRegisters; }

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
    
    // Condition codes for conditional jumps
    enum Condition {
        COND_O   = 0x00,  // Overflow
        COND_NO  = 0x01,  // Not overflow
        COND_B   = 0x02,  // Below / Carry
        COND_NB  = 0x03,  // Not below / Not carry
        COND_Z   = 0x04,  // Zero / Equal
        COND_NZ  = 0x05,  // Not zero / Not equal
        COND_BE  = 0x06,  // Below or equal
        COND_NBE = 0x07,  // Not below or equal
        COND_S   = 0x08,  // Sign
        COND_NS  = 0x09,  // Not sign
        COND_P   = 0x0A,  // Parity
        COND_NP  = 0x0B,  // Not parity
        COND_L   = 0x0C,  // Less
        COND_NL  = 0x0D,  // Not less
        COND_LE  = 0x0E,  // Less or equal
        COND_NLE = 0x0F   // Not less or equal
    };
    
    // Utility functions
    bool TestCondition(Condition cond);
    void LogTrace(const char* format, ...);
    HandlerResult CreateResult(bool success, uint32_t next_eip = 0, 
                            uint32_t cycles = 1, const char* error = nullptr);
};