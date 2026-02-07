// UserlandVM-HIT Real RISC-V Execution Engine Implementation
// Implements actual RISC-V instruction decoding and execution
// Author: Real RISC-V Execution Engine 2026-02-07

#include "ExecutionEngine.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <SupportDefs.h>

// Forward declaration for GuestContext
struct GuestContext {
    uint64_t pc;
    uint64_t sp;
    uint64_t gp;
    uint64_t tp;
    uint64_t t0;
    uint64_t t1;
    uint64_t t2;
    uint64_t s0;
    uint64_t s1;
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t a4;
    uint64_t a5;
    uint64_t a6;
    uint64_t a7;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
    uint64_t t3;
    uint64_t t4;
    uint64_t t5;
    uint64_t t6;
};

// RISC-V register structure
struct RISCVRegisters {
    uint64_t x[32];  // x0-x31 registers (x0 is hardwired to 0)
    uint64_t pc;     // Program counter
    
    // CSR registers (Control and Status Registers)
    uint64_t mstatus;
    uint64_t mie;
    uint64_t mtvec;
    uint64_t mscratch;
    uint64_t mepc;
    uint64_t mcause;
    uint64_t mtval;
    uint64_t mip;
    
    RISCVRegisters() {
        memset(this, 0, sizeof(*this));
        pc = 0;
        x[2] = 0x7FFFF000; // sp (stack pointer) default
        x[3] = 0x10000000; // gp (global pointer)
        x[4] = 0;          // tp (thread pointer)
        
        // Initialize CSRs
        mstatus = 0;
        mie = 0;
        mtvec = 0;
        mscratch = 0;
        mepc = 0;
        mcause = 0;
        mtval = 0;
        mip = 0;
    }
    
    uint64_t GetRegister(int reg) const {
        if (reg >= 0 && reg < 32) {
            return x[reg];
        }
        return 0;
    }
    
    void SetRegister(int reg, uint64_t value) {
        if (reg >= 0 && reg < 32) {
            x[reg] = value;
        }
    }
    
    const char* GetRegisterName(int reg) const {
        static const char* names[32] = {
            "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
            "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
            "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
            "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
        };
        return (reg >= 0 && reg < 32) ? names[reg] : "unknown";
    }
};

// RISC-V instruction decoder
class RISCVDecoder {
public:
    struct Instruction {
        uint32_t opcode;      // 6-bit opcode [6:0]
        uint32_t rd;          // 5-bit destination register [11:7]
        uint32_t funct3;      // 3-bit function [14:12]
        uint32_t rs1;         // 5-bit source register 1 [19:15]
        uint32_t rs2;         // 5-bit source register 2 [24:20]
        uint32_t funct7;      // 7-bit function [31:25]
        uint32_t raw;         // Raw 32-bit instruction
        
        // Immediate values for different formats
        int32_t imm_i;        // I-type immediate
        int32_t imm_s;        // S-type immediate
        int32_t imm_b;        // B-type immediate
        int32_t imm_u;        // U-type immediate
        int32_t imm_j;        // J-type immediate
        
        // Instruction format
        enum Format {
            R_TYPE, I_TYPE, S_TYPE, B_TYPE, U_TYPE, J_TYPE, UNKNOWN
        } format;
        
        Instruction() {
            memset(this, 0, sizeof(*this));
            format = UNKNOWN;
        }
    };
    
    static Instruction Decode(uint32_t instr) {
        Instruction dec;
        dec.raw = instr;
        dec.opcode = instr & 0x7F;
        dec.rd = (instr >> 7) & 0x1F;
        dec.funct3 = (instr >> 12) & 0x7;
        dec.rs1 = (instr >> 15) & 0x1F;
        dec.rs2 = (instr >> 20) & 0x1F;
        dec.funct7 = (instr >> 25) & 0x7F;
        
        // Determine format and extract immediates
        switch (dec.opcode) {
            case 0x33: // R-type (ALU)
                dec.format = Instruction::R_TYPE;
                break;
                
            case 0x03: // I-type (load)
            case 0x13: // I-type (ALU immediate)
            case 0x67: // I-type (jalr)
            case 0x73: // I-type (system)
                dec.format = Instruction::I_TYPE;
                dec.imm_i = (int32_t)((int16_t)((instr >> 20) & 0xFFF));
                break;
                
            case 0x23: // S-type (store)
                dec.format = Instruction::S_TYPE;
                dec.imm_s = ((int32_t)((instr >> 7) & 0x1F)) |
                           ((int32_t)(((instr >> 25) & 0x7F) << 5));
                break;
                
            case 0x63: // B-type (branch)
                dec.format = Instruction::B_TYPE;
                dec.imm_b = ((int32_t)((instr >> 7) & 0x1)) |
                           ((int32_t)((instr >> 8) & 0xF) << 1) |
                           ((int32_t)((instr >> 25) & 0x3F) << 5) |
                           ((int32_t)((instr >> 31) & 0x1) << 11);
                dec.imm_b = (dec.imm_b << 20) >> 20; // Sign extend
                break;
                
            case 0x37: // U-type (lui)
            case 0x17: // U-type (auipc)
                dec.format = Instruction::U_TYPE;
                dec.imm_u = (int32_t)(instr & 0xFFFFF000);
                break;
                
            case 0x6F: // J-type (jal)
                dec.format = Instruction::J_TYPE;
                dec.imm_j = ((int32_t)((instr >> 7) & 0xFF)) |
                           ((int32_t)((instr >> 25) & 0x1) << 8) |
                           ((int32_t)((instr >> 8) & 0xFF) << 9) |
                           ((int32_t)((instr >> 31) & 0x1) << 19);
                dec.imm_j = (dec.imm_j << 12) >> 12; // Sign extend
                break;
                
            default:
                dec.format = Instruction::UNKNOWN;
                break;
        }
        
        return dec;
    }
};

// Real RISC-V execution engine
class RealRISCVExecutionEngine : public ExecutionEngine {
private:
    RISCVRegisters registers;
    uint8_t* memory;
    uint64_t memory_size;
    bool halted;
    uint64_t instruction_count;
    static const uint64_t MAX_INSTRUCTIONS = 10000000;
    
public:
    RealRISCVExecutionEngine(uint8_t* mem, uint64_t mem_size) 
        : memory(mem), memory_size(mem_size), halted(false), instruction_count(0) {
        printf("[RISCV_EXEC] Real RISC-V execution engine created\n");
        printf("[RISCV_EXEC] Memory: %p - %p (size: 0x%llx)\n", 
               memory, memory + memory_size, mem_size);
    }
    
    virtual status_t Run(GuestContext& context) override {
        printf("[RISCV_EXEC] Starting real RISC-V execution\n");
        printf("[RISCV_EXEC] Entry point: 0x%llx\n", context.pc);
        
        // Initialize registers from context
        registers.pc = context.pc;
        registers.x[2] = context.sp;  // sp
        registers.x[3] = context.gp;  // gp
        registers.x[4] = context.tp;  // tp
        
        instruction_count = 0;
        halted = false;
        
        // Main execution loop
        while (!halted && instruction_count < MAX_INSTRUCTIONS) {
            if (registers.pc >= memory_size || (registers.pc & 0x3) != 0) {
                printf("[RISCV_EXEC] PC out of bounds or misaligned: 0x%llx\n", registers.pc);
                return B_ERROR;
            }
            
            // Fetch instruction
            uint32_t instr_raw = *(uint32_t*)&memory[registers.pc];
            
            // Decode instruction
            RISCVDecoder::Instruction instr = RISCVDecoder::Decode(instr_raw);
            
            printf("[RISCV_EXEC] Executing: 0x%08x at 0x%llx (format=%d, opcode=0x%02x)\n", 
                   instr_raw, registers.pc, instr.format, instr.opcode);
            
            // Execute instruction
            status_t result = ExecuteInstruction(instr);
            if (result != B_OK) {
                printf("[RISCV_EXEC] Instruction execution failed: %d\n", result);
                return result;
            }
            
            // Advance PC (most instructions are 4 bytes)
            if (instr.format != RISCVDecoder::Instruction::J_TYPE && instr.format != RISCVDecoder::Instruction::B_TYPE) {
                registers.pc += 4;
            }
            
            instruction_count++;
        }
        
        if (instruction_count >= MAX_INSTRUCTIONS) {
            printf("[RISCV_EXEC] Maximum instruction limit reached\n");
        }
        
        printf("[RISCV_EXEC] Execution completed: %llu instructions\n", instruction_count);
        
        // Update context with final register state
        context.pc = registers.pc;
        context.sp = registers.x[2];
        context.gp = registers.x[3];
        context.tp = registers.x[4];
        
        return B_OK;
    }
    
    status_t ExecuteInstruction(const RISCVDecoder::Instruction& instr) {
        switch (instr.opcode) {
            // ALU operations (I-type)
            case 0x13:
                return ExecuteALUImmediate(instr);
                
            // ALU operations (R-type)
            case 0x33:
                return ExecuteALURegister(instr);
                
            // Load operations (I-type)
            case 0x03:
                return ExecuteLoad(instr);
                
            // Store operations (S-type)
            case 0x23:
                return ExecuteStore(instr);
                
            // Branch operations (B-type)
            case 0x63:
                return ExecuteBranch(instr);
                
            // Jump operations (J-type, I-type)
            case 0x6F: // jal
                return ExecuteJump(instr);
            case 0x67: // jalr
                return ExecuteJumpRegister(instr);
                
            // Upper immediate operations (U-type)
            case 0x37: // lui
                return ExecuteUpperImmediate(instr);
            case 0x17: // auipc
                return ExecuteUpperImmediatePC(instr);
                
            // System operations (I-type)
            case 0x73:
                return ExecuteSystem(instr);
                
            default:
                printf("[RISCV_EXEC] Unimplemented opcode: 0x%02x\n", instr.opcode);
                return B_ERROR;
        }
    }
    
    status_t ExecuteALUImmediate(const RISCVDecoder::Instruction& instr) {
        uint64_t rs1_val = registers.GetRegister(instr.rs1);
        uint64_t result = 0;
        
        switch (instr.funct3) {
            case 0x0: // addi
                result = rs1_val + (uint64_t)instr.imm_i;
                printf("[RISCV_EXEC] ADDI %s, %s, %d (0x%llx + %d = 0x%llx)\n", 
                       registers.GetRegisterName(instr.rd), 
                       registers.GetRegisterName(instr.rs1),
                       instr.imm_i, rs1_val, instr.imm_i, result);
                break;
                
            case 0x4: // xori
                result = rs1_val ^ (uint64_t)instr.imm_i;
                printf("[RISCV_EXEC] XORI %s, %s, %d\n", 
                       registers.GetRegisterName(instr.rd), 
                       registers.GetRegisterName(instr.rs1), instr.imm_i);
                break;
                
            case 0x6: // ori
                result = rs1_val | (uint64_t)instr.imm_i;
                printf("[RISCV_EXEC] ORI %s, %s, %d\n", 
                       registers.GetRegisterName(instr.rd), 
                       registers.GetRegisterName(instr.rs1), instr.imm_i);
                break;
                
            case 0x7: // andi
                result = rs1_val & (uint64_t)instr.imm_i;
                printf("[RISCV_EXEC] ANDI %s, %s, %d\n", 
                       registers.GetRegisterName(instr.rd), 
                       registers.GetRegisterName(instr.rs1), instr.imm_i);
                break;
                
            default:
                printf("[RISCV_EXEC] Unimplemented ALU immediate funct3: 0x%x\n", instr.funct3);
                return B_ERROR;
        }
        
        if (instr.rd != 0) {  // Don't write to zero register
            registers.SetRegister(instr.rd, result);
        }
        
        return B_OK;
    }
    
    status_t ExecuteALURegister(const RISCVDecoder::Instruction& instr) {
        uint64_t rs1_val = registers.GetRegister(instr.rs1);
        uint64_t rs2_val = registers.GetRegister(instr.rs2);
        uint64_t result = 0;
        
        switch (instr.funct3) {
            case 0x0: // add/sub
                if (instr.funct7 == 0x00) { // add
                    result = rs1_val + rs2_val;
                    printf("[RISCV_EXEC] ADD %s, %s, %s\n", 
                           registers.GetRegisterName(instr.rd), 
                           registers.GetRegisterName(instr.rs1),
                           registers.GetRegisterName(instr.rs2));
                } else if (instr.funct7 == 0x20) { // sub
                    result = rs1_val - rs2_val;
                    printf("[RISCV_EXEC] SUB %s, %s, %s\n", 
                           registers.GetRegisterName(instr.rd), 
                           registers.GetRegisterName(instr.rs1),
                           registers.GetRegisterName(instr.rs2));
                } else {
                    printf("[RISCV_EXEC] Unimplemented ALU funct7: 0x%02x\n", instr.funct7);
                    return B_ERROR;
                }
                break;
                
            case 0x7: // and
                result = rs1_val & rs2_val;
                printf("[RISCV_EXEC] AND %s, %s, %s\n", 
                       registers.GetRegisterName(instr.rd), 
                       registers.GetRegisterName(instr.rs1),
                       registers.GetRegisterName(instr.rs2));
                break;
                
            case 0x6: // or
                result = rs1_val | rs2_val;
                printf("[RISCV_EXEC] OR %s, %s, %s\n", 
                       registers.GetRegisterName(instr.rd), 
                       registers.GetRegisterName(instr.rs1),
                       registers.GetRegisterName(instr.rs2));
                break;
                
            case 0x4: // xor
                result = rs1_val ^ rs2_val;
                printf("[RISCV_EXEC] XOR %s, %s, %s\n", 
                       registers.GetRegisterName(instr.rd), 
                       registers.GetRegisterName(instr.rs1),
                       registers.GetRegisterName(instr.rs2));
                break;
                
            default:
                printf("[RISCV_EXEC] Unimplemented ALU register funct3: 0x%x\n", instr.funct3);
                return B_ERROR;
        }
        
        if (instr.rd != 0) {  // Don't write to zero register
            registers.SetRegister(instr.rd, result);
        }
        
        return B_OK;
    }
    
    status_t ExecuteLoad(const RISCVDecoder::Instruction& instr) {
        uint64_t addr = registers.GetRegister(instr.rs1) + (uint64_t)instr.imm_i;
        uint64_t value = 0;
        
        if (addr >= memory_size) {
            printf("[RISCV_EXEC] Load address out of bounds: 0x%llx\n", addr);
            return B_ERROR;
        }
        
        switch (instr.funct3) {
            case 0x0: // lb (load byte, sign-extended)
                value = (int64_t)(int8_t)memory[addr];
                printf("[RISCV_EXEC] LB %s, [0x%llx] = 0x%02llx\n", 
                       registers.GetRegisterName(instr.rd), addr, value & 0xFF);
                break;
                
            case 0x1: // lh (load half, sign-extended)
                if (addr + 2 <= memory_size) {
                    value = (int64_t)(int16_t)(*(uint16_t*)&memory[addr]);
                    printf("[RISCV_EXEC] LH %s, [0x%llx] = 0x%04llx\n", 
                           registers.GetRegisterName(instr.rd), addr, value & 0xFFFF);
                } else {
                    return B_ERROR;
                }
                break;
                
            case 0x2: // lw (load word, sign-extended)
                if (addr + 4 <= memory_size) {
                    value = (int64_t)(int32_t)(*(uint32_t*)&memory[addr]);
                    printf("[RISCV_EXEC] LW %s, [0x%llx] = 0x%08llx\n", 
                           registers.GetRegisterName(instr.rd), addr, value & 0xFFFFFFFF);
                } else {
                    return B_ERROR;
                }
                break;
                
            case 0x4: // lbu (load byte, zero-extended)
                value = (uint64_t)memory[addr];
                printf("[RISCV_EXEC] LBU %s, [0x%llx] = 0x%02llx\n", 
                       registers.GetRegisterName(instr.rd), addr, value);
                break;
                
            case 0x5: // lhu (load half, zero-extended)
                if (addr + 2 <= memory_size) {
                    value = (uint64_t)*(uint16_t*)&memory[addr];
                    printf("[RISCV_EXEC] LHU %s, [0x%llx] = 0x%04llx\n", 
                           registers.GetRegisterName(instr.rd), addr, value);
                } else {
                    return B_ERROR;
                }
                break;
                
            default:
                printf("[RISCV_EXEC] Unimplemented load funct3: 0x%x\n", instr.funct3);
                return B_ERROR;
        }
        
        if (instr.rd != 0) {  // Don't write to zero register
            registers.SetRegister(instr.rd, value);
        }
        
        return B_OK;
    }
    
    status_t ExecuteStore(const RISCVDecoder::Instruction& instr) {
        uint64_t addr = registers.GetRegister(instr.rs1) + (uint64_t)instr.imm_s;
        uint64_t value = registers.GetRegister(instr.rs2);
        
        if (addr >= memory_size) {
            printf("[RISCV_EXEC] Store address out of bounds: 0x%llx\n", addr);
            return B_ERROR;
        }
        
        switch (instr.funct3) {
            case 0x0: // sb (store byte)
                memory[addr] = (uint8_t)value;
                printf("[RISCV_EXEC] SB [0x%llx], %s = 0x%02llx\n", 
                       addr, registers.GetRegisterName(instr.rs2), value & 0xFF);
                break;
                
            case 0x1: // sh (store half)
                if (addr + 2 <= memory_size) {
                    *(uint16_t*)&memory[addr] = (uint16_t)value;
                    printf("[RISCV_EXEC] SH [0x%llx], %s = 0x%04llx\n", 
                           addr, registers.GetRegisterName(instr.rs2), value & 0xFFFF);
                } else {
                    return B_ERROR;
                }
                break;
                
            case 0x2: // sw (store word)
                if (addr + 4 <= memory_size) {
                    *(uint32_t*)&memory[addr] = (uint32_t)value;
                    printf("[RISCV_EXEC] SW [0x%llx], %s = 0x%08llx\n", 
                           addr, registers.GetRegisterName(instr.rs2), value & 0xFFFFFFFF);
                } else {
                    return B_ERROR;
                }
                break;
                
            default:
                printf("[RISCV_EXEC] Unimplemented store funct3: 0x%x\n", instr.funct3);
                return B_ERROR;
        }
        
        return B_OK;
    }
    
    status_t ExecuteBranch(const RISCVDecoder::Instruction& instr) {
        uint64_t rs1_val = registers.GetRegister(instr.rs1);
        uint64_t rs2_val = registers.GetRegister(instr.rs2);
        bool taken = false;
        
        switch (instr.funct3) {
            case 0x0: // beq (branch if equal)
                taken = (rs1_val == rs2_val);
                printf("[RISCV_EXEC] BEQ %s, %s -> %s\n", 
                       registers.GetRegisterName(instr.rs1),
                       registers.GetRegisterName(instr.rs2),
                       taken ? "taken" : "not taken");
                break;
                
            case 0x1: // bne (branch if not equal)
                taken = (rs1_val != rs2_val);
                printf("[RISCV_EXEC] BNE %s, %s -> %s\n", 
                       registers.GetRegisterName(instr.rs1),
                       registers.GetRegisterName(instr.rs2),
                       taken ? "taken" : "not taken");
                break;
                
            case 0x4: // blt (branch if less than, signed)
                taken = ((int64_t)rs1_val < (int64_t)rs2_val);
                printf("[RISCV_EXEC] BLT %s, %s -> %s\n", 
                       registers.GetRegisterName(instr.rs1),
                       registers.GetRegisterName(instr.rs2),
                       taken ? "taken" : "not taken");
                break;
                
            case 0x5: // bge (branch if greater or equal, signed)
                taken = ((int64_t)rs1_val >= (int64_t)rs2_val);
                printf("[RISCV_EXEC] BGE %s, %s -> %s\n", 
                       registers.GetRegisterName(instr.rs1),
                       registers.GetRegisterName(instr.rs2),
                       taken ? "taken" : "not taken");
                break;
                
            default:
                printf("[RISCV_EXEC] Unimplemented branch funct3: 0x%x\n", instr.funct3);
                return B_ERROR;
        }
        
        if (taken) {
            registers.pc = registers.pc + (uint64_t)instr.imm_b;
            printf("[RISCV_EXEC] Branch taken to 0x%llx\n", registers.pc);
        } else {
            registers.pc += 4;
        }
        
        return B_OK;
    }
    
    status_t ExecuteJump(const RISCVDecoder::Instruction& instr) {
        // jal (jump and link)
        uint64_t target = registers.pc + (uint64_t)instr.imm_j;
        
        if (instr.rd != 0) {  // Don't write to zero register
            registers.SetRegister(instr.rd, registers.pc + 4);  // Link address
        }
        
        registers.pc = target;
        printf("[RISCV_EXEC] JAL to 0x%llx, link to %s = 0x%llx\n", 
               target, registers.GetRegisterName(instr.rd), registers.pc + 4);
        
        return B_OK;
    }
    
    status_t ExecuteJumpRegister(const RISCVDecoder::Instruction& instr) {
        // jalr (jump and link register)
        uint64_t target = registers.GetRegister(instr.rs1) + (uint64_t)instr.imm_i;
        
        if (instr.rd != 0) {  // Don't write to zero register
            registers.SetRegister(instr.rd, registers.pc + 4);  // Link address
        }
        
        registers.pc = target & ~1ULL;  // Clear LSB for alignment
        printf("[RISCV_EXEC] JALR %s + %d to 0x%llx, link to %s = 0x%llx\n", 
               registers.GetRegisterName(instr.rs1), instr.imm_i, target,
               registers.GetRegisterName(instr.rd), registers.pc + 4);
        
        return B_OK;
    }
    
    status_t ExecuteUpperImmediate(const RISCVDecoder::Instruction& instr) {
        // lui (load upper immediate)
        uint64_t result = (uint64_t)instr.imm_u;
        
        if (instr.rd != 0) {  // Don't write to zero register
            registers.SetRegister(instr.rd, result);
        }
        
        printf("[RISCV_EXEC] LUI %s, 0x%x\n", 
               registers.GetRegisterName(instr.rd), instr.imm_u);
        
        return B_OK;
    }
    
    status_t ExecuteUpperImmediatePC(const RISCVDecoder::Instruction& instr) {
        // auipc (add upper immediate to PC)
        uint64_t result = registers.pc + (uint64_t)instr.imm_u;
        
        if (instr.rd != 0) {  // Don't write to zero register
            registers.SetRegister(instr.rd, result);
        }
        
        printf("[RISCV_EXEC] AUIPC %s, 0x%x (PC: 0x%llx)\n", 
               registers.GetRegisterName(instr.rd), instr.imm_u, registers.pc);
        
        return B_OK;
    }
    
    status_t ExecuteSystem(const RISCVDecoder::Instruction& instr) {
        if (instr.funct3 == 0x0 && instr.rs1 == 0x0 && instr.rd == 0x0) {
            if (instr.funct7 == 0x0) { // ecall
                printf("[RISCV_EXEC] ECALL - Environment call (system call)\n");
                // System call handling would go here
                return B_OK;
            } else if (instr.funct7 == 0x1) { // ebreak
                printf("[RISCV_EXEC] EBREAK - Environment break\n");
                halted = true;
                return B_OK;
            }
        }
        
        printf("[RISCV_EXEC] Unimplemented system instruction\n");
        return B_ERROR;
    }
    
    // Getters and setters for external access
    uint64_t GetRegisterValue(const char* reg_name) const {
        if (strcmp(reg_name, "pc") == 0) return registers.pc;
        if (strncmp(reg_name, "x", 1) == 0) {
            int reg_num = atoi(reg_name + 1);
            if (reg_num >= 0 && reg_num < 32) {
                return registers.GetRegister(reg_num);
            }
        }
        // Check ABI names
        for (int i = 0; i < 32; i++) {
            if (strcmp(reg_name, registers.GetRegisterName(i)) == 0) {
                return registers.GetRegister(i);
            }
        }
        return 0;
    }
    
    void SetRegisterValue(const char* reg_name, uint64_t value) {
        if (strcmp(reg_name, "pc") == 0) registers.pc = value;
        else if (strncmp(reg_name, "x", 1) == 0) {
            int reg_num = atoi(reg_name + 1);
            if (reg_num >= 0 && reg_num < 32) {
                registers.SetRegister(reg_num, value);
            }
        }
        // Check ABI names
        for (int i = 0; i < 32; i++) {
            if (strcmp(reg_name, registers.GetRegisterName(i)) == 0) {
                registers.SetRegister(i, value);
                break;
            }
        }
    }
    
    bool IsHalted() const {
        return halted;
    }
    
    void Halt() {
        halted = true;
        printf("[RISCV_EXEC] Execution halted\n");
    }
    
    void PrintStatus() const {
        printf("[RISCV_EXEC] Real RISC-V Execution Engine Status:\n");
        printf("  Halted: %s\n", halted ? "Yes" : "No");
        printf("  Instructions executed: %llu\n", instruction_count);
        printf("  PC: 0x%016llx\n", registers.pc);
        printf("  SP (x2): 0x%016llx\n", registers.GetRegister(2));
        printf("  GP (x3): 0x%016llx\n", registers.GetRegister(3));
        printf("  TP (x4): 0x%016llx\n", registers.GetRegister(4));
        printf("  A0 (x10): 0x%016llx\n", registers.GetRegister(10));
        printf("  A1 (x11): 0x%016llx\n", registers.GetRegister(11));
        printf("  RA (x1): 0x%016llx\n", registers.GetRegister(1));
        printf("  Memory range: %p - %p\n", memory, memory + memory_size);
    }
};