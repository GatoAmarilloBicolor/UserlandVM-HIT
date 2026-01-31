/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Generic Floating Point Unit implementation
 * Pure abstraction layer - delegates arithmetic to backend
 */

#include "FloatingPointUnit.h"
#include <cstring>
#include <cmath>

FloatingPointUnit::FloatingPointUnit()
    : fStackTop(0)
{
    // Initialize status word
    fStatusWord.FromUint16(0x0000);
    
    // Initialize control word to defaults
    // PC = 3 (64-bit), RC = 0 (nearest), all exceptions masked
    fControlWord.FromUint16(0x037F);
    
    // Tag word = all empty initially
    fTagWord = 0xFFFF;
    
    // Initialize stack
    for (int i = 0; i < STACK_SIZE; ++i) {
        fStack[i].mantissa = 0;
        fStack[i].exponent_sign = 0;
    }
    
    // Instruction tracking
    fLastInstPtr = 0;
    fLastDataPtr = 0;
    fLastInstOpcode = 0;
}

FloatingPointUnit::~FloatingPointUnit()
{
}

void FloatingPointUnit::Init()
{
    // Reset to default state
    fStackTop = 0;
    fStatusWord.FromUint16(0x0000);
    fControlWord.FromUint16(0x037F);
    fTagWord = 0xFFFF;
    
    for (int i = 0; i < STACK_SIZE; ++i) {
        fStack[i].mantissa = 0;
        fStack[i].exponent_sign = 0;
    }
}

void FloatingPointUnit::Reset()
{
    // FINIT - Initialize FPU
    Init();
}

void FloatingPointUnit::SaveState(void* state_buffer)
{
    if (!state_buffer) return;
    
    FPUState* state = (FPUState*)state_buffer;
    
    // Copy stack
    for (int i = 0; i < STACK_SIZE; ++i) {
        state->stack[i] = fStack[i];
    }
    
    // Copy registers
    state->status_word = fStatusWord.AsUint16();
    state->control_word = fControlWord.AsUint16();
    state->tag_word = fTagWord;
    state->last_inst_ptr = fLastInstPtr;
    state->last_data_ptr = fLastDataPtr;
    state->last_inst_opcode = fLastInstOpcode;
}

void FloatingPointUnit::RestoreState(const void* state_buffer)
{
    if (!state_buffer) return;
    
    const FPUState* state = (const FPUState*)state_buffer;
    
    // Restore stack
    for (int i = 0; i < STACK_SIZE; ++i) {
        fStack[i] = state->stack[i];
    }
    
    // Restore registers
    fStatusWord.FromUint16(state->status_word);
    fControlWord.FromUint16(state->control_word);
    fTagWord = state->tag_word;
    fLastInstPtr = state->last_inst_ptr;
    fLastDataPtr = state->last_data_ptr;
    fLastInstOpcode = state->last_inst_opcode;
    
    // Stack top from status word
    fStackTop = fStatusWord.top;
}

void FloatingPointUnit::Push(ExtendedDouble value)
{
    // Check for stack overflow
    if (fStackTop >= STACK_SIZE) {
        // Stack fault - set exception
        fStatusWord.stack_fault = 1;
        fStatusWord.error_summary = 1;
        return;
    }
    
    // Decrement TOP (wraps with & 7)
    fStatusWord.top = (fStatusWord.top - 1) & 0x7;
    fStackTop = fStatusWord.top;
    
    // Push value onto stack
    fStack[fStackTop] = value;
    
    // Update tag - assume VALID for now (should check for special values)
    SetTag(fStackTop, TAG_VALID);
}

ExtendedDouble FloatingPointUnit::Pop()
{
    // Check for stack underflow
    if (fStatusWord.top == 0 && fStackTop == 0) {
        // Stack fault
        fStatusWord.stack_fault = 1;
        fStatusWord.error_summary = 1;
        return ExtendedDouble{0, 0};
    }
    
    // Get value from top of stack
    ExtendedDouble result = fStack[fStackTop];
    
    // Mark as empty
    SetTag(fStackTop, TAG_EMPTY);
    
    // Increment TOP (wraps with & 7)
    fStatusWord.top = (fStatusWord.top + 1) & 0x7;
    fStackTop = fStatusWord.top;
    
    return result;
}

ExtendedDouble FloatingPointUnit::Peek(int index)
{
    // Get value relative to TOP
    // index 0 = ST(0) = TOP
    // index 1 = ST(1) = (TOP + 1) & 7, etc.
    
    if (index < 0 || index >= STACK_SIZE) {
        return ExtendedDouble{0, 0};
    }
    
    int physical_index = (fStatusWord.top + index) & 0x7;
    return fStack[physical_index];
}

void FloatingPointUnit::SetStackValue(int index, ExtendedDouble value)
{
    if (index < 0 || index >= STACK_SIZE) {
        return;
    }
    
    int physical_index = (fStatusWord.top + index) & 0x7;
    fStack[physical_index] = value;
    
    // Update tag
    SetTag(physical_index, TAG_VALID);
}

void FloatingPointUnit::ClearExceptions()
{
    fStatusWord.invalid_operation = 0;
    fStatusWord.denormalized = 0;
    fStatusWord.zero_divide = 0;
    fStatusWord.overflow = 0;
    fStatusWord.underflow = 0;
    fStatusWord.precision = 0;
    fStatusWord.error_summary = 0;
}

void FloatingPointUnit::SetException(uint16_t flag)
{
    // Set individual exception flag
    if (flag & 0x01) fStatusWord.invalid_operation = 1;
    if (flag & 0x02) fStatusWord.denormalized = 1;
    if (flag & 0x04) fStatusWord.zero_divide = 1;
    if (flag & 0x08) fStatusWord.overflow = 1;
    if (flag & 0x10) fStatusWord.underflow = 1;
    if (flag & 0x20) fStatusWord.precision = 1;
    
    // Set error summary if any exception not masked
    uint16_t exceptions = flag & 0x3F;
    uint16_t masks = fControlWord.AsUint16() & 0x3F;
    if (exceptions & ~masks) {
        fStatusWord.error_summary = 1;
    }
}

// Arithmetic operations - pure delegation pattern
ExtendedDouble FloatingPointUnit::Add(ExtendedDouble a, ExtendedDouble b)
{
    // Convert to host doubles, add, convert back
    double ad = ExtToDouble(a);
    double bd = ExtToDouble(b);
    double result = ad + bd;
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::Subtract(ExtendedDouble a, ExtendedDouble b)
{
    double ad = ExtToDouble(a);
    double bd = ExtToDouble(b);
    double result = ad - bd;
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::Multiply(ExtendedDouble a, ExtendedDouble b)
{
    double ad = ExtToDouble(a);
    double bd = ExtToDouble(b);
    double result = ad * bd;
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::Divide(ExtendedDouble a, ExtendedDouble b)
{
    double ad = ExtToDouble(a);
    double bd = ExtToDouble(b);
    
    if (bd == 0.0) {
        SetException(0x04);  // Zero divide exception
    }
    
    double result = ad / bd;
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::SquareRoot(ExtendedDouble value)
{
    double d = ExtToDouble(value);
    double result = std::sqrt(d);
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::Sin(ExtendedDouble value)
{
    double d = ExtToDouble(value);
    double result = std::sin(d);
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::Cos(ExtendedDouble value)
{
    double d = ExtToDouble(value);
    double result = std::cos(d);
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::Tan(ExtendedDouble value)
{
    double d = ExtToDouble(value);
    double result = std::tan(d);
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::ArcTan(ExtendedDouble value)
{
    double d = ExtToDouble(value);
    double result = std::atan(d);
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::Log10(ExtendedDouble value)
{
    double d = ExtToDouble(value);
    double result = std::log10(d);
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::LogNatural(ExtendedDouble value)
{
    double d = ExtToDouble(value);
    double result = std::log(d);
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::Power(ExtendedDouble base, ExtendedDouble exp)
{
    double b = ExtToDouble(base);
    double e = ExtToDouble(exp);
    double result = std::pow(b, e);
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::Abs(ExtendedDouble value)
{
    // Clear sign bit
    ExtendedDouble result = value;
    result.exponent_sign &= 0x7FFF;
    return result;
}

ExtendedDouble FloatingPointUnit::Negate(ExtendedDouble value)
{
    // Toggle sign bit
    ExtendedDouble result = value;
    result.exponent_sign ^= 0x8000;
    return result;
}

ExtendedDouble FloatingPointUnit::Remainder(ExtendedDouble a, ExtendedDouble b)
{
    double ad = ExtToDouble(a);
    double bd = ExtToDouble(b);
    double result = std::remainder(ad, bd);
    return DoubleToExt(result);
}

ExtendedDouble FloatingPointUnit::RoundToInt(ExtendedDouble value)
{
    double d = ExtToDouble(value);
    double result;
    
    // Apply rounding mode from control word
    switch (fControlWord.rounding) {
        case 0:  // Round to nearest
            result = std::round(d);
            break;
        case 1:  // Round down
            result = std::floor(d);
            break;
        case 2:  // Round up
            result = std::ceil(d);
            break;
        case 3:  // Round towards zero
            result = std::trunc(d);
            break;
        default:
            result = std::round(d);
    }
    
    return DoubleToExt(result);
}

void FloatingPointUnit::Compare(ExtendedDouble a, ExtendedDouble b)
{
    double ad = ExtToDouble(a);
    double bd = ExtToDouble(b);
    
    // Clear condition codes
    SetConditionCodes(0);
    
    if (std::isnan(ad) || std::isnan(bd)) {
        // Unordered comparison
        SetConditionCodes(0x0F);  // All condition codes set
        SetException(0x01);  // Invalid operation
    } else if (ad < bd) {
        SetConditionCodes(0x01);  // C0=1, C1=0, C2=0, C3=0 (less than)
    } else if (ad > bd) {
        SetConditionCodes(0x00);  // All zeros (greater than)
    } else {
        // Equal
        SetConditionCodes(0x04);  // C2=1, others=0 (equal)
    }
}

void FloatingPointUnit::Unordered(ExtendedDouble a, ExtendedDouble b)
{
    // FUCOM - unordered comparison
    double ad = ExtToDouble(a);
    double bd = ExtToDouble(b);
    
    if (std::isnan(ad) || std::isnan(bd)) {
        SetConditionCodes(0x0F);
    } else {
        Compare(a, b);
    }
}

FloatingPointUnit::TagValue FloatingPointUnit::GetTag(int index) const
{
    if (index < 0 || index >= STACK_SIZE) {
        return TAG_EMPTY;
    }
    
    // Tag word is 2 bits per register
    // Bits 1:0 = ST(0), 3:2 = ST(1), etc.
    int shift = (index * 2);
    uint16_t tag_bits = (fTagWord >> shift) & 0x3;
    
    return (TagValue)tag_bits;
}

void FloatingPointUnit::SetTag(int index, TagValue tag)
{
    if (index < 0 || index >= STACK_SIZE) {
        return;
    }
    
    int shift = (index * 2);
    uint16_t mask = ~(0x3 << shift);
    fTagWord = (fTagWord & mask) | ((uint16_t)tag << shift);
}

void FloatingPointUnit::UpdateStackTop()
{
    fStackTop = fStatusWord.top;
}

void FloatingPointUnit::SetConditionCodes(uint8_t codes)
{
    // CC field is 4 bits: C0, C1, C2, C3
    fStatusWord.condition_code = codes & 0x0F;
}

// Helper conversions
double FloatingPointUnit::ExtToDouble(const ExtendedDouble& ext) const
{
    // Simple conversion from 80-bit extended to 64-bit double
    // In a real implementation, this would properly handle
    // denormals, special values, etc.
    
    if (ext.exponent_sign == 0 && ext.mantissa == 0) {
        return 0.0;  // Zero
    }
    
    // Extract sign
    int sign = (ext.exponent_sign >> 15) & 1;
    
    // Extract exponent (11 bits, but we use 15 bits from the format)
    int exponent = (ext.exponent_sign & 0x7FFF);
    
    // Mantissa
    uint64_t mantissa = ext.mantissa;
    
    // Build a double-compatible representation
    // For now, simple approximation
    double d = (double)mantissa;
    if (exponent > 0) {
        d *= std::pow(2.0, (double)(exponent - 63));
    }
    
    if (sign) {
        d = -d;
    }
    
    return d;
}

ExtendedDouble FloatingPointUnit::DoubleToExt(double d) const
{
    ExtendedDouble ext;
    
    if (d == 0.0) {
        ext.mantissa = 0;
        ext.exponent_sign = 0;
        return ext;
    }
    
    // Extract sign
    int sign = (d < 0) ? 1 : 0;
    d = std::abs(d);
    
    // Get exponent
    int exponent = 0;
    while (d >= 2.0) {
        d /= 2.0;
        exponent++;
    }
    while (d < 1.0 && d > 0) {
        d *= 2.0;
        exponent--;
    }
    
    // Mantissa
    ext.mantissa = (uint64_t)(d * (1ULL << 63));
    
    // Exponent + bias (for 80-bit: bias = 16383)
    ext.exponent_sign = ((exponent + 16383) & 0x7FFF) | ((sign & 1) << 15);
    
    return ext;
}
