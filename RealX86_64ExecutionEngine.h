// UserlandVM-HIT Real x86-64 Execution Engine Implementation
// Implements actual x86-64 instruction decoding and execution
// Author: Real x86-64 Execution Engine 2026-02-07

#include "ExecutionEngine.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <SupportDefs.h>

// Forward declaration for GuestContext
struct GuestContext {
    uint64_t rip;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rflags;
};

// x86-64 register structure
struct X86_64Registers {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip;
    uint64_t rflags;
    
    // Segment registers (16-bit)
    uint16_t cs, ds, es, fs, gs, ss;
    
    X86_64Registers() {
        memset(this, 0, sizeof(*this));
        rip = 0;
        rsp = 0x7FFFF000; // Default stack top
        rflags = 0x2;     // Reserved bit set
    }
    
    uint64_t GetRegister(int reg) const {
        switch (reg & 0xF) {
            case 0: return rax;
            case 1: return rcx;
            case 2: return rdx;
            case 3: return rbx;
            case 4: return rsp;
            case 5: return rbp;
            case 6: return rsi;
            case 7: return rdi;
            case 8: return r8;
            case 9: return r9;
            case 10: return r10;
            case 11: return r11;
            case 12: return r12;
            case 13: return r13;
            case 14: return r14;
            case 15: return r15;
            default: return 0;
        }
    }
    
    void SetRegister(int reg, uint64_t value) {
        switch (reg & 0xF) {
            case 0: rax = value; break;
            case 1: rcx = value; break;
            case 2: rdx = value; break;
            case 3: rbx = value; break;
            case 4: rsp = value; break;
            case 5: rbp = value; break;
            case 6: rsi = value; break;
            case 7: rdi = value; break;
            case 8: r8 = value; break;
            case 9: r9 = value; break;
            case 10: r10 = value; break;
            case 11: r11 = value; break;
            case 12: r12 = value; break;
            case 13: r13 = value; break;
            case 14: r14 = value; break;
            case 15: r15 = value; break;
        }
    }
    
    uint32_t GetRegister32(int reg) const {
        return (uint32_t)GetRegister(reg);
    }
    
    uint16_t GetRegister16(int reg) const {
        return (uint16_t)GetRegister(reg);
    }
    
    uint8_t GetRegister8(int reg) const {
        return (uint8_t)GetRegister(reg);
    }
    
    void SetRegister32(int reg, uint32_t value) {
        SetRegister(reg, (uint64_t)value);
    }
    
    void SetRegister16(int reg, uint16_t value) {
        SetRegister(reg, (uint64_t)value);
    }
    
    void SetRegister8(int reg, uint8_t value) {
        SetRegister(reg, (uint64_t)value);
    }
};

// x86-64 instruction decoder
class X86_64Decoder {
public:
    struct Instruction {
        uint8_t opcode;
        uint8_t modrm;
        uint8_t sib;
        uint64_t displacement;
        uint64_t immediate;
        uint8_t length;
        bool has_modrm;
        bool has_sib;
        bool has_displacement;
        bool has_immediate;
        uint8_t operand_size;
        uint8_t address_size;
        
        Instruction() {
            memset(this, 0, sizeof(*this));
            operand_size = 64;  // Default to 64-bit
            address_size = 64;  // Default to 64-bit
        }
    };
    
    static Instruction Decode(const uint8_t* code, uint64_t rip) {
        Instruction instr;
        uint64_t pos = 0;
        
        // Handle prefixes
        while (true) {
            uint8_t prefix = code[pos];
            
            // Operand size override
            if (prefix == 0x66) {
                instr.operand_size = 16;
                pos++;
                continue;
            }
            
            // Address size override
            if (prefix == 0x67) {
                instr.address_size = 32;
                pos++;
                continue;
            }
            
            // REX prefix (x86-64 specific)
            if ((prefix & 0xF0) == 0x40) {
                pos++;
                continue;
            }
            
            // Segment prefixes
            if (prefix == 0x26 || prefix == 0x2E || prefix == 0x36 || prefix == 0x3E ||
                prefix == 0x64 || prefix == 0x65) {
                pos++;
                continue;
            }
            
            break;
        }
        
        // Main opcode
        if (code[pos] == 0x0F) {
            // Two-byte opcode
            instr.opcode = 0x0F00 | code[pos + 1];
            pos += 2;
        } else {
            // One-byte opcode
            instr.opcode = code[pos];
            pos++;
        }
        
        // Check for ModR/M byte
        instr.has_modrm = NeedsModRM(instr.opcode);
        if (instr.has_modrm) {
            instr.modrm = code[pos++];
            
            // Check for SIB byte
            uint8_t mod = (instr.modrm >> 6) & 3;
            uint8_t rm = instr.modrm & 7;
            
            if (mod != 3 && rm == 4) {
                instr.has_sib = true;
                instr.sib = code[pos++];
            }
            
            // Check for displacement
            if (mod == 1) {
                // 8-bit displacement
                instr.has_displacement = true;
                instr.displacement = (int8_t)code[pos++];
            } else if (mod == 2) {
                // 32-bit displacement
                instr.has_displacement = true;
                instr.displacement = *(int32_t*)&code[pos];
                pos += 4;
            } else if (mod == 0 && (rm == 5 || (rm == 4 && (instr.sib & 7) == 5))) {
                // 32-bit displacement for RIP-relative or absolute
                instr.has_displacement = true;
                instr.displacement = *(int32_t*)&code[pos];
                pos += 4;
            }
        }
        
        // Check for immediate
        if (NeedsImmediate(instr.opcode)) {
            instr.has_immediate = true;
            
            // Determine immediate size based on opcode
            if ((instr.opcode & 0xF8) == 0xB8) {  // MOV reg, imm64
                instr.immediate = *(uint64_t*)&code[pos];
                pos += 8;
            } else if (instr.opcode == 0xC7 || (instr.opcode & 0xFE) == 0x68) {  // 32-bit immediate
                instr.immediate = *(uint32_t*)&code[pos];
                pos += 4;
            } else if (instr.opcode == 0x83) {  // 8-bit sign-extended immediate
                instr.immediate = (int8_t)code[pos++];
            } else {  // 8-bit immediate
                instr.immediate = code[pos++];
            }
        }
        
        instr.length = pos;
        return instr;
    }
    
private:
    static bool NeedsModRM(uint8_t opcode) {
        // Simplified check - many opcodes need ModR/M
        return (opcode & 0xC6) == 0x00 || (opcode & 0xFE) == 0xC0 ||
               (opcode & 0xF8) == 0xD8 || (opcode & 0xF0) == 0x80 ||
               (opcode & 0xF0) == 0x90 || (opcode & 0xF0) == 0xB0 ||
               (opcode & 0xF8) == 0xB8 || opcode == 0xC7 || opcode == 0x83 ||
               opcode == 0x69 || opcode == 0x6B || opcode == 0x81;
    }
    
    static bool NeedsImmediate(uint8_t opcode) {
        return (opcode & 0xF0) == 0xB0 || (opcode & 0xF8) == 0xB8 ||
               opcode == 0x68 || opcode == 0x6A || opcode == 0x83 ||
               opcode == 0xC7 || opcode == 0x69 || opcode == 0x6B ||
               opcode == 0x81;
    }
};

// Real x86-64 execution engine
class RealX86_64ExecutionEngine : public ExecutionEngine {
private:
    X86_64Registers registers;
    uint8_t* memory;
    uint64_t memory_size;
    bool halted;
    uint64_t instruction_count;
    static const uint64_t MAX_INSTRUCTIONS = 10000000;
    
public:
    RealX86_64ExecutionEngine(uint8_t* mem, uint64_t mem_size) 
        : memory(mem), memory_size(mem_size), halted(false), instruction_count(0) {
        printf("[X86_64_EXEC] Real x86-64 execution engine created\n");
        printf("[X86_64_EXEC] Memory: %p - %p (size: 0x%llx)\n", 
               memory, memory + memory_size, mem_size);
    }
    
    virtual status_t Run(GuestContext& context) override {
        printf("[X86_64_EXEC] Starting real x86-64 execution\n");
        printf("[X86_64_EXEC] Entry point: 0x%llx\n", context.rip);
        
        // Initialize registers from context
        registers.rip = context.rip;
        registers.rsp = context.rsp;
        registers.rbp = context.rbp;
        
        instruction_count = 0;
        halted = false;
        
        // Main execution loop
        while (!halted && instruction_count < MAX_INSTRUCTIONS) {
            if (registers.rip >= memory_size) {
                printf("[X86_64_EXEC] RIP out of bounds: 0x%llx\n", registers.rip);
                return B_ERROR;
            }
            
            // Decode and execute instruction
            X86_64Decoder::Instruction instr = X86_64Decoder::Decode(
                &memory[registers.rip], registers.rip);
            
            printf("[X86_64_EXEC] Executing: opcode=0x%02x, length=%u at 0x%llx\n", 
                   instr.opcode, instr.length, registers.rip);
            
            status_t result = ExecuteInstruction(instr);
            if (result != B_OK) {
                printf("[X86_64_EXEC] Instruction execution failed: %d\n", result);
                return result;
            }
            
            // Advance RIP
            registers.rip += instr.length;
            instruction_count++;
            
            // Check for halt conditions
            if (registers.rip == 0) {
                printf("[X86_64_EXEC] RIP reached 0, halting\n");
                halted = true;
            }
        }
        
        if (instruction_count >= MAX_INSTRUCTIONS) {
            printf("[X86_64_EXEC] Maximum instruction limit reached\n");
        }
        
        printf("[X86_64_EXEC] Execution completed: %llu instructions\n", instruction_count);
        
        // Update context with final register state
        context.rip = registers.rip;
        context.rsp = registers.rsp;
        context.rbp = registers.rbp;
        
        return B_OK;
    }
    
    status_t ExecuteInstruction(const X86_64Decoder::Instruction& instr) {
        switch (instr.opcode) {
            // NOP
            case 0x90:
                printf("[X86_64_EXEC] NOP\n");
                break;
                
            // HLT
            case 0xF4:
                printf("[X86_64_EXEC] HLT - Halting execution\n");
                halted = true;
                break;
                
            // MOV r32, imm32
            case 0xB8: case 0xB9: case 0xBA: case 0xBB:
            case 0xBC: case 0xBD: case 0xBE: case 0xBF:
                if (instr.has_immediate) {
                    int reg = instr.opcode - 0xB8;
                    registers.SetRegister32(reg, (uint32_t)instr.immediate);
                    printf("[X86_64_EXEC] MOV r%d, 0x%x\n", reg, (uint32_t)instr.immediate);
                }
                break;
                
            // MOV r64, imm64 (REX.W + B8+r)
            case 0x48B8: case 0x48B9: case 0x48BA: case 0x48BB:
            case 0x48BC: case 0x48BD: case 0x48BE: case 0x48BF:
                if (instr.has_immediate) {
                    int reg = (instr.opcode & 0xFF) - 0xB8;
                    registers.SetRegister(reg, instr.immediate);
                    printf("[X86_64_EXEC] MOV r%d, 0x%llx\n", reg, instr.immediate);
                }
                break;
                
            // MOV r/m32, r32
            case 0x89:
                if (instr.has_modrm) {
                    int reg = (instr.modrm >> 3) & 7;
                    uint64_t addr = GetEffectiveAddress(instr);
                    if (addr + 4 <= memory_size) {
                        *(uint32_t*)&memory[addr] = registers.GetRegister32(reg);
                        printf("[X86_64_EXEC] MOV [0x%llx], r%d (0x%x)\n", 
                               addr, reg, registers.GetRegister32(reg));
                    }
                }
                break;
                
            // MOV r32, r/m32
            case 0x8B:
                if (instr.has_modrm) {
                    int reg = (instr.modrm >> 3) & 7;
                    uint64_t addr = GetEffectiveAddress(instr);
                    if (addr + 4 <= memory_size) {
                        registers.SetRegister32(reg, *(uint32_t*)&memory[addr]);
                        printf("[X86_64_EXEC] MOV r%d, [0x%llx] (0x%x)\n", 
                               reg, addr, *(uint32_t*)&memory[addr]);
                    }
                }
                break;
                
            // ADD r/m32, imm8
            case 0x83:
                if (instr.has_modrm && instr.has_immediate) {
                    int reg = instr.modrm & 7;
                    uint32_t current = registers.GetRegister32(reg);
                    uint32_t result = current + (uint32_t)instr.immediate;
                    registers.SetRegister32(reg, result);
                    printf("[X86_64_EXEC] ADD r%d, 0x%x (0x%x -> 0x%x)\n", 
                           reg, (uint32_t)instr.immediate, current, result);
                }
                break;
                
            // PUSH r64
            case 0x50: case 0x51: case 0x52: case 0x53:
            case 0x54: case 0x55: case 0x56: case 0x57:
                {
                    int reg = instr.opcode - 0x50;
                    uint64_t value = registers.GetRegister(reg);
                    registers.rsp -= 8;
                    if (registers.rsp + 8 <= memory_size) {
                        *(uint64_t*)&memory[registers.rsp] = value;
                        printf("[X86_64_EXEC] PUSH r%d (0x%llx) to [rsp]\n", reg, value);
                    }
                }
                break;
                
            // POP r64
            case 0x58: case 0x59: case 0x5A: case 0x5B:
            case 0x5C: case 0x5D: case 0x5E: case 0x5F:
                {
                    int reg = instr.opcode - 0x58;
                    if (registers.rsp + 8 <= memory_size) {
                        uint64_t value = *(uint64_t*)&memory[registers.rsp];
                        registers.SetRegister(reg, value);
                        registers.rsp += 8;
                        printf("[X86_64_EXEC] POP r%d = 0x%llx from [rsp]\n", reg, value);
                    }
                }
                break;
                
            // RET
            case 0xC3:
                if (registers.rsp + 8 <= memory_size) {
                    uint64_t ret_addr = *(uint64_t*)&memory[registers.rsp];
                    registers.rsp += 8;
                    registers.rip = ret_addr;
                    printf("[X86_64_EXEC] RET to 0x%llx\n", ret_addr);
                }
                break;
                
            // SYSCALL
            case 0x0F05:
                printf("[X86_64_EXEC] SYSCALL - System call\n");
                // Syscall handling would go here
                break;
                
            default:
                printf("[X86_64_EXEC] Unimplemented opcode: 0x%02x\n", instr.opcode);
                return B_ERROR;
        }
        
        return B_OK;
    }
    
    uint64_t GetEffectiveAddress(const X86_64Decoder::Instruction& instr) {
        if (!instr.has_modrm) {
            return 0;
        }
        
        uint8_t mod = (instr.modrm >> 6) & 3;
        uint8_t rm = instr.modrm & 7;
        uint8_t reg = (instr.modrm >> 3) & 7;
        
        uint64_t addr = 0;
        
        if (mod == 3) {
            // Register direct
            return registers.GetRegister(rm);
        }
        
        // Calculate base address
        if (mod == 0 && rm == 4) {
            // SIB without displacement
            if (instr.has_sib) {
                uint8_t base = instr.sib & 7;
                uint8_t index = (instr.sib >> 3) & 7;
                uint8_t scale = (instr.sib >> 6) & 3;
                
                addr = registers.GetRegister(base);
                if (index != 4) {  // SIB index 4 means no index
                    addr += registers.GetRegister(index) * (1 << scale);
                }
            }
        } else if (mod == 0 && rm == 5) {
            // RIP-relative displacement
            addr = registers.rip + instr.displacement;
        } else {
            // Direct address or base register + displacement
            addr = registers.GetRegister(rm);
            if (mod == 1 || mod == 2) {
                addr += instr.displacement;
            }
        }
        
        return addr;
    }
    
    // Getters and setters for external access
    uint64_t GetRegisterValue(const char* reg_name) const {
        if (strcmp(reg_name, "rax") == 0) return registers.rax;
        if (strcmp(reg_name, "rbx") == 0) return registers.rbx;
        if (strcmp(reg_name, "rcx") == 0) return registers.rcx;
        if (strcmp(reg_name, "rdx") == 0) return registers.rdx;
        if (strcmp(reg_name, "rsi") == 0) return registers.rsi;
        if (strcmp(reg_name, "rdi") == 0) return registers.rdi;
        if (strcmp(reg_name, "rbp") == 0) return registers.rbp;
        if (strcmp(reg_name, "rsp") == 0) return registers.rsp;
        if (strcmp(reg_name, "rip") == 0) return registers.rip;
        if (strncmp(reg_name, "r", 1) == 0) {
            int reg_num = atoi(reg_name + 1);
            if (reg_num >= 8 && reg_num <= 15) {
                return registers.GetRegister(reg_num);
            }
        }
        return 0;
    }
    
    void SetRegisterValue(const char* reg_name, uint64_t value) {
        if (strcmp(reg_name, "rax") == 0) registers.rax = value;
        else if (strcmp(reg_name, "rbx") == 0) registers.rbx = value;
        else if (strcmp(reg_name, "rcx") == 0) registers.rcx = value;
        else if (strcmp(reg_name, "rdx") == 0) registers.rdx = value;
        else if (strcmp(reg_name, "rsi") == 0) registers.rsi = value;
        else if (strcmp(reg_name, "rdi") == 0) registers.rdi = value;
        else if (strcmp(reg_name, "rbp") == 0) registers.rbp = value;
        else if (strcmp(reg_name, "rsp") == 0) registers.rsp = value;
        else if (strcmp(reg_name, "rip") == 0) registers.rip = value;
        else if (strncmp(reg_name, "r", 1) == 0) {
            int reg_num = atoi(reg_name + 1);
            if (reg_num >= 8 && reg_num <= 15) {
                registers.SetRegister(reg_num, value);
            }
        }
    }
    
    bool IsHalted() const {
        return halted;
    }
    
    void Halt() {
        halted = true;
        printf("[X86_64_EXEC] Execution halted\n");
    }
    
    void PrintStatus() const {
        printf("[X86_64_EXEC] Real x86-64 Execution Engine Status:\n");
        printf("  Halted: %s\n", halted ? "Yes" : "No");
        printf("  Instructions executed: %llu\n", instruction_count);
        printf("  RIP: 0x%016llx\n", registers.rip);
        printf("  RSP: 0x%016llx\n", registers.rsp);
        printf("  RBP: 0x%016llx\n", registers.rbp);
        printf("  RAX: 0x%016llx\n", registers.rax);
        printf("  RBX: 0x%016llx\n", registers.rbx);
        printf("  RCX: 0x%016llx\n", registers.rcx);
        printf("  RDX: 0x%016llx\n", registers.rdx);
        printf("  Memory range: %p - %p\n", memory, memory + memory_size);
    }
};