// UserlandVM-HIT Optimized RISC-V Execution Engine
// High-performance RISC-V execution with reduced cycles and optimized paths
// Author: Optimized RISC-V Execution Engine 2026-02-07

#include "ExecutionEngine.h"
#include "PerformanceOptimization.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <SupportDefs.h>

// Forward declaration for GuestContext (assuming it has basic fields)
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

// Optimized RISC-V register file with fast access
struct OptimizedRISCVRegisters {
    uint64_t x[32];  // Fast array access: 0=zero, 1=ra, 2=sp, 3=gp, 4=tp, 5=t0, 6=t1, 7=t2, 8=s0, 9=s1, 10-17=a0-a7, 18-27=s2-s11, 28-31=t3-t6
    uint64_t pc;
    
    // CSR registers
    uint64_t mstatus;
    uint64_t mie;
    uint64_t mtvec;
    uint64_t mscratch;
    uint64_t mepc;
    uint64_t mcause;
    uint64_t mtval;
    uint64_t mip;
    
    // Precomputed register name mapping (eliminates string operations)
    static constexpr const char* REG_NAMES[32] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    
    OptimizedRISCVRegisters() {
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
    
    // Fast inline register access
    inline uint64_t get_reg(int reg) const {
        return x[reg & 0x1F];
    }
    
    inline void set_reg(int reg, uint64_t value) {
        if ((reg & 0x1F) != 0) { // Don't write to zero register
            x[reg & 0x1F] = value;
        }
    }
    
    inline const char* get_reg_name(int reg) const {
        return REG_NAMES[reg & 0x1F];
    }
    
    // Fast CSR access
    inline uint64_t get_csr(int csr_num) const {
        switch (csr_num) {
            case 0x300: return mstatus;
            case 0x304: return mie;
            case 0x305: return mtvec;
            case 0x340: return mscratch;
            case 0x341: return mepc;
            case 0x342: return mcause;
            case 0x343: return mtval;
            case 0x344: return mip;
            default: return 0;
        }
    }
    
    inline void set_csr(int csr_num, uint64_t value) {
        switch (csr_num) {
            case 0x300: mstatus = value; break;
            case 0x304: mie = value; break;
            case 0x305: mtvec = value; break;
            case 0x340: mscratch = value; break;
            case 0x341: mepc = value; break;
            case 0x342: mcause = value; break;
            case 0x343: mtval = value; break;
            case 0x344: mip = value; break;
        }
    }
};

// Optimized RISC-V instruction structure with precomputed fields
struct OptimizedRISCVInstruction {
    uint32_t opcode;      // 6-bit opcode [6:0]
    uint32_t rd;          // 5-bit destination register [11:7]
    uint32_t funct3;      // 3-bit function [14:12]
    uint32_t rs1;         // 5-bit source register 1 [19:15]
    uint32_t rs2;         // 5-bit source register 2 [24:20]
    uint32_t funct7;      // 7-bit function [31:25]
    uint32_t raw;         // Raw 32-bit instruction
    
    // Precomputed immediates
    int32_t imm_i;        // I-type immediate
    int32_t imm_s;        // S-type immediate
    int32_t imm_b;        // B-type immediate
    int32_t imm_u;        // U-type immediate
    int32_t imm_j;        // J-type immediate
    
    // Precomputed flags for fast branching
    uint8_t format;      // Instruction format
    bool is_branch : 1;
    bool is_jump : 1;
    bool is_load : 1;
    bool is_store : 1;
    bool is_alu_imm : 1;
    bool is_alu_reg : 1;
    bool is_system : 1;
    bool changes_pc : 1;
    
    // Cached values for common operations
    uint64_t cached_target;  // For jumps/branches
    bool target_cached;
    
    enum Format {
        R_TYPE = 0, I_TYPE = 1, S_TYPE = 2, B_TYPE = 3, U_TYPE = 4, J_TYPE = 5, UNKNOWN = 6
    };
    
    OptimizedRISCVInstruction() {
        memset(this, 0, sizeof(*this));
        format = UNKNOWN;
        target_cached = false;
    }
};

// Optimized RISC-V instruction decoder with caching
class OptimizedRISCVDecoder {
private:
    InstructionCache<OptimizedRISCVInstruction> instruction_cache;
    
public:
    OptimizedRISCVDecoder() {
        PERF_LOG("Optimized RISC-V decoder initialized");
    }
    
    inline const OptimizedRISCVInstruction* decode(uint32_t instr_raw, uint64_t pc) {
        // Check cache first
        const auto* cached = instruction_cache.lookup(pc);
        if (cached) {
            return cached;
        }
        
        OptimizedRISCVInstruction instr;
        instr.raw = instr_raw;
        instr.opcode = instr_raw & 0x7F;
        instr.rd = (instr_raw >> 7) & 0x1F;
        instr.funct3 = (instr_raw >> 12) & 0x7;
        instr.rs1 = (instr_raw >> 15) & 0x1F;
        instr.rs2 = (instr_raw >> 20) & 0x1F;
        instr.funct7 = (instr_raw >> 25) & 0x7F;
        
        // Fast format detection and immediate extraction
        switch (instr.opcode) {
            case 0x33: // R-type (ALU)
                instr.format = OptimizedRISCVInstruction::R_TYPE;
                instr.is_alu_reg = true;
                break;
                
            case 0x03: // I-type (load)
                instr.format = OptimizedRISCVInstruction::I_TYPE;
                instr.is_load = true;
                instr.imm_i = (int32_t)((int16_t)((instr_raw >> 20) & 0xFFF));
                break;
                
            case 0x13: // I-type (ALU immediate)
                instr.format = OptimizedRISCVInstruction::I_TYPE;
                instr.is_alu_imm = true;
                instr.imm_i = (int32_t)((int16_t)((instr_raw >> 20) & 0xFFF));
                break;
                
            case 0x67: // I-type (jalr)
                instr.format = OptimizedRISCVInstruction::I_TYPE;
                instr.is_jump = true;
                instr.changes_pc = true;
                instr.imm_i = (int32_t)((int16_t)((instr_raw >> 20) & 0xFFF));
                // Cache jump target
                instr.cached_target = 0; // Will be computed at runtime (rs1 + imm)
                instr.target_cached = false;
                break;
                
            case 0x73: // I-type (system)
                instr.format = OptimizedRISCVInstruction::I_TYPE;
                instr.is_system = true;
                instr.imm_i = (int32_t)((int16_t)((instr_raw >> 20) & 0xFFF));
                break;
                
            case 0x23: // S-type (store)
                instr.format = OptimizedRISCVInstruction::S_TYPE;
                instr.is_store = true;
                instr.imm_s = ((int32_t)((instr_raw >> 7) & 0x1F)) |
                           ((int32_t)(((instr_raw >> 25) & 0x7F) << 5));
                break;
                
            case 0x63: // B-type (branch)
                instr.format = OptimizedRISCVInstruction::B_TYPE;
                instr.is_branch = true;
                instr.changes_pc = true;
                instr.imm_b = ((int32_t)((instr_raw >> 7) & 0x1)) |
                           ((int32_t)((instr_raw >> 8) & 0xF) << 1) |
                           ((int32_t)((instr_raw >> 25) & 0x3F) << 5) |
                           ((int32_t)((instr_raw >> 31) & 0x1) << 11);
                instr.imm_b = (instr.imm_b << 20) >> 20; // Sign extend
                // Cache branch target
                instr.cached_target = pc + instr.imm_b;
                instr.target_cached = true;
                break;
                
            case 0x37: // U-type (lui)
                instr.format = OptimizedRISCVInstruction::U_TYPE;
                instr.imm_u = (int32_t)(instr_raw & 0xFFFFF000);
                break;
                
            case 0x17: // U-type (auipc)
                instr.format = OptimizedRISCVInstruction::U_TYPE;
                instr.imm_u = (int32_t)(instr_raw & 0xFFFFF000);
                break;
                
            case 0x6F: // J-type (jal)
                instr.format = OptimizedRISCVInstruction::J_TYPE;
                instr.is_jump = true;
                instr.changes_pc = true;
                instr.imm_j = ((int32_t)((instr_raw >> 7) & 0xFF)) |
                           ((int32_t)((instr_raw >> 25) & 0x1) << 8) |
                           ((int32_t)((instr_raw >> 8) & 0xFF) << 9) |
                           ((int32_t)((instr_raw >> 31) & 0x1) << 19);
                instr.imm_j = (instr.imm_j << 12) >> 12; // Sign extend
                // Cache jump target
                instr.cached_target = pc + instr.imm_j;
                instr.target_cached = true;
                break;
                
            default:
                instr.format = OptimizedRISCVInstruction::UNKNOWN;
                break;
        }
        
        // Cache the instruction
        instruction_cache.insert(pc, instr);
        
        return instruction_cache.lookup(pc);
    }
};

// High-performance RISC-V execution engine
class OptimizedRISCVExecutionEngine : public ExecutionEngine {
private:
    OptimizedRISCVRegisters registers;
    uint8_t* memory;
    uint64_t memory_size;
    bool halted;
    uint64_t instruction_count;
    static constexpr uint64_t MAX_INSTRUCTIONS = 10000000;
    
    OptimizedRISCVDecoder decoder;
    
    // Fast inline memory access with bounds checking
    inline bool check_memory_access(uint64_t addr, size_t size) const {
        return addr + size <= memory_size;
    }
    
    template<typename T>
    inline T read_memory(uint64_t addr) const {
        if (!check_memory_access(addr, sizeof(T))) {
            ERROR_LOG("Memory read out of bounds: 0x%llx", addr);
            return T(0);
        }
        return *reinterpret_cast<const T*>(&memory[addr]);
    }
    
    template<typename T>
    inline bool write_memory(uint64_t addr, T value) {
        if (!check_memory_access(addr, sizeof(T))) {
            ERROR_LOG("Memory write out of bounds: 0x%llx", addr);
            return false;
        }
        *reinterpret_cast<T*>(&memory[addr]) = value;
        return true;
    }
    
    // Optimized instruction execution with fast paths
    inline status_t execute_instruction_fast(const OptimizedRISCVInstruction& instr) {
        PERF_COUNT();
        
        // Fast path based on precomputed flags
        if (instr.is_alu_imm) {
            return execute_alu_imm_fast(instr);
        } else if (instr.is_alu_reg) {
            return execute_alu_reg_fast(instr);
        } else if (instr.is_load) {
            return execute_load_fast(instr);
        } else if (instr.is_store) {
            return execute_store_fast(instr);
        } else if (instr.is_branch) {
            return execute_branch_fast(instr);
        } else if (instr.is_jump) {
            return execute_jump_fast(instr);
        } else if (instr.format == OptimizedRISCVInstruction::U_TYPE) {
            return execute_upper_imm_fast(instr);
        } else if (instr.is_system) {
            return execute_system_fast(instr);
        }
        
        ERROR_LOG("Unknown instruction format: %d", instr.format);
        return B_ERROR;
    }
    
    // Fast ALU immediate execution
    inline status_t execute_alu_imm_fast(const OptimizedRISCVInstruction& instr) {
        uint64_t rs1_val = registers.get_reg(instr.rs1);
        uint64_t result = 0;
        
        switch (instr.funct3) {
            case 0x0: // addi
                result = rs1_val + (uint64_t)instr.imm_i;
                registers.set_reg(instr.rd, result);
                DEBUG_LOG("ADDI %s, %s, %d", registers.get_reg_name(instr.rd), 
                         registers.get_reg_name(instr.rs1), instr.imm_i);
                break;
                
            case 0x4: // xori
                result = rs1_val ^ (uint64_t)instr.imm_i;
                registers.set_reg(instr.rd, result);
                break;
                
            case 0x6: // ori
                result = rs1_val | (uint64_t)instr.imm_i;
                registers.set_reg(instr.rd, result);
                break;
                
            case 0x7: // andi
                result = rs1_val & (uint64_t)instr.imm_i;
                registers.set_reg(instr.rd, result);
                break;
                
            default:
                ERROR_LOG("Unimplemented ALU immediate funct3: 0x%x", instr.funct3);
                return B_ERROR;
        }
        
        return B_OK;
    }
    
    // Fast ALU register execution
    inline status_t execute_alu_reg_fast(const OptimizedRISCVInstruction& instr) {
        uint64_t rs1_val = registers.get_reg(instr.rs1);
        uint64_t rs2_val = registers.get_reg(instr.rs2);
        uint64_t result = 0;
        
        switch (instr.funct3) {
            case 0x0: // add/sub
                if (instr.funct7 == 0x00) { // add
                    result = rs1_val + rs2_val;
                    registers.set_reg(instr.rd, result);
                    DEBUG_LOG("ADD %s, %s, %s", registers.get_reg_name(instr.rd),
                             registers.get_reg_name(instr.rs1), registers.get_reg_name(instr.rs2));
                } else if (instr.funct7 == 0x20) { // sub
                    result = rs1_val - rs2_val;
                    registers.set_reg(instr.rd, result);
                    DEBUG_LOG("SUB %s, %s, %s", registers.get_reg_name(instr.rd),
                             registers.get_reg_name(instr.rs1), registers.get_reg_name(instr.rs2));
                } else {
                    return B_ERROR;
                }
                break;
                
            case 0x7: // and
                result = rs1_val & rs2_val;
                registers.set_reg(instr.rd, result);
                break;
                
            case 0x6: // or
                result = rs1_val | rs2_val;
                registers.set_reg(instr.rd, result);
                break;
                
            case 0x4: // xor
                result = rs1_val ^ rs2_val;
                registers.set_reg(instr.rd, result);
                break;
                
            default:
                ERROR_LOG("Unimplemented ALU register funct3: 0x%x", instr.funct3);
                return B_ERROR;
        }
        
        return B_OK;
    }
    
    // Fast load execution
    inline status_t execute_load_fast(const OptimizedRISCVInstruction& instr) {
        uint64_t addr = registers.get_reg(instr.rs1) + (uint64_t)instr.imm_i;
        uint64_t value = 0;
        
        switch (instr.funct3) {
            case 0x0: // lb (load byte, sign-extended)
                value = (int64_t)(int8_t)read_memory<uint8_t>(addr);
                registers.set_reg(instr.rd, value);
                DEBUG_LOG("LB %s, [0x%llx] = 0x%02llx", registers.get_reg_name(instr.rd), addr, value & 0xFF);
                break;
                
            case 0x1: // lh (load half, sign-extended)
                value = (int64_t)(int16_t)read_memory<uint16_t>(addr);
                registers.set_reg(instr.rd, value);
                DEBUG_LOG("LH %s, [0x%llx] = 0x%04llx", registers.get_reg_name(instr.rd), addr, value & 0xFFFF);
                break;
                
            case 0x2: // lw (load word, sign-extended)
                value = (int64_t)(int32_t)read_memory<uint32_t>(addr);
                registers.set_reg(instr.rd, value);
                DEBUG_LOG("LW %s, [0x%llx] = 0x%08llx", registers.get_reg_name(instr.rd), addr, value & 0xFFFFFFFF);
                break;
                
            case 0x4: // lbu (load byte, zero-extended)
                value = (uint64_t)read_memory<uint8_t>(addr);
                registers.set_reg(instr.rd, value);
                DEBUG_LOG("LBU %s, [0x%llx] = 0x%02llx", registers.get_reg_name(instr.rd), addr, value);
                break;
                
            case 0x5: // lhu (load half, zero-extended)
                value = (uint64_t)read_memory<uint16_t>(addr);
                registers.set_reg(instr.rd, value);
                DEBUG_LOG("LHU %s, [0x%llx] = 0x%04llx", registers.get_reg_name(instr.rd), addr, value);
                break;
                
            default:
                ERROR_LOG("Unimplemented load funct3: 0x%x", instr.funct3);
                return B_ERROR;
        }
        
        return B_OK;
    }
    
    // Fast store execution
    inline status_t execute_store_fast(const OptimizedRISCVInstruction& instr) {
        uint64_t addr = registers.get_reg(instr.rs1) + (uint64_t)instr.imm_s;
        uint64_t value = registers.get_reg(instr.rs2);
        
        switch (instr.funct3) {
            case 0x0: // sb (store byte)
                write_memory<uint8_t>(addr, (uint8_t)value);
                DEBUG_LOG("SB [0x%llx], %s = 0x%02llx", addr, registers.get_reg_name(instr.rs2), value & 0xFF);
                break;
                
            case 0x1: // sh (store half)
                write_memory<uint16_t>(addr, (uint16_t)value);
                DEBUG_LOG("SH [0x%llx], %s = 0x%04llx", addr, registers.get_reg_name(instr.rs2), value & 0xFFFF);
                break;
                
            case 0x2: // sw (store word)
                write_memory<uint32_t>(addr, (uint32_t)value);
                DEBUG_LOG("SW [0x%llx], %s = 0x%08llx", addr, registers.get_reg_name(instr.rs2), value & 0xFFFFFFFF);
                break;
                
            default:
                ERROR_LOG("Unimplemented store funct3: 0x%x", instr.funct3);
                return B_ERROR;
        }
        
        return B_OK;
    }
    
    // Fast branch execution
    inline status_t execute_branch_fast(const OptimizedRISCVInstruction& instr) {
        uint64_t rs1_val = registers.get_reg(instr.rs1);
        uint64_t rs2_val = registers.get_reg(instr.rs2);
        bool taken = false;
        
        switch (instr.funct3) {
            case 0x0: // beq (branch if equal)
                taken = (rs1_val == rs2_val);
                DEBUG_LOG("BEQ %s, %s -> %s", registers.get_reg_name(instr.rs1),
                         registers.get_reg_name(instr.rs2), taken ? "taken" : "not taken");
                break;
                
            case 0x1: // bne (branch if not equal)
                taken = (rs1_val != rs2_val);
                DEBUG_LOG("BNE %s, %s -> %s", registers.get_reg_name(instr.rs1),
                         registers.get_reg_name(instr.rs2), taken ? "taken" : "not taken");
                break;
                
            case 0x4: // blt (branch if less than, signed)
                taken = ((int64_t)rs1_val < (int64_t)rs2_val);
                DEBUG_LOG("BLT %s, %s -> %s", registers.get_reg_name(instr.rs1),
                         registers.get_reg_name(instr.rs2), taken ? "taken" : "not taken");
                break;
                
            case 0x5: // bge (branch if greater or equal, signed)
                taken = ((int64_t)rs1_val >= (int64_t)rs2_val);
                DEBUG_LOG("BGE %s, %s -> %s", registers.get_reg_name(instr.rs1),
                         registers.get_reg_name(instr.rs2), taken ? "taken" : "not taken");
                break;
                
            default:
                ERROR_LOG("Unimplemented branch funct3: 0x%x", instr.funct3);
                return B_ERROR;
        }
        
        if (taken) {
            registers.pc = instr.target_cached ? instr.cached_target : (registers.pc + instr.imm_b);
            DEBUG_LOG("Branch taken to 0x%llx", registers.pc);
        } else {
            registers.pc += 4;
        }
        
        return B_OK;
    }
    
    // Fast jump execution
    inline status_t execute_jump_fast(const OptimizedRISCVInstruction& instr) {
        if (instr.opcode == 0x6F) { // jal (jump and link)
            if (instr.rd != 0) {  // Don't write to zero register
                registers.set_reg(instr.rd, registers.pc + 4);  // Link address
            }
            registers.pc = instr.target_cached ? instr.cached_target : (registers.pc + instr.imm_j);
            DEBUG_LOG("JAL to 0x%llx, link to %s = 0x%llx", registers.pc,
                     registers.get_reg_name(instr.rd), registers.pc + 4);
        } else if (instr.opcode == 0x67) { // jalr (jump and link register)
            uint64_t target = registers.get_reg(instr.rs1) + (uint64_t)instr.imm_i;
            
            if (instr.rd != 0) {  // Don't write to zero register
                registers.set_reg(instr.rd, registers.pc + 4);  // Link address
            }
            
            registers.pc = target & ~1ULL;  // Clear LSB for alignment
            DEBUG_LOG("JALR %s + %d to 0x%llx, link to %s = 0x%llx",
                     registers.get_reg_name(instr.rs1), instr.imm_i, target,
                     registers.get_reg_name(instr.rd), registers.pc + 4);
        }
        
        return B_OK;
    }
    
    // Fast upper immediate execution
    inline status_t execute_upper_imm_fast(const OptimizedRISCVInstruction& instr) {
        uint64_t result;
        
        if (instr.opcode == 0x37) { // lui (load upper immediate)
            result = (uint64_t)instr.imm_u;
            registers.set_reg(instr.rd, result);
            DEBUG_LOG("LUI %s, 0x%x", registers.get_reg_name(instr.rd), instr.imm_u);
        } else if (instr.opcode == 0x17) { // auipc (add upper immediate to PC)
            result = registers.pc + (uint64_t)instr.imm_u;
            registers.set_reg(instr.rd, result);
            DEBUG_LOG("AUIPC %s, 0x%x (PC: 0x%llx)", registers.get_reg_name(instr.rd), instr.imm_u, registers.pc);
        }
        
        return B_OK;
    }
    
    // Fast system instruction execution
    inline status_t execute_system_fast(const OptimizedRISCVInstruction& instr) {
        if (instr.funct3 == 0x0 && instr.rs1 == 0x0 && instr.rd == 0x0) {
            if (instr.funct7 == 0x0) { // ecall
                PRODUCTION_LOG("ECALL - Environment call (system call)");
                // System call handling would go here
                return B_OK;
            } else if (instr.funct7 == 0x1) { // ebreak
                PRODUCTION_LOG("EBREAK - Environment break");
                halted = true;
                return B_OK;
            }
        }
        
        ERROR_LOG("Unimplemented system instruction");
        return B_ERROR;
    }
    
public:
    OptimizedRISCVExecutionEngine(uint8_t* mem, uint64_t mem_size) 
        : memory(mem), memory_size(mem_size), halted(false), instruction_count(0) {
        PRODUCTION_LOG("Optimized RISC-V execution engine created");
        PRODUCTION_LOG("Memory: %p - %p (size: 0x%llx)", memory, memory + memory_size, mem_size);
    }
    
    virtual status_t Run(GuestContext& context) override {
        PRODUCTION_LOG("Starting optimized RISC-V execution");
        PRODUCTION_LOG("Entry point: 0x%llx", context.pc);
        
        // Initialize registers from context
        registers.pc = context.pc;
        registers.x[2] = context.sp;  // sp
        registers.x[3] = context.gp;  // gp
        registers.x[4] = context.tp;  // tp
        
        instruction_count = 0;
        halted = false;
        
        // Main execution loop with optimizations
        while (!halted && instruction_count < MAX_INSTRUCTIONS) {
            if (registers.pc >= memory_size || (registers.pc & 0x3) != 0) {
                ERROR_LOG("PC out of bounds or misaligned: 0x%llx", registers.pc);
                return B_ERROR;
            }
            
            // Fetch instruction
            uint32_t instr_raw = read_memory<uint32_t>(registers.pc);
            
            // Decode instruction with caching
            const auto* instr = decoder.decode(instr_raw, registers.pc);
            if (!instr) {
                ERROR_LOG("Instruction decoding failed at 0x%llx", registers.pc);
                return B_ERROR;
            }
            
            DEBUG_LOG("Executing: 0x%08x at 0x%llx (format=%d, opcode=0x%02x)", 
                     instr_raw, registers.pc, instr->format, instr->opcode);
            
            // Execute instruction with fast path
            status_t result = execute_instruction_fast(*instr);
            if (result != B_OK) {
                ERROR_LOG("Instruction execution failed: %d", result);
                return result;
            }
            
            // Advance PC (unless instruction changed it)
            if (!instr->changes_pc) {
                registers.pc += 4;
            }
            
            instruction_count++;
        }
        
        if (instruction_count >= MAX_INSTRUCTIONS) {
            PRODUCTION_LOG("Maximum instruction limit reached");
        }
        
        PRODUCTION_LOG("Execution completed: %llu instructions", instruction_count);
        
        // Update context with final register state
        context.pc = registers.pc;
        context.sp = registers.x[2];
        context.gp = registers.x[3];
        context.tp = registers.x[4];
        
        // Print performance report
        PERF_REPORT();
        
        return B_OK;
    }
    
    // Performance-optimized getters/setters
    uint64_t get_register_value(const char* reg_name) const {
        // Fast lookup for common registers
        switch (reg_name[0]) {
            case 'x':
                if (reg_name[1] >= '0' && reg_name[1] <= '9') {
                    return registers.get_reg(reg_name[1] - '0');
                }
                break;
            case 'p':
                if (reg_name[1] == 'c') return registers.pc; // pc
                break;
            case 'z':
                if (strcmp(reg_name, "zero") == 0) return 0;
                break;
            case 'r':
                if (strcmp(reg_name, "ra") == 0) return registers.get_reg(1);
                break;
            case 's':
                if (strcmp(reg_name, "sp") == 0) return registers.get_reg(2);
                if (strcmp(reg_name, "s0") == 0) return registers.get_reg(8);
                if (strcmp(reg_name, "s1") == 0) return registers.get_reg(9);
                break;
            case 'g':
                if (strcmp(reg_name, "gp") == 0) return registers.get_reg(3);
                break;
            case 't':
                if (strcmp(reg_name, "tp") == 0) return registers.get_reg(4);
                if (strcmp(reg_name, "t0") == 0) return registers.get_reg(5);
                if (strcmp(reg_name, "t1") == 0) return registers.get_reg(6);
                break;
            case 'a':
                if (reg_name[1] >= '0' && reg_name[1] <= '7') {
                    return registers.get_reg(10 + (reg_name[1] - '0')); // a0-a7
                }
                break;
        }
        
        // Check ABI names array
        for (int i = 0; i < 32; i++) {
            if (strcmp(reg_name, registers.get_reg_name(i)) == 0) {
                return registers.get_reg(i);
            }
        }
        
        ERROR_LOG("Unknown register: %s", reg_name);
        return 0;
    }
    
    void set_register_value(const char* reg_name, uint64_t value) {
        // Fast lookup for common registers
        switch (reg_name[0]) {
            case 'x':
                if (reg_name[1] >= '0' && reg_name[1] <= '9') {
                    registers.set_reg(reg_name[1] - '0', value);
                    return;
                }
                break;
            case 'p':
                if (reg_name[1] == 'c') { registers.pc = value; return; }
                break;
            case 'r':
                if (strcmp(reg_name, "ra") == 0) { registers.set_reg(1, value); return; }
                break;
            case 's':
                if (strcmp(reg_name, "sp") == 0) { registers.set_reg(2, value); return; }
                if (strcmp(reg_name, "s0") == 0) { registers.set_reg(8, value); return; }
                if (strcmp(reg_name, "s1") == 0) { registers.set_reg(9, value); return; }
                break;
            case 'g':
                if (strcmp(reg_name, "gp") == 0) { registers.set_reg(3, value); return; }
                break;
            case 't':
                if (strcmp(reg_name, "tp") == 0) { registers.set_reg(4, value); return; }
                if (strcmp(reg_name, "t0") == 0) { registers.set_reg(5, value); return; }
                if (strcmp(reg_name, "t1") == 0) { registers.set_reg(6, value); return; }
                break;
            case 'a':
                if (reg_name[1] >= '0' && reg_name[1] <= '7') {
                    registers.set_reg(10 + (reg_name[1] - '0'), value);
                    return;
                }
                break;
        }
        
        // Check ABI names array
        for (int i = 0; i < 32; i++) {
            if (strcmp(reg_name, registers.get_reg_name(i)) == 0) {
                registers.set_reg(i, value);
                return;
            }
        }
        
        ERROR_LOG("Unknown register: %s", reg_name);
    }
    
    bool is_halted() const {
        return halted;
    }
    
    void halt() {
        halted = true;
        PRODUCTION_LOG("Execution halted");
    }
    
    void print_status() const {
        printf("[OPT_RISCV] Optimized RISC-V Execution Engine Status:\n");
        printf("  Halted: %s\n", halted ? "Yes" : "No");
        printf("  Instructions executed: %llu\n", instruction_count);
        printf("  PC: 0x%016llx\n", registers.pc);
        printf("  SP (x2): 0x%016llx\n", registers.get_reg(2));
        printf("  GP (x3): 0x%016llx\n", registers.get_reg(3));
        printf("  TP (x4): 0x%016llx\n", registers.get_reg(4));
        printf("  A0 (x10): 0x%016llx\n", registers.get_reg(10));
        printf("  A1 (x11): 0x%016llx\n", registers.get_reg(11));
        printf("  RA (x1): 0x%016llx\n", registers.get_reg(1));
        printf("  Memory range: %p - %p\n", memory, memory + memory_size);
    }
};