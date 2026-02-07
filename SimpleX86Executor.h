#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include "Phase2SyscallHandler.h"

// Very simple x86-32 executor for Phase 2
// Only handles the most basic instructions and syscalls

class SimpleX86Executor {
public:
    struct Registers {
        uint32_t eax, ebx, ecx, edx;
        uint32_t esi, edi, ebp, esp;
        uint32_t eip;
        uint32_t eflags;
    };
    
    SimpleX86Executor(void *base, size_t size)
        : memory_base((uint8_t *)base), memory_size(size) {
        memset(&regs, 0, sizeof(regs));
        regs.eip = 0;
        regs.esp = 0x30000000;  // Stack pointer
    }
    
    ~SimpleX86Executor() {}
    
    // Set entry point and execute
    bool Execute(uint32_t entry_point, Phase2SyscallHandler &handler) {
        printf("[SimpleX86] ============================================\n");
        printf("[SimpleX86] Starting execution at 0x%08x\n", entry_point);
        printf("[SimpleX86] ============================================\n");
        
        regs.eip = entry_point;
        
        // Simple execution loop - very limited
        uint32_t max_instructions = 100000;
        
        for (uint32_t i = 0; i < max_instructions; i++) {
            if (!ValidateAddress(regs.eip, 15)) {
                printf("[SimpleX86] ERROR: Invalid EIP 0x%08x\n", regs.eip);
                return false;
            }
            
            uint8_t *instr = memory_base + regs.eip;
            uint32_t bytes_consumed = 0;
            
            // Very basic instruction dispatch
            // Only handle INT 0x80 (syscalls) for now
            if (instr[0] == 0xcd && instr[1] == 0x80) {
                // INT 0x80 - syscall
                bytes_consumed = 2;
                
                // Get syscall args from registers
                uint32_t args[6] = {
                    regs.ebx, regs.ecx, regs.edx,
                    regs.esi, regs.edi, regs.ebp
                };
                
                uint32_t result = 0;
                bool should_exit = handler.HandleSyscall(regs.eax, args, &result);
                
                regs.eax = result;
                regs.eip += bytes_consumed;
                
                if (should_exit) {
                    printf("[SimpleX86] Program exited with code %d\n", result);
                    return true;
                }
            }
            else if (instr[0] == 0x55) {
                // PUSH EBP
                regs.esp -= 4;
                bytes_consumed = 1;
                regs.eip += bytes_consumed;
            }
            else if (instr[0] == 0x5d) {
                // POP EBP
                regs.esp += 4;
                bytes_consumed = 1;
                regs.eip += bytes_consumed;
            }
            else if (instr[0] == 0xc3) {
                // RET
                regs.esp += 4;
                bytes_consumed = 1;
                regs.eip += bytes_consumed;
            }
            else if (instr[0] == 0x90) {
                // NOP
                bytes_consumed = 1;
                regs.eip += bytes_consumed;
            }
            else {
                // Unknown instruction - try to skip it
                printf("[SimpleX86] Unknown instruction at 0x%08x: 0x%02x\n", 
                       regs.eip, instr[0]);
                bytes_consumed = 1;
                regs.eip += bytes_consumed;
            }
            
            if (bytes_consumed == 0) {
                printf("[SimpleX86] ERROR: Could not execute instruction at 0x%08x\n", regs.eip);
                return false;
            }
        }
        
        printf("[SimpleX86] Instruction limit reached (100000)\n");
        return false;
    }
    
private:
    uint8_t *memory_base;
    size_t memory_size;
    Registers regs;
    
    bool ValidateAddress(uint32_t addr, size_t size) {
        // In real execution, this would use the actual guest memory mapping
        // For now, just check basic bounds
        return addr < 0x40000000 || (addr >= 0x40000000 && addr + size < 0x40000000 + memory_size);
    }
};
