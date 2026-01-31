/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * FPU Instruction Handler
 * Decodes and executes x87 FPU instructions
 * Delegates to FloatingPointUnit for actual computation
 */

#pragma once

#include <SupportDefs.h>
#include "FloatingPointUnit.h"

class X86_32GuestContext;
class AddressSpace;

/**
 * FPU Instruction opcodes (first byte = 0xD8-0xDF)
 */
enum FPUOpcode {
    FPU_CLASS_D8 = 0xD8,  // ESC 0
    FPU_CLASS_D9 = 0xD9,  // ESC 1
    FPU_CLASS_DA = 0xDA,  // ESC 2
    FPU_CLASS_DB = 0xDB,  // ESC 3
    FPU_CLASS_DC = 0xDC,  // ESC 4
    FPU_CLASS_DD = 0xDD,  // ESC 5
    FPU_CLASS_DE = 0xDE,  // ESC 6
    FPU_CLASS_DF = 0xDF   // ESC 7
};

/**
 * FPU Instruction Handler
 * Processes x87 FPU instructions (ESC opcodes D8-DF)
 */
class FPUInstructionHandler {
public:
    FPUInstructionHandler(FloatingPointUnit& fpu);
    ~FPUInstructionHandler();
    
    /**
     * Execute an FPU instruction
     * Returns:
     *   B_OK: instruction executed successfully
     *   B_BAD_VALUE: invalid opcode
     *   B_DEVICE_NOT_READY: FPU not ready
     */
    status_t Execute(uint8_t opcode, uint8_t modrm, 
                     X86_32GuestContext& context, 
                     AddressSpace& addressSpace);
    
private:
    FloatingPointUnit& fFPU;
    
    // Handler functions for each FPU class (D8-DF)
    status_t HandleD8(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space);
    status_t HandleD9(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space);
    status_t HandleDA(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space);
    status_t HandleDB(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space);
    status_t HandleDC(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space);
    status_t HandleDD(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space);
    status_t HandleDE(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space);
    status_t HandleDF(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space);
    
    // Common operations
    status_t ExecuteStackOperation(uint8_t opcode, uint8_t reg);
    status_t ExecuteMemoryOperation(uint8_t opcode, uint8_t modrm, 
                                     X86_32GuestContext& context, AddressSpace& space);
    
    // Specific instruction implementations
    status_t Inst_FILD(uint32_t addr, AddressSpace& space);    // Load integer
    status_t Inst_FISTORE(uint32_t addr, AddressSpace& space); // Store integer
    status_t Inst_FLD(uint32_t addr, AddressSpace& space);     // Load real
    status_t Inst_FST(uint32_t addr, AddressSpace& space);     // Store real
    status_t Inst_FLDZ();                                       // Load zero
    status_t Inst_FLD1();                                       // Load one
    status_t Inst_FLDPI();                                      // Load pi
    status_t Inst_FLDL2E();                                     // Load log2(e)
    status_t Inst_FLDL2T();                                     // Load log2(10)
    status_t Inst_FLDLG2();                                     // Load log10(2)
    status_t Inst_FLDLN2();                                     // Load ln(2)
    
    // Arithmetic instructions
    status_t Inst_FADD(int reg1, int reg2);
    status_t Inst_FSUB(int reg1, int reg2);
    status_t Inst_FMUL(int reg1, int reg2);
    status_t Inst_FDIV(int reg1, int reg2);
    status_t Inst_FSQRT();
    status_t Inst_FABS();
    status_t Inst_FCHS();
    status_t Inst_FREM();
    
    // Trigonometric instructions
    status_t Inst_FSIN();
    status_t Inst_FCOS();
    status_t Inst_FTAN();
    status_t Inst_FATAN2(int reg1, int reg2);
    
    // Logarithmic instructions
    status_t Inst_FLN();
    status_t Inst_FLN2();
    status_t Inst_FLOG();
    status_t Inst_FLOG2();
    status_t Inst_FYL2X();
    status_t Inst_FEXP();
    
    // Control instructions
    status_t Inst_FINIT();
    status_t Inst_FWAIT();
    status_t Inst_FSTENV(uint32_t addr, AddressSpace& space);
    status_t Inst_FLDENV(uint32_t addr, AddressSpace& space);
    status_t Inst_FSTSW(X86_32GuestContext& context);
    status_t Inst_FXAM();
    status_t Inst_FCOM(int reg1, int reg2);
    status_t Inst_FUCOM(int reg1, int reg2);
    
    // Helper to extract register field from ModRM
    int ExtractReg(uint8_t modrm) { return (modrm >> 3) & 0x7; }
    int ExtractRM(uint8_t modrm) { return modrm & 0x7; }
    int ExtractMod(uint8_t modrm) { return (modrm >> 6) & 0x3; }
};
