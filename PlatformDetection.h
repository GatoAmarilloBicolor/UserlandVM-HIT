// UserlandVM-HIT Platform Detection and Native Execution System
// Detects processor type and handles native/32-bit execution with correct offsets
// Author: Platform Detection System 2026-02-07

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <SupportDefs.h>

// Platform detection and execution system
class PlatformDetection {
public:
    // Processor architectures
    enum ProcessorArchitecture {
        ARCH_X86_32,
        ARCH_X86_64,
        ARCH_RISCV_32,
        ARCH_RISCV_64,
        ARCH_ARM_32,
        ARCH_ARM_64,
        ARCH_UNKNOWN
    };
    
    // Execution modes
    enum ExecutionMode {
        MODE_NATIVE,        // Execute directly on host processor
        MODE_EMULATED_32,  // Emulate 32-bit execution
        MODE_EMULATED_64,  // Emulate 64-bit execution
        MODE_SYSROOT        // Execute via sysroot/ld.so
    };
    
    // Platform information
    struct PlatformInfo {
        ProcessorArchitecture host_arch;
        ProcessorArchitecture target_arch;
        ExecutionMode execution_mode;
        bool is_little_endian;
        bool has_sse;
        bool has_sse2;
        bool has_avx;
        bool has_avx2;
        bool is_x86;
        bool is_riscv;
        bool is_arm;
        size_t page_size;
        size_t cache_line_size;
        
        const char* get_arch_name() const {
            switch (host_arch) {
                case ARCH_X86_32: return "x86-32";
                case ARCH_X86_64: return "x86-64";
                case ARCH_RISCV_32: return "RISC-V-32";
                case ARCH_RISCV_64: return "RISC-V-64";
                case ARCH_ARM_32: return "ARM-32";
                case ARCH_ARM_64: return "ARM-64";
                default: return "Unknown";
            }
        }
        
        const char* get_mode_name() const {
            switch (execution_mode) {
                case MODE_NATIVE: return "Native";
                case MODE_EMULATED_32: return "Emulated-32";
                case MODE_EMULATED_64: return "Emulated-64";
                case MODE_SYSROOT: return "Sysroot";
                default: return "Unknown";
            }
        }
    };
    
private:
    static PlatformInfo platform_info;
    static bool initialized;
    
    // Simple x86 detection using /proc/cpuinfo
    static bool is_x86_processor() {
        #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
            return true;
        #endif
        
        // Runtime detection as fallback
        FILE* fp = fopen("/proc/cpuinfo", "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "Intel") || strstr(line, "AMD") || 
                    strstr(line, "x86") || strstr(line, "i686") ||
                    strstr(line, "i386") || strstr(line, "x86_64")) {
                    fclose(fp);
                    return true;
                }
            }
            fclose(fp);
        }
        return false;
    }
    
    static bool is_riscv_processor() {
        #if defined(__riscv) || defined(__riscv__) || defined(RISCV)
            return true;
        #endif
        
        // Runtime detection as fallback
        FILE* fp = fopen("/proc/cpuinfo", "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "isa") || strstr(line, "riscv")) {
                    fclose(fp);
                    return true;
                }
            }
            fclose(fp);
        }
        return false;
    }
    
    static bool is_arm_processor() {
        #if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
            return true;
        #endif
        
        // Runtime detection as fallback
        FILE* fp = fopen("/proc/cpuinfo", "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "ARM") || strstr(line, "aarch64")) {
                    fclose(fp);
                    return true;
                }
            }
            fclose(fp);
        }
        return false;
    }
    
    // Detect processor architecture using compiler macros and runtime checks
    static ProcessorArchitecture detect_host_architecture() {
        // Use compiler preprocessor macros first
        #if defined(__x86_64__) || defined(_M_X64)
            printf("[PLATFORM] Detected x86-64 processor (compiler)\n");
            return ARCH_X86_64;
        #elif defined(__i386__) || defined(_M_IX86)
            printf("[PLATFORM] Detected x86-32 processor (compiler)\n");
            return ARCH_X86_32;
        #elif defined(__aarch64__) || defined(_M_ARM64)
            printf("[PLATFORM] Detected ARM-64 processor (compiler)\n");
            return ARCH_ARM_64;
        #elif defined(__arm__) || defined(_M_ARM)
            printf("[PLATFORM] Detected ARM-32 processor (compiler)\n");
            return ARCH_ARM_32;
        #elif defined(__riscv) || defined(RISCV)
            // Check pointer size to distinguish 32/64 bit
            if (sizeof(void*) == 4) {
                printf("[PLATFORM] Detected RISC-V-32 processor (compiler)\n");
                return ARCH_RISCV_32;
            } else {
                printf("[PLATFORM] Detected RISC-V-64 processor (compiler)\n");
                return ARCH_RISCV_64;
            }
        #endif
        
        // Runtime detection as fallback
        if (is_x86_processor()) {
            // Check if we're running on 32-bit or 64-bit
            if (sizeof(void*) == 4) {
                printf("[PLATFORM] Detected x86-32 processor (runtime)\n");
                return ARCH_X86_32;
            } else {
                printf("[PLATFORM] Detected x86-64 processor (runtime)\n");
                return ARCH_X86_64;
            }
        }
        
        // Try RISC-V detection
        if (is_riscv_processor()) {
            if (sizeof(void*) == 4) {
                printf("[PLATFORM] Detected RISC-V-32 processor (runtime)\n");
                return ARCH_RISCV_32;
            } else {
                printf("[PLATFORM] Detected RISC-V-64 processor (runtime)\n");
                return ARCH_RISCV_64;
            }
        }
        
        // Try ARM detection
        if (is_arm_processor()) {
            if (sizeof(void*) == 4) {
                printf("[PLATFORM] Detected ARM-32 processor (runtime)\n");
                return ARCH_ARM_32;
            } else {
                printf("[PLATFORM] Detected ARM-64 processor (runtime)\n");
                return ARCH_ARM_64;
            }
        }
        
        printf("[PLATFORM] Unknown processor architecture\n");
        return ARCH_UNKNOWN;
    }
    
    // Determine execution mode based on host and target architectures
    static ExecutionMode determine_execution_mode(ProcessorArchitecture target_arch) {
        ProcessorArchitecture host_arch = platform_info.host_arch;
        
        // If host and target architectures match, use native execution
        if (host_arch == target_arch) {
            printf("[PLATFORM] Host and target architectures match - NATIVE mode\n");
            return MODE_NATIVE;
        }
        
        // If we're on x86-64 running 32-bit target, emulate 32-bit
        if (host_arch == ARCH_X86_64 && target_arch == ARCH_X86_32) {
            printf("[PLATFORM] x86-64 host running x86-32 target - EMULATED-32 mode\n");
            return MODE_EMULATED_32;
        }
        
        // If we're on x86 running non-x86 target, use sysroot
        if ((host_arch == ARCH_X86_32 || host_arch == ARCH_X86_64) &&
            (target_arch == ARCH_RISCV_32 || target_arch == ARCH_RISCV_64 ||
             target_arch == ARCH_ARM_32 || target_arch == ARCH_ARM_64)) {
            printf("[PLATFORM] x86 host running non-x86 target - SYSROOT mode\n");
            return MODE_SYSROOT;
        }
        
        // Default to sysroot for mismatched architectures
        printf("[PLATFORM] Architecture mismatch - SYSROOT mode\n");
        return MODE_SYSROOT;
    }
    
    // Detect CPU features
    static void detect_cpu_features() {
        if (platform_info.is_x86) {
            // Simplified x86 feature detection
            platform_info.has_sse = true;
            platform_info.has_sse2 = true;
            platform_info.has_avx = false; // Would need proper CPUID
            platform_info.has_avx2 = false;
            
            printf("[PLATFORM] x86 CPU Features: SSE=%s SSE2=%s AVX=%s AVX2=%s\n",
                   platform_info.has_sse ? "Yes" : "No",
                   platform_info.has_sse2 ? "Yes" : "No",
                   platform_info.has_avx ? "Yes" : "No",
                   platform_info.has_avx2 ? "Yes" : "No");
        }
        
        // Detect endianness
        uint32_t test_value = 0x12345678;
        uint8_t* test_bytes = (uint8_t*)&test_value;
        platform_info.is_little_endian = (test_bytes[0] == 0x78);
        
        printf("[PLATFORM] Endianness: %s\n", 
               platform_info.is_little_endian ? "Little Endian" : "Big Endian");
        
        // Detect page size
        platform_info.page_size = 4096; // Default, could detect with sysconf()
        
        // Detect cache line size
        platform_info.cache_line_size = 64; // Default, could detect with CPUID
    }
    
public:
    static void initialize() {
        if (initialized) {
            return;
        }
        
        printf("[PLATFORM] Initializing platform detection system...\n");
        
        // Detect host architecture
        platform_info.host_arch = detect_host_architecture();
        platform_info.is_x86 = (platform_info.host_arch == ARCH_X86_32 || platform_info.host_arch == ARCH_X86_64);
        platform_info.is_riscv = (platform_info.host_arch == ARCH_RISCV_32 || platform_info.host_arch == ARCH_RISCV_64);
        platform_info.is_arm = (platform_info.host_arch == ARCH_ARM_32 || platform_info.host_arch == ARCH_ARM_64);
        
        // Detect CPU features
        detect_cpu_features();
        
        // For now, default to x86-32 target
        platform_info.target_arch = ARCH_X86_32;
        
        // Determine execution mode
        platform_info.execution_mode = determine_execution_mode(platform_info.target_arch);
        
        initialized = true;
        
        printf("[PLATFORM] Platform detection complete\n");
        printf("[PLATFORM] Host: %s, Target: %s, Mode: %s\n",
               platform_info.get_arch_name(),
               "x86-32", // Target for this implementation
               platform_info.get_mode_name());
    }
    
    static const PlatformInfo& get_platform_info() {
        if (!initialized) {
            initialize();
        }
        return platform_info;
    }
    
    static bool is_native_execution() {
        return get_platform_info().execution_mode == MODE_NATIVE;
    }
    
    static bool needs_emulation() {
        ExecutionMode mode = get_platform_info().execution_mode;
        return mode == MODE_EMULATED_32 || mode == MODE_EMULATED_64;
    }
    
    static bool needs_sysroot() {
        return get_platform_info().execution_mode == MODE_SYSROOT;
    }
    
    static void print_platform_info() {
        const PlatformInfo& info = get_platform_info();
        
        printf("\n=== PLATFORM INFORMATION ===\n");
        printf("Host Architecture: %s\n", info.get_arch_name());
        printf("Target Architecture: x86-32\n");
        printf("Execution Mode: %s\n", info.get_mode_name());
        printf("Endianness: %s\n", info.is_little_endian ? "Little Endian" : "Big Endian");
        printf("Page Size: %zu bytes\n", info.page_size);
        printf("Cache Line Size: %zu bytes\n", info.cache_line_size);
        
        if (info.is_x86) {
            printf("CPU Features:\n");
            printf("  SSE: %s\n", info.has_sse ? "Yes" : "No");
            printf("  SSE2: %s\n", info.has_sse2 ? "Yes" : "No");
            printf("  AVX: %s\n", info.has_avx ? "Yes" : "No");
            printf("  AVX2: %s\n", info.has_avx2 ? "Yes" : "No");
        }
        
        printf("==========================\n\n");
    }
};

// Static member initialization
PlatformDetection::PlatformInfo PlatformDetection::platform_info = {};
bool PlatformDetection::initialized = false;

// Correct instruction offset constants for 32-bit x86
namespace X86_32Offsets {
    // Correct offsets for control flow instructions
    const int32_t JMP_RELATIVE_8 = 2;   // JMP rel8: opcode(1) + offset(1)
    const int32_t JMP_RELATIVE_32 = 5;  // JMP rel32: opcode(1) + offset(4)
    const int32_t JCC_RELATIVE_8 = 2;   // Jcc rel8: opcode(2) + offset(1)
    const int32_t JCC_RELATIVE_32 = 6;  // Jcc rel32: opcode(2) + offset(4)
    const int32_t CALL_RELATIVE_32 = 5; // CALL rel32: opcode(1) + offset(4)
    const int32_t RET_NEAR = 1;        // RET: opcode(1)
    const int32_t RET_NEAR_IMM16 = 3;   // RET imm16: opcode(1) + imm(2)
    
    // ModR/M encoding offsets
    const int32_t MODRM_SIZE = 1;       // ModR/M byte
    const int32_t SIB_SIZE = 1;          // SIB byte
    const int32_t DISP8_SIZE = 1;        // 8-bit displacement
    const int32_t DISP32_SIZE = 4;       // 32-bit displacement
    const int32_t IMM8_SIZE = 1;        // 8-bit immediate
    const int32_t IMM16_SIZE = 2;       // 16-bit immediate
    const int32_t IMM32_SIZE = 4;       // 32-bit immediate
}

// 32-bit specific instruction execution with correct offsets
class X86_32NativeExecutor {
private:
    uint8_t* memory;
    uint32_t memory_size;
    
    struct Registers {
        uint32_t eax, ebx, ecx, edx;
        uint32_t esi, edi, ebp, esp;
        uint32_t eip;
        uint32_t eflags;
        
        Registers() {
            memset(this, 0, sizeof(*this));
            esp = 0x7FFFF000; // Default stack top
            eflags = 0x2;      // Reserved bit set
        }
    } regs;
    
public:
    X86_32NativeExecutor(uint8_t* mem, uint32_t mem_size) 
        : memory(mem), memory_size(mem_size) {
        printf("[X86_32_NATIVE] 32-bit native executor initialized\n");
        printf("[X86_32_NATIVE] Memory: %p - %p (size: 0x%x)\n", 
               memory, memory + memory_size, mem_size);
    }
    
    uint32_t execute_instruction() {
        if (regs.eip >= memory_size) {
            printf("[X86_32_NATIVE] EIP out of bounds: 0x%x\n", regs.eip);
            return 0xFFFFFFFF; // Error
        }
        
        uint8_t opcode = memory[regs.eip];
        uint32_t instruction_length = 1;
        
        switch (opcode) {
            // JMP instructions with correct offsets
            case 0xEB: // JMP rel8
                {
                    int8_t offset = (int8_t)memory[regs.eip + 1];
                    regs.eip += X86_32Offsets::JMP_RELATIVE_8 + offset;
                    instruction_length = 0; // EIP already updated
                    printf("[X86_32_NATIVE] JMP rel8: 0x%x (offset: %d)\n", 
                           regs.eip, offset);
                }
                break;
                
            case 0xE9: // JMP rel32
                {
                    int32_t offset = *(int32_t*)&memory[regs.eip + 1];
                    regs.eip += X86_32Offsets::JMP_RELATIVE_32 + offset;
                    instruction_length = 0; // EIP already updated
                    printf("[X86_32_NATIVE] JMP rel32: 0x%x (offset: %d)\n", 
                           regs.eip, offset);
                }
                break;
                
            // Conditional jump instructions
            case 0x70: case 0x71: case 0x72: case 0x73:
            case 0x74: case 0x75: case 0x76: case 0x77:
            case 0x78: case 0x79: case 0x7A: case 0x7B:
            case 0x7C: case 0x7D: case 0x7E: case 0x7F:
                // Jcc rel8
                {
                    int8_t offset = (int8_t)memory[regs.eip + 1];
                    bool taken = evaluate_condition(opcode - 0x70);
                    if (taken) {
                        regs.eip += X86_32Offsets::JCC_RELATIVE_8 + offset;
                        instruction_length = 0; // EIP already updated
                    } else {
                        instruction_length = X86_32Offsets::JCC_RELATIVE_8;
                    }
                    printf("[X86_32_NATIVE] Jcc rel8: opcode=0x%02x, taken=%s, new_eip=0x%x\n", 
                           opcode, taken ? "Yes" : "No", regs.eip);
                }
                break;
                
            // CALL instruction
            case 0xE8: // CALL rel32
                {
                    // Push return address
                    regs.esp -= 4;
                    if (regs.esp + 4 <= memory_size) {
                        *(uint32_t*)&memory[regs.esp] = regs.eip + X86_32Offsets::CALL_RELATIVE_32;
                    }
                    
                    // Jump to target
                    int32_t offset = *(int32_t*)&memory[regs.eip + 1];
                    regs.eip += offset;
                    instruction_length = 0; // EIP already updated
                    printf("[X86_32_NATIVE] CALL rel32: target=0x%x, return_addr=0x%x\n", 
                           regs.eip, regs.esp);
                }
                break;
                
            // RET instructions
            case 0xC3: // RET near
                {
                    if (regs.esp + 4 <= memory_size) {
                        regs.eip = *(uint32_t*)&memory[regs.esp];
                        regs.esp += 4;
                        instruction_length = 0; // EIP already updated
                        printf("[X86_32_NATIVE] RET: return to 0x%x\n", regs.eip);
                    } else {
                        printf("[X86_32_NATIVE] RET: stack overflow\n");
                        return 0xFFFFFFFF;
                    }
                }
                break;
                
            case 0xC2: // RET near imm16
                {
                    uint16_t imm16 = *(uint16_t*)&memory[regs.eip + 1];
                    if (regs.esp + 4 <= memory_size) {
                        regs.eip = *(uint32_t*)&memory[regs.esp];
                        regs.esp += 4 + imm16;
                        instruction_length = 0; // EIP already updated
                        printf("[X86_32_NATIVE] RET imm16: return to 0x%x, stack_add=%u\n", 
                               regs.eip, imm16);
                    } else {
                        printf("[X86_32_NATIVE] RET: stack overflow\n");
                        return 0xFFFFFFFF;
                    }
                }
                break;
                
            // Basic arithmetic instructions
            case 0x40: case 0x41: case 0x42: case 0x43:
            case 0x44: case 0x45: case 0x46: case 0x47:
                // INC reg
                {
                    int reg = opcode - 0x40;
                    set_register(reg, get_register(reg) + 1);
                    instruction_length = 1;
                    printf("[X86_32_NATIVE] INC %s\n", get_register_name(reg));
                }
                break;
                
            case 0x48: case 0x49: case 0x4A: case 0x4B:
            case 0x4C: case 0x4D: case 0x4E: case 0x4F:
                // DEC reg
                {
                    int reg = opcode - 0x48;
                    set_register(reg, get_register(reg) - 1);
                    instruction_length = 1;
                    printf("[X86_32_NATIVE] DEC %s\n", get_register_name(reg));
                }
                break;
                
            case 0xB8: case 0xB9: case 0xBA: case 0xBB:
            case 0xBC: case 0xBD: case 0xBE: case 0xBF:
                // MOV reg, imm32
                {
                    int reg = opcode - 0xB8;
                    uint32_t imm32 = *(uint32_t*)&memory[regs.eip + 1];
                    set_register(reg, imm32);
                    instruction_length = X86_32Offsets::IMM32_SIZE + 1;
                    printf("[X86_32_NATIVE] MOV %s, 0x%x\n", 
                           get_register_name(reg), imm32);
                }
                break;
                
            default:
                instruction_length = 1; // Default to 1 byte for unimplemented
                printf("[X86_32_NATIVE] Unimplemented opcode: 0x%02x\n", opcode);
                break;
        }
        
        return instruction_length;
    }
    
private:
    // Register access methods
    uint32_t get_register(int reg) const {
        switch (reg) {
            case 0: return regs.eax;
            case 1: return regs.ecx;
            case 2: return regs.edx;
            case 3: return regs.ebx;
            case 4: return regs.esp;
            case 5: return regs.ebp;
            case 6: return regs.esi;
            case 7: return regs.edi;
            default: return 0;
        }
    }
    
    void set_register(int reg, uint32_t value) {
        switch (reg) {
            case 0: regs.eax = value; break;
            case 1: regs.ecx = value; break;
            case 2: regs.edx = value; break;
            case 3: regs.ebx = value; break;
            case 4: regs.esp = value; break;
            case 5: regs.ebp = value; break;
            case 6: regs.esi = value; break;
            case 7: regs.edi = value; break;
        }
    }
    
    const char* get_register_name(int reg) const {
        static const char* names[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"};
        return (reg >= 0 && reg < 8) ? names[reg] : "Unknown";
    }
    
    // Condition evaluation for conditional jumps
    bool evaluate_condition(int condition) const {
        uint32_t flags = regs.eflags;
        
        switch (condition) {
            case 0: // JO (Overflow)
                return (flags & 0x800) != 0;
            case 1: // JNO (No Overflow)
                return (flags & 0x800) == 0;
            case 2: // JB (Below)
                return (flags & 0x1) != 0;
            case 3: // JNB (Not Below)
                return (flags & 0x1) == 0;
            case 4: // JE (Equal)
                return (flags & 0x40) != 0;
            case 5: // JNE (Not Equal)
                return (flags & 0x40) == 0;
            case 6: // JBE (Below or Equal)
                return (flags & 0x41) != 0;
            case 7: // JNBE (Not Below or Equal)
                return (flags & 0x41) == 0;
            default:
                return false;
        }
    }
    
public:
    // Execute until completion or error
    uint32_t run(uint32_t entry_point, uint32_t stack_pointer = 0x7FFFF000) {
        printf("[X86_32_NATIVE] Starting 32-bit execution at 0x%x\n", entry_point);
        printf("[X86_32_NATIVE] Stack pointer: 0x%x\n", stack_pointer);
        
        regs.eip = entry_point;
        regs.esp = stack_pointer;
        
        uint32_t instruction_count = 0;
        const uint32_t MAX_INSTRUCTIONS = 10000000;
        
        while (instruction_count < MAX_INSTRUCTIONS) {
            uint32_t instruction_length = execute_instruction();
            
            if (instruction_length == 0xFFFFFFFF) {
                printf("[X86_32_NATIVE] Execution error\n");
                return 0xFFFFFFFF;
            }
            
            if (instruction_length == 0) {
                // EIP was updated by the instruction
                instruction_count++;
                continue;
            }
            
            regs.eip += instruction_length;
            instruction_count++;
            
            // Check for termination conditions
            if (regs.eip == 0) {
                printf("[X86_32_NATIVE] Execution completed (EIP = 0)\n");
                break;
            }
        }
        
        printf("[X86_32_NATIVE] Execution finished: %u instructions\n", instruction_count);
        return 0; // Success
    }
};

// Main execution dispatcher that chooses the appropriate method
class ExecutionDispatcher {
public:
    static uint32_t execute_binary(uint8_t* binary_data, uint32_t binary_size, 
                                uint32_t entry_point, uint32_t stack_pointer = 0x7FFFF000) {
        PlatformDetection::initialize();
        PlatformDetection::print_platform_info();
        
        const PlatformDetection::PlatformInfo& platform_info = PlatformDetection::get_platform_info();
        
        if (platform_info.execution_mode == PlatformDetection::MODE_NATIVE) {
            // Check if we're running on 32-bit x86
            if (platform_info.host_arch == PlatformDetection::ARCH_X86_32) {
                printf("[DISPATCHER] Using native 32-bit execution\n");
                X86_32NativeExecutor executor(binary_data, binary_size);
                return executor.run(entry_point, stack_pointer);
            }
        }
        
        // For now, return error for non-native cases
        printf("[DISPATCHER] Non-native execution not yet implemented\n");
        return 0xFFFFFFFF;
    }
};