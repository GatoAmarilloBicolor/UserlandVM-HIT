// UserlandVM-HIT Optimized x86-64 Execution Engine
// High-performance execution with reduced cycles and optimized paths
// Author: Optimized x86-64 Execution Engine 2026-02-07

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

// Optimized register file with fast access
struct OptimizedX86_64Registers {
    uint64_t regs[16];  // Fast array access: 0=RAX, 1=RCX, 2=RDX, 3=RBX, 4=RSP, 5=RBP, 6=RSI, 7=RDI, 8-15=R8-R15
    uint64_t rip;
    uint64_t rflags;
    
    // Precomputed register name mapping (eliminates string operations)
    static constexpr const char* REG_NAMES[16] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
    };
    
    OptimizedX86_64Registers() {
        memset(this, 0, sizeof(*this));
        rip = 0;
        regs[4] = 0x7FFFF000; // RSP default stack top
        rflags = 0x2;         // Reserved bit set
    }
    
    // Fast inline register access
    inline uint64_t get_reg(int reg) const {
        return regs[reg & 0xF];
    }
    
    inline void set_reg(int reg, uint64_t value) {
        regs[reg & 0xF] = value;
    }
    
    inline uint32_t get_reg32(int reg) const {
        return (uint32_t)regs[reg & 0xF];
    }
    
    inline void set_reg32(int reg, uint32_t value) {
        regs[reg & 0xF] = (uint64_t)value;
    }
    
    inline uint16_t get_reg16(int reg) const {
        return (uint16_t)regs[reg & 0xF];
    }
    
    inline void set_reg16(int reg, uint16_t value) {
        regs[reg & 0xF] = (uint64_t)value;
    }
    
    inline uint8_t get_reg8(int reg) const {
        return (uint8_t)regs[reg & 0xF];
    }
    
    inline void set_reg8(int reg, uint8_t value) {
        regs[reg & 0xF] = (uint64_t)value;
    }
    
    inline const char* get_reg_name(int reg) const {
        return REG_NAMES[reg & 0xF];
    }
};

// Optimized instruction structure with precomputed fields
struct OptimizedX86_64Instruction {
    uint8_t opcode;
    uint8_t modrm;
    uint8_t sib;
    uint64_t displacement;
    uint64_t immediate;
    uint8_t length;
    uint8_t operand_size;  // Precomputed
    uint8_t address_size;  // Precomputed
    
    // Precomputed flags for fast branching
    bool has_modrm : 1;
    bool has_sib : 1;
    bool has_displacement : 1;
    bool has_immediate : 1;
    bool is_64bit : 1;
    bool is_mem_access : 1;
    bool is_jump : 1;
    bool is_call : 1;
    
    // Precomputed effective address (when possible)
    uint64_t cached_addr;
    bool addr_cached;
    
    OptimizedX86_64Instruction() {
        memset(this, 0, sizeof(*this));
        operand_size = 64;
        address_size = 64;
        addr_cached = false;
    }
};

// Optimized instruction decoder with caching
class OptimizedX86_64Decoder {
private:
    static constexpr uint8_t NEEDS_MODRM_TABLE[256] = {
        // Simplified table - 1 if opcode needs ModR/M, 0 otherwise
        0,0,0,0,0,0,0,0, 1,1,1,1,1,1,1,1, // 0x00-0x0F
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 0x10-0x1F
        1,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0, // 0x20-0x2F
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 0x30-0x3F
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 0x40-0x4F
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 0x50-0x5F
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 0x60-0x6F
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 0x70-0x7F
        1,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0, // 0x80-0x8F
        1,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0, // 0x90-0x9F
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 0xA0-0xAF
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 0xB0-0xBF
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 0xC0-0xCF
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, // 0xD0-0xDF
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, // 0xE0-0xEF
        1,1,1,1,1,1,1,1  // 0xF0-0xF7
    };
    
    InstructionCache<OptimizedX86_64Instruction> instruction_cache;
    
public:
    OptimizedX86_64Decoder() {
        PERF_LOG("Optimized x86-64 decoder initialized");
    }
    
    inline const OptimizedX86_64Instruction* decode(const uint8_t* code, uint64_t rip) {
        // Check cache first
        const auto* cached = instruction_cache.lookup(rip);
        if (cached) {
            return cached;
        }
        
        OptimizedX86_64Instruction instr;
        uint64_t pos = 0;
        bool rex_w = false; // REX.W for 64-bit operations
        
        // Fast prefix processing
        while (true) {
            uint8_t prefix = code[pos];
            
            if (prefix == 0x66) {
                instr.operand_size = 16;
                pos++;
            } else if (prefix == 0x67) {
                instr.address_size = 32;
                pos++;
            } else if ((prefix & 0xF0) == 0x40) {
                // REX prefix
                rex_w = (prefix & 0x8) != 0;
                pos++;
            } else if (prefix == 0x26 || prefix == 0x2E || prefix == 0x36 || prefix == 0x3E ||
                      prefix == 0x64 || prefix == 0x65) {
                pos++; // Segment prefix
            } else {
                break;
            }
        }
        
        instr.is_64bit = rex_w || instr.operand_size == 64;
        
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
        
        // Precompute instruction type flags
        instr.is_jump = (instr.opcode >= 0x70 && instr.opcode <= 0x8F) || 
                       (instr.opcode >= 0xE0 && instr.opcode <= 0xE9) ||
                        instr.opcode == 0xEA || instr.opcode == 0xEB;
        instr.is_call = (instr.opcode == 0xE8) || (instr.opcode == 0x9A) || 
                       (instr.opcode == 0xFF && (code[pos] & 0x38) == 0x10);
        
        // Fast ModR/M detection
        instr.has_modrm = NEEDS_MODRM_TABLE[instr.opcode & 0xFF] != 0;
        
        if (instr.has_modrm) {
            instr.modrm = code[pos++];
            uint8_t mod = (instr.modrm >> 6) & 3;
            uint8_t rm = instr.modrm & 7;
            
            // Check for SIB
            if (mod != 3 && rm == 4) {
                instr.has_sib = true;
                instr.sib = code[pos++];
            }
            
            // Fast displacement calculation
            if (mod == 1) {
                instr.has_displacement = true;
                instr.displacement = (int8_t)code[pos++];
            } else if (mod == 2) {
                instr.has_displacement = true;
                instr.displacement = *(int32_t*)&code[pos];
                pos += 4;
            } else if (mod == 0 && (rm == 5 || (instr.has_sib && (instr.sib & 7) == 5))) {
                instr.has_displacement = true;
                instr.displacement = *(int32_t*)&code[pos];
                pos += 4;
                
                // RIP-relative - we can cache this
                if (!instr.has_sib) {
                    instr.cached_addr = rip + pos + instr.displacement;
                    instr.addr_cached = true;
                    instr.is_mem_access = true;
                }
            }
        }
        
        // Fast immediate detection and extraction
        if (needs_immediate(instr.opcode)) {
            instr.has_immediate = true;
            
            if ((instr.opcode & 0xF8) == 0xB8) { // MOV reg, imm64
                instr.immediate = *(uint64_t*)&code[pos];
                pos += 8;
            } else if (instr.opcode == 0xC7 || (instr.opcode & 0xFE) == 0x68) {
                instr.immediate = *(uint32_t*)&code[pos];
                pos += 4;
            } else if (instr.opcode == 0x83) {
                instr.immediate = (int8_t)code[pos++];
            } else {
                instr.immediate = code[pos++];
            }
        }
        
        instr.length = pos;
        
        // Cache the instruction
        instruction_cache.insert(rip, instr);
        
        return instruction_cache.lookup(rip);
    }
    
private:
    inline bool needs_immediate(uint8_t opcode) const {
        return (opcode & 0xF0) == 0xB0 || (opcode & 0xF8) == 0xB8 ||
               opcode == 0x68 || opcode == 0x6A || opcode == 0x83 ||
               opcode == 0xC7 || opcode == 0x69 || opcode == 0x6B ||
               opcode == 0x81;
    }
};

// High-performance x86-64 execution engine
class OptimizedX86_64ExecutionEngine : public ExecutionEngine {
private:
    OptimizedX86_64Registers registers;
    uint8_t* memory;
    uint64_t memory_size;
    bool halted;
    uint64_t instruction_count;
    static constexpr uint64_t MAX_INSTRUCTIONS = 10000000;
    
    OptimizedX86_64Decoder decoder;
    
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
    
    // Fast effective address calculation with caching
    inline uint64_t get_effective_address(const OptimizedX86_64Instruction& instr) {
        if (instr.addr_cached) {
            return instr.cached_addr;
        }
        
        if (!instr.has_modrm) {
            return 0;
        }
        
        uint8_t mod = (instr.modrm >> 6) & 3;
        uint8_t rm = instr.modrm & 7;
        
        uint64_t addr = 0;
        
        if (mod == 3) {
            // Register direct
            return registers.get_reg(rm);
        }
        
        // Calculate base address
        if (mod == 0 && rm == 4) {
            // SIB without displacement
            if (instr.has_sib) {
                uint8_t base = instr.sib & 7;
                uint8_t index = (instr.sib >> 3) & 7;
                uint8_t scale = (instr.sib >> 6) & 3;
                
                addr = registers.get_reg(base);
                if (index != 4) { // SIB index 4 means no index
                    addr += registers.get_reg(index) * (1 << scale);
                }
            }
        } else if (mod == 0 && rm == 5) {
            // RIP-relative displacement (should be cached, but fallback)
            addr = registers.rip + instr.displacement;
        } else {
            // Direct address or base register + displacement
            addr = registers.get_reg(rm);
            if (instr.has_displacement) {
                addr += instr.displacement;
            }
        }
        
        return addr;
    }
    
    // Optimized instruction execution with fast paths
    inline status_t execute_instruction_fast(const OptimizedX86_64Instruction& instr) {
        PERF_COUNT();
        
        switch (instr.opcode) {
            // Fast path for common instructions
            case 0x90: // NOP
                return B_OK;
                
            case 0xF4: // HLT
                PRODUCTION_LOG("HLT - Halting execution");
                halted = true;
                return B_OK;
                
            // Fast path for MOV r32, imm32
            case 0xB8: case 0xB9: case 0xBA: case 0xBB:
            case 0xBC: case 0xBD: case 0xBE: case 0xBF:
                if (instr.has_immediate) {
                    int reg = instr.opcode - 0xB8;
                    registers.set_reg32(reg, (uint32_t)instr.immediate);
                    DEBUG_LOG("MOV %s, 0x%x", registers.get_reg_name(reg), (uint32_t)instr.immediate);
                }
                return B_OK;
                
            // Fast path for MOV r64, imm64 (REX.W + B8+r)
            case 0x48B8: case 0x48B9: case 0x48BA: case 0x48BB:
            case 0x48BC: case 0x48BD: case 0x48BE: case 0x48BF:
                if (instr.has_immediate) {
                    int reg = (instr.opcode & 0xFF) - 0xB8;
                    registers.set_reg(reg, instr.immediate);
                    DEBUG_LOG("MOV %s, 0x%llx", registers.get_reg_name(reg), instr.immediate);
                }
                return B_OK;
                
            // Fast path for register-to-register moves
            case 0x89: // MOV r/m32, r32
                if (instr.has_modrm) {
                    int reg = (instr.modrm >> 3) & 7;
                    uint64_t addr = get_effective_address(instr);
                    write_memory<uint32_t>(addr, registers.get_reg32(reg));
                    DEBUG_LOG("MOV [0x%llx], %s", addr, registers.get_reg_name(reg));
                }
                return B_OK;
                
            case 0x8B: // MOV r32, r/m32
                if (instr.has_modrm) {
                    int reg = (instr.modrm >> 3) & 7;
                    uint64_t addr = get_effective_address(instr);
                    uint32_t value = read_memory<uint32_t>(addr);
                    registers.set_reg32(reg, value);
                    DEBUG_LOG("MOV %s, [0x%llx] (0x%x)", registers.get_reg_name(reg), addr, value);
                }
                return B_OK;
                
            // Fast path for immediate ALU operations
            case 0x83: // ADD/SUB/AND/OR/CMP r/m32, imm8
                if (instr.has_modrm && instr.has_immediate) {
                    int reg = instr.modrm & 7;
                    uint8_t sub_op = (instr.modrm >> 3) & 7;
                    uint32_t current = registers.get_reg32(reg);
                    uint32_t imm = (uint32_t)instr.immediate;
                    
                    switch (sub_op) {
                        case 0: // ADD
                            registers.set_reg32(reg, current + imm);
                            DEBUG_LOG("ADD %s, 0x%x", registers.get_reg_name(reg), imm);
                            break;
                        case 5: // SUB
                            registers.set_reg32(reg, current - imm);
                            DEBUG_LOG("SUB %s, 0x%x", registers.get_reg_name(reg), imm);
                            break;
                        default:
                            return B_ERROR;
                    }
                }
                return B_OK;
                
            // Fast path for PUSH/POP
            case 0x50: case 0x51: case 0x52: case 0x53:
            case 0x54: case 0x55: case 0x56: case 0x57:
                {
                    int reg = instr.opcode - 0x50;
                    uint64_t value = registers.get_reg(reg);
                    registers.regs[4] -= 8; // RSP
                    write_memory<uint64_t>(registers.regs[4], value);
                    DEBUG_LOG("PUSH %s (0x%llx)", registers.get_reg_name(reg), value);
                }
                return B_OK;
                
            case 0x58: case 0x59: case 0x5A: case 0x5B:
            case 0x5C: case 0x5D: case 0x5E: case 0x5F:
                {
                    int reg = instr.opcode - 0x58;
                    uint64_t value = read_memory<uint64_t>(registers.regs[4]);
                    registers.set_reg(reg, value);
                    registers.regs[4] += 8; // RSP
                    DEBUG_LOG("POP %s = 0x%llx", registers.get_reg_name(reg), value);
                }
                return B_OK;
                
            case 0xC3: // RET
                {
                    uint64_t ret_addr = read_memory<uint64_t>(registers.regs[4]);
                    registers.regs[4] += 8;
                    registers.rip = ret_addr;
                    DEBUG_LOG("RET to 0x%llx", ret_addr);
                }
                return B_OK;
                
            case 0x0F05: // SYSCALL
                PRODUCTION_LOG("SYSCALL - System call");
                // System call handling would go here
                return B_OK;
                
            default:
                DEBUG_LOG("Unimplemented opcode: 0x%02x", instr.opcode);
                return B_ERROR;
        }
    }
    
public:
    OptimizedX86_64ExecutionEngine(uint8_t* mem, uint64_t mem_size) 
        : memory(mem), memory_size(mem_size), halted(false), instruction_count(0) {
        PRODUCTION_LOG("Optimized x86-64 execution engine created");
        PRODUCTION_LOG("Memory: %p - %p (size: 0x%llx)", memory, memory + memory_size, mem_size);
    }
    
    virtual status_t Run(GuestContext& context) override {
        PRODUCTION_LOG("Starting optimized x86-64 execution");
        PRODUCTION_LOG("Entry point: 0x%llx", context.rip);
        
        // Initialize registers from context
        registers.rip = context.rip;
        registers.regs[4] = context.rsp; // RSP
        registers.regs[5] = context.rbp; // RBP
        
        instruction_count = 0;
        halted = false;
        
        // Main execution loop with optimizations
        while (!halted && instruction_count < MAX_INSTRUCTIONS) {
            if (registers.rip >= memory_size) {
                ERROR_LOG("RIP out of bounds: 0x%llx", registers.rip);
                return B_ERROR;
            }
            
            // Decode instruction with caching
            const auto* instr = decoder.decode(&memory[registers.rip], registers.rip);
            if (!instr) {
                ERROR_LOG("Instruction decoding failed at 0x%llx", registers.rip);
                return B_ERROR;
            }
            
            DEBUG_LOG("Executing: opcode=0x%02x, length=%u at 0x%llx", 
                     instr->opcode, instr->length, registers.rip);
            
            // Execute instruction with fast path
            status_t result = execute_instruction_fast(*instr);
            if (result != B_OK) {
                ERROR_LOG("Instruction execution failed: %d", result);
                return result;
            }
            
            // Advance RIP (unless instruction changed it)
            if (!instr->is_jump && !instr->is_call) {
                registers.rip += instr->length;
            }
            
            instruction_count++;
        }
        
        if (instruction_count >= MAX_INSTRUCTIONS) {
            PRODUCTION_LOG("Maximum instruction limit reached");
        }
        
        PRODUCTION_LOG("Execution completed: %llu instructions", instruction_count);
        
        // Update context with final register state
        context.rip = registers.rip;
        context.rsp = registers.regs[4]; // RSP
        context.rbp = registers.regs[5]; // RBP
        
        // Print performance report
        PERF_REPORT();
        
        return B_OK;
    }
    
    // Performance-optimized getters/setters
    uint64_t get_register_value(const char* reg_name) const {
        // Fast lookup for common registers
        switch (reg_name[0]) {
            case 'r':
                switch (reg_name[1]) {
                    case 'a': return registers.regs[0]; // rax
                    case 'b': return registers.regs[3]; // rbx  
                    case 'c': return registers.regs[1]; // rcx
                    case 'd': return registers.regs[2]; // rdx
                    case 's':
                        if (reg_name[2] == 'p') return registers.regs[4]; // rsp
                        if (reg_name[2] == 'i') return registers.regs[6]; // rsi
                        break;
                    case 'i': return registers.regs[7]; // rdi
                    case '8': return registers.regs[8];
                    case '9': return registers.regs[9];
                    default:
                        if (reg_name[2] >= '0' && reg_name[2] <= '5') {
                            return registers.regs[10 + (reg_name[2] - '0')]; // r10-r15
                        }
                        break;
                }
                break;
            case 'p':
                if (reg_name[1] == 'c') return registers.rip; // pc/rip
                break;
        }
        
        ERROR_LOG("Unknown register: %s", reg_name);
        return 0;
    }
    
    void set_register_value(const char* reg_name, uint64_t value) {
        // Fast lookup for common registers
        switch (reg_name[0]) {
            case 'r':
                switch (reg_name[1]) {
                    case 'a': registers.regs[0] = value; break; // rax
                    case 'b': registers.regs[3] = value; break; // rbx
                    case 'c': registers.regs[1] = value; break; // rcx
                    case 'd': registers.regs[2] = value; break; // rdx
                    case 's':
                        if (reg_name[2] == 'p') registers.regs[4] = value; break; // rsp
                        if (reg_name[2] == 'i') registers.regs[6] = value; break; // rsi
                        break;
                    case 'i': registers.regs[7] = value; break; // rdi
                    case '8': registers.regs[8] = value; break;
                    case '9': registers.regs[9] = value; break;
                    default:
                        if (reg_name[2] >= '0' && reg_name[2] <= '5') {
                            registers.regs[10 + (reg_name[2] - '0')] = value; break; // r10-r15
                        }
                        break;
                }
                break;
            case 'p':
                if (reg_name[1] == 'c') registers.rip = value; break; // pc/rip
                break;
            default:
                ERROR_LOG("Unknown register: %s", reg_name);
                break;
        }
    }
    
    bool is_halted() const {
        return halted;
    }
    
    void halt() {
        halted = true;
        PRODUCTION_LOG("Execution halted");
    }
    
    void print_status() const {
        printf("[OPT_X86_64] Optimized x86-64 Execution Engine Status:\n");
        printf("  Halted: %s\n", halted ? "Yes" : "No");
        printf("  Instructions executed: %llu\n", instruction_count);
        printf("  RIP: 0x%016llx\n", registers.rip);
        printf("  RSP: 0x%016llx\n", registers.regs[4]);
        printf("  RBP: 0x%016llx\n", registers.regs[5]);
        printf("  RAX: 0x%016llx\n", registers.regs[0]);
        printf("  RBX: 0x%016llx\n", registers.regs[3]);
        printf("  RCX: 0x%016llx\n", registers.regs[1]);
        printf("  RDX: 0x%016llx\n", registers.regs[2]);
        printf("  Memory range: %p - %p\n", memory, memory + memory_size);
        
        // Print cache statistics
        // decoder.print_cache_stats(); // Would need to add this method
    }
};