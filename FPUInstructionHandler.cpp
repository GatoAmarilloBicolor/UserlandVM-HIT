/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * FPU Instruction Handler implementation
 */

#include "FPUInstructionHandler.h"
#include "X86_32GuestContext.h"
#include "AddressSpace.h"
#include "DebugOutput.h"

#include <cstring>
#include <cmath>

FPUInstructionHandler::FPUInstructionHandler(FloatingPointUnit& fpu)
    : fFPU(fpu)
{
}

FPUInstructionHandler::~FPUInstructionHandler()
{
}

status_t FPUInstructionHandler::Execute(uint8_t opcode, uint8_t modrm,
                                        X86_32GuestContext& context,
                                        AddressSpace& addressSpace)
{
    DebugPrintf("[FPU] Executing instruction: opcode=0x%02x modrm=0x%02x\n", opcode, modrm);
    
    switch (opcode) {
        case FPU_CLASS_D8:
            return HandleD8(modrm, context, addressSpace);
        case FPU_CLASS_D9:
            return HandleD9(modrm, context, addressSpace);
        case FPU_CLASS_DA:
            return HandleDA(modrm, context, addressSpace);
        case FPU_CLASS_DB:
            return HandleDB(modrm, context, addressSpace);
        case FPU_CLASS_DC:
            return HandleDC(modrm, context, addressSpace);
        case FPU_CLASS_DD:
            return HandleDD(modrm, context, addressSpace);
        case FPU_CLASS_DE:
            return HandleDE(modrm, context, addressSpace);
        case FPU_CLASS_DF:
            return HandleDF(modrm, context, addressSpace);
        default:
            DebugPrintf("[FPU] ERROR: Invalid FPU opcode 0x%02x\n", opcode);
            return B_BAD_VALUE;
    }
}

// ============================================================================
// Class D8: Floating-point arithmetic operations (ESC 0)
// ============================================================================

status_t FPUInstructionHandler::HandleD8(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space)
{
    uint8_t mod = ExtractMod(modrm);
    uint8_t reg = ExtractReg(modrm);
    uint8_t rm = ExtractRM(modrm);
    
    // Memory operations (mod != 11)
    if (mod != 3) {
        // Extract memory address (would need SIB, displacement handling)
        DebugPrintf("[FPU] D8 memory operation: reg=%d\n", reg);
        // D8 /0 = FADD  m32real
        // D8 /1 = FMUL  m32real
        // D8 /4 = FSUB  m32real
        // D8 /5 = FSUBR m32real
        // D8 /6 = FDIV  m32real
        // D8 /7 = FDIVR m32real
        return B_OK;
    }
    
    // Register operations (mod == 11)
    // D8 /0 = FADD  ST(0), ST(i)
    // D8 /1 = FMUL  ST(0), ST(i)
    // D8 /4 = FSUB  ST(0), ST(i)
    // D8 /5 = FSUBR ST(0), ST(i)
    // D8 /6 = FDIV  ST(0), ST(i)
    // D8 /7 = FDIVR ST(0), ST(i)
    
    switch (reg) {
        case 0: return Inst_FADD(0, rm);
        case 1: return Inst_FMUL(0, rm);
        case 4: return Inst_FSUB(0, rm);
        case 5: return Inst_FSUB(rm, 0);  // Reverse
        case 6: return Inst_FDIV(0, rm);
        case 7: return Inst_FDIV(rm, 0);  // Reverse
        default:
            DebugPrintf("[FPU] D8: Unknown register operation %d\n", reg);
            return B_BAD_VALUE;
    }
}

// ============================================================================
// Class D9: Floating-point and integer transfers (ESC 1)
// ============================================================================

status_t FPUInstructionHandler::HandleD9(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space)
{
    uint8_t mod = ExtractMod(modrm);
    uint8_t reg = ExtractReg(modrm);
    uint8_t rm = ExtractRM(modrm);
    
    if (mod != 3) {
        // Memory operations
        // D9 /0 = FLD  m32real
        // D9 /2 = FST  m32real
        // D9 /3 = FSTP m32real
        // Would extract address from modrm here
        DebugPrintf("[FPU] D9 memory operation: reg=%d\n", reg);
        return B_OK;
    }
    
    // Register/No operand operations
    // D9 C0 + i = FLD ST(i)
    // D9 C8 + i = FXCH ST(i)
    // D9 E0 = FCHS
    // D9 E1 = FABS
    // D9 E4 = FTST
    // D9 E5 = FXAM
    // D9 E8 = FLD1
    // D9 E9 = FLDL2T
    // D9 EA = FLDL2E
    // D9 EB = FLDPI
    // D9 EC = FLDLG2
    // D9 ED = FLDLN2
    // D9 EE = FLDZ
    
    uint8_t byte = modrm;
    
    if ((byte & 0xF8) == 0xC0) {
        // FLD ST(i)
        DebugPrintf("[FPU] FLD ST(%d)\n", rm);
        return B_OK;
    }
    
    if ((byte & 0xF8) == 0xC8) {
        // FXCH ST(i)
        DebugPrintf("[FPU] FXCH ST(%d)\n", rm);
        return B_OK;
    }
    
    switch (byte) {
        case 0xE0: return Inst_FCHS();
        case 0xE1: return Inst_FABS();
        case 0xE8: return Inst_FLD1();
        case 0xE9: return Inst_FLDL2T();
        case 0xEA: return Inst_FLDL2E();
        case 0xEB: return Inst_FLDPI();
        case 0xEC: return Inst_FLDLG2();
        case 0xED: return Inst_FLDLN2();
        case 0xEE: return Inst_FLDZ();
        default:
            DebugPrintf("[FPU] D9: Unknown operation 0x%02x\n", byte);
            return B_BAD_VALUE;
    }
}

// ============================================================================
// Class DA: Integer arithmetic (ESC 2)
// ============================================================================

status_t FPUInstructionHandler::HandleDA(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space)
{
    uint8_t mod = ExtractMod(modrm);
    uint8_t reg = ExtractReg(modrm);
    
    if (mod != 3) {
        // Memory: 32-bit integers
        // DA /0 = FIADD m32int
        // DA /1 = FIMUL m32int
        // DA /4 = FISUB m32int
        // DA /5 = FISUBR m32int
        // DA /6 = FIDIV m32int
        // DA /7 = FIDIVR m32int
        DebugPrintf("[FPU] DA memory operation: reg=%d\n", reg);
        return B_OK;
    }
    
    // Register operations - compare
    DebugPrintf("[FPU] DA register operation: reg=%d\n", reg);
    return B_OK;
}

// ============================================================================
// Class DB: Floating-point compare and transcendental (ESC 3)
// ============================================================================

status_t FPUInstructionHandler::HandleDB(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space)
{
    uint8_t mod = ExtractMod(modrm);
    uint8_t reg = ExtractReg(modrm);
    
    if (mod != 3) {
        // Memory: 32-bit integers or extended
        DebugPrintf("[FPU] DB memory operation: reg=%d\n", reg);
        return B_OK;
    }
    
    // Register operations - transcendentals
    // DB E8 = FSIN
    // DB E9 = FCOS
    // DB F0 = FSINCOS
    // DB F4 = FXTRACT
    // DB F5 = FPREM1
    
    switch (modrm) {
        case 0xE8: return Inst_FSIN();
        case 0xE9: return Inst_FCOS();
        default:
            DebugPrintf("[FPU] DB: Unknown operation 0x%02x\n", modrm);
            return B_OK;
    }
}

// ============================================================================
// Class DC: Floating-point arithmetic (ESC 4)
// ============================================================================

status_t FPUInstructionHandler::HandleDC(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space)
{
    uint8_t mod = ExtractMod(modrm);
    uint8_t reg = ExtractReg(modrm);
    uint8_t rm = ExtractRM(modrm);
    
    if (mod != 3) {
        // Memory: 64-bit reals
        DebugPrintf("[FPU] DC memory operation: reg=%d\n", reg);
        return B_OK;
    }
    
    // DC C0 + i = FADD  ST(i), ST(0)
    // DC C8 + i = FMUL  ST(i), ST(0)
    // DC E0 + i = FSUB  ST(i), ST(0)
    // DC E8 + i = FSUBR ST(i), ST(0)
    // DC F0 + i = FDIV  ST(i), ST(0)
    // DC F8 + i = FDIVR ST(i), ST(0)
    
    uint8_t byte = modrm;
    
    if ((byte & 0xF8) == 0xC0) return Inst_FADD(rm, 0);
    if ((byte & 0xF8) == 0xC8) return Inst_FMUL(rm, 0);
    if ((byte & 0xF8) == 0xE0) return Inst_FSUB(rm, 0);
    if ((byte & 0xF8) == 0xE8) return Inst_FSUB(0, rm);
    if ((byte & 0xF8) == 0xF0) return Inst_FDIV(rm, 0);
    if ((byte & 0xF8) == 0xF8) return Inst_FDIV(0, rm);
    
    DebugPrintf("[FPU] DC: Unknown operation 0x%02x\n", byte);
    return B_BAD_VALUE;
}

// ============================================================================
// Class DD: Floating-point load/store (ESC 5)
// ============================================================================

status_t FPUInstructionHandler::HandleDD(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space)
{
    uint8_t mod = ExtractMod(modrm);
    uint8_t reg = ExtractReg(modrm);
    uint8_t rm = ExtractRM(modrm);
    
    if (mod != 3) {
        // Memory operations on 64-bit reals
        // DD /0 = FLD  m64real
        // DD /2 = FST  m64real
        // DD /3 = FSTP m64real
        DebugPrintf("[FPU] DD memory operation: reg=%d\n", reg);
        return B_OK;
    }
    
    // Register operations
    // DD C0 + i = FLD ST(i)
    // DD D0 + i = FST ST(i)
    // DD D8 + i = FSTP ST(i)
    // DD E0 + i = FUCOM ST(i)
    // DD E8 + i = FUCOMP ST(i)
    
    if ((modrm & 0xF8) == 0xC0) {
        DebugPrintf("[FPU] FLD ST(%d)\n", rm);
        return B_OK;
    }
    if ((modrm & 0xF8) == 0xD0) {
        DebugPrintf("[FPU] FST ST(%d)\n", rm);
        return B_OK;
    }
    if ((modrm & 0xF8) == 0xD8) {
        DebugPrintf("[FPU] FSTP ST(%d)\n", rm);
        return B_OK;
    }
    if ((modrm & 0xF8) == 0xE0) {
        return Inst_FUCOM(0, rm);
    }
    if ((modrm & 0xF8) == 0xE8) {
        return Inst_FUCOM(0, rm);
    }
    
    DebugPrintf("[FPU] DD: Unknown operation 0x%02x\n", modrm);
    return B_BAD_VALUE;
}

// ============================================================================
// Class DE: Floating-point arithmetic (ESC 6)
// ============================================================================

status_t FPUInstructionHandler::HandleDE(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space)
{
    uint8_t mod = ExtractMod(modrm);
    uint8_t reg = ExtractReg(modrm);
    uint8_t rm = ExtractRM(modrm);
    
    if (mod != 3) {
        // Memory: 16-bit integers
        DebugPrintf("[FPU] DE memory operation: reg=%d\n", reg);
        return B_OK;
    }
    
    // DE C0 + i = FADDP ST(i), ST(0)
    // DE C8 + i = FMULP ST(i), ST(0)
    // DE E0 + i = FSUBRP ST(i), ST(0)
    // DE E8 + i = FSUBP ST(i), ST(0)
    // DE F0 + i = FDIVRP ST(i), ST(0)
    // DE F8 + i = FDIVP ST(i), ST(0)
    
    uint8_t byte = modrm;
    
    if ((byte & 0xF8) == 0xC0) return Inst_FADD(rm, 0);
    if ((byte & 0xF8) == 0xC8) return Inst_FMUL(rm, 0);
    if ((byte & 0xF8) == 0xE0) return Inst_FSUB(0, rm);
    if ((byte & 0xF8) == 0xE8) return Inst_FSUB(rm, 0);
    if ((byte & 0xF8) == 0xF0) return Inst_FDIV(0, rm);
    if ((byte & 0xF8) == 0xF8) return Inst_FDIV(rm, 0);
    
    DebugPrintf("[FPU] DE: Unknown operation 0x%02x\n", byte);
    return B_BAD_VALUE;
}

// ============================================================================
// Class DF: Floating-point compare and load/store (ESC 7)
// ============================================================================

status_t FPUInstructionHandler::HandleDF(uint8_t modrm, X86_32GuestContext& context, AddressSpace& space)
{
    uint8_t mod = ExtractMod(modrm);
    uint8_t reg = ExtractReg(modrm);
    
    if (mod != 3) {
        // Memory: 16-bit integers, packed BCD, or 64-bit integers
        DebugPrintf("[FPU] DF memory operation: reg=%d\n", reg);
        return B_OK;
    }
    
    // DF E0 = FNSTSW AX
    if (modrm == 0xE0) {
        return Inst_FSTSW(context);
    }
    
    DebugPrintf("[FPU] DF: Unknown operation 0x%02x\n", modrm);
    return B_OK;
}

// ============================================================================
// Instruction Implementations
// ============================================================================

status_t FPUInstructionHandler::Inst_FADD(int reg1, int reg2)
{
    ExtendedDouble a = fFPU.Peek(reg1);
    ExtendedDouble b = fFPU.Peek(reg2);
    ExtendedDouble result = fFPU.Add(a, b);
    fFPU.SetStackValue(reg1, result);
    DebugPrintf("[FPU] FADD ST(%d), ST(%d)\n", reg1, reg2);
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FMUL(int reg1, int reg2)
{
    ExtendedDouble a = fFPU.Peek(reg1);
    ExtendedDouble b = fFPU.Peek(reg2);
    ExtendedDouble result = fFPU.Multiply(a, b);
    fFPU.SetStackValue(reg1, result);
    DebugPrintf("[FPU] FMUL ST(%d), ST(%d)\n", reg1, reg2);
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FSUB(int reg1, int reg2)
{
    ExtendedDouble a = fFPU.Peek(reg1);
    ExtendedDouble b = fFPU.Peek(reg2);
    ExtendedDouble result = fFPU.Subtract(a, b);
    fFPU.SetStackValue(reg1, result);
    DebugPrintf("[FPU] FSUB ST(%d), ST(%d)\n", reg1, reg2);
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FDIV(int reg1, int reg2)
{
    ExtendedDouble a = fFPU.Peek(reg1);
    ExtendedDouble b = fFPU.Peek(reg2);
    ExtendedDouble result = fFPU.Divide(a, b);
    fFPU.SetStackValue(reg1, result);
    DebugPrintf("[FPU] FDIV ST(%d), ST(%d)\n", reg1, reg2);
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FSQRT()
{
    ExtendedDouble val = fFPU.Peek(0);
    ExtendedDouble result = fFPU.SquareRoot(val);
    fFPU.SetStackValue(0, result);
    DebugPrintf("[FPU] FSQRT\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FABS()
{
    ExtendedDouble val = fFPU.Peek(0);
    ExtendedDouble result = fFPU.Abs(val);
    fFPU.SetStackValue(0, result);
    DebugPrintf("[FPU] FABS\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FCHS()
{
    ExtendedDouble val = fFPU.Peek(0);
    ExtendedDouble result = fFPU.Negate(val);
    fFPU.SetStackValue(0, result);
    DebugPrintf("[FPU] FCHS\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FREM()
{
    DebugPrintf("[FPU] FREM\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FSIN()
{
    ExtendedDouble val = fFPU.Peek(0);
    ExtendedDouble result = fFPU.Sin(val);
    fFPU.SetStackValue(0, result);
    DebugPrintf("[FPU] FSIN\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FCOS()
{
    ExtendedDouble val = fFPU.Peek(0);
    ExtendedDouble result = fFPU.Cos(val);
    fFPU.SetStackValue(0, result);
    DebugPrintf("[FPU] FCOS\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FTAN()
{
    ExtendedDouble val = fFPU.Peek(0);
    ExtendedDouble result = fFPU.Tan(val);
    fFPU.SetStackValue(0, result);
    DebugPrintf("[FPU] FTAN\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FATAN2(int reg1, int reg2)
{
    ExtendedDouble a = fFPU.Peek(reg1);
    ExtendedDouble b = fFPU.Peek(reg2);
    // Result of ATAN2 stored based on implementation
    DebugPrintf("[FPU] FATAN2\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLN()
{
    ExtendedDouble val = fFPU.Peek(0);
    ExtendedDouble result = fFPU.LogNatural(val);
    fFPU.SetStackValue(0, result);
    DebugPrintf("[FPU] FLN\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLN2()
{
    DebugPrintf("[FPU] FLN2\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLOG()
{
    ExtendedDouble val = fFPU.Peek(0);
    ExtendedDouble result = fFPU.Log10(val);
    fFPU.SetStackValue(0, result);
    DebugPrintf("[FPU] FLOG\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLOG2()
{
    DebugPrintf("[FPU] FLOG2\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FYL2X()
{
    DebugPrintf("[FPU] FYL2X\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FEXP()
{
    DebugPrintf("[FPU] FEXP\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FINIT()
{
    fFPU.Reset();
    DebugPrintf("[FPU] FINIT\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FWAIT()
{
    // Wait for FPU - in our case, no-op
    DebugPrintf("[FPU] FWAIT\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FSTENV(uint32_t addr, AddressSpace& space)
{
    FPUState state;
    fFPU.SaveState(&state);
    DebugPrintf("[FPU] FSTENV\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLDENV(uint32_t addr, AddressSpace& space)
{
    FPUState state;
    fFPU.RestoreState(&state);
    DebugPrintf("[FPU] FLDENV\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FSTSW(X86_32GuestContext& context)
{
    FPUStatusWord sw = fFPU.GetStatusWord();
    context.Registers().eax = (context.Registers().eax & 0xFF00) | sw.AsUint16();
    DebugPrintf("[FPU] FSTSW AX (status=0x%04x)\n", sw.AsUint16());
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FXAM()
{
    DebugPrintf("[FPU] FXAM\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FCOM(int reg1, int reg2)
{
    ExtendedDouble a = fFPU.Peek(reg1);
    ExtendedDouble b = fFPU.Peek(reg2);
    fFPU.Compare(a, b);
    DebugPrintf("[FPU] FCOM ST(%d), ST(%d)\n", reg1, reg2);
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FUCOM(int reg1, int reg2)
{
    ExtendedDouble a = fFPU.Peek(reg1);
    ExtendedDouble b = fFPU.Peek(reg2);
    fFPU.Unordered(a, b);
    DebugPrintf("[FPU] FUCOM ST(%d), ST(%d)\n", reg1, reg2);
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FILD(uint32_t addr, AddressSpace& space)
{
    DebugPrintf("[FPU] FILD at 0x%08x\n", addr);
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FISTORE(uint32_t addr, AddressSpace& space)
{
    DebugPrintf("[FPU] FISTORE at 0x%08x\n", addr);
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLD(uint32_t addr, AddressSpace& space)
{
    DebugPrintf("[FPU] FLD\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FST(uint32_t addr, AddressSpace& space)
{
    DebugPrintf("[FPU] FST\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLDZ()
{
    ExtendedDouble zero;
    zero.mantissa = 0;
    zero.exponent_sign = 0;
    fFPU.Push(zero);
    DebugPrintf("[FPU] FLDZ\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLD1()
{
    ExtendedDouble one;
    one.mantissa = 0x8000000000000000ULL;  // Mantissa for 1.0
    one.exponent_sign = 0x3FFF;             // Exponent for 1.0
    fFPU.Push(one);
    DebugPrintf("[FPU] FLD1\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLDPI()
{
    DebugPrintf("[FPU] FLDPI\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLDL2E()
{
    DebugPrintf("[FPU] FLDL2E\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLDL2T()
{
    DebugPrintf("[FPU] FLDL2T\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLDLG2()
{
    DebugPrintf("[FPU] FLDLG2\n");
    return B_OK;
}

status_t FPUInstructionHandler::Inst_FLDLN2()
{
    DebugPrintf("[FPU] FLDLN2\n");
    return B_OK;
}
