/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Generic Floating Point Unit interface
 * Provides platform-independent FPU abstraction
 * Can use host FPU directly or software emulation
 */

#pragma once

#include <SupportDefs.h>
#include <stdint.h>
#include <cmath>
#include <cstring>
#include <cstdlib>

/**
 * FPU State Register (FSW - Floating Point Status Word)
 * Maps to x87 status register
 */
struct FPUStatusWord {
    uint16_t invalid_operation : 1;  // IE
    uint16_t denormalized : 1;        // DE
    uint16_t zero_divide : 1;         // ZE
    uint16_t overflow : 1;            // OE
    uint16_t underflow : 1;           // UE
    uint16_t precision : 1;           // PE
    uint16_t stack_fault : 1;         // SF
    uint16_t error_summary : 1;       // ES
    uint16_t condition_code : 4;      // C0, C1, C2, C3
    uint16_t top : 3;                 // TOP (stack pointer)
    uint16_t busy : 1;                // B
    
    uint16_t AsUint16() const {
        return *(uint16_t*)this;
    }
    
    void FromUint16(uint16_t val) {
        *(uint16_t*)this = val;
    }
};

/**
 * FPU Control Register (FCW)
 * Controls rounding mode, exceptions, precision
 */
struct FPUControlWord {
    uint16_t invalid_mask : 1;    // IM
    uint16_t denorm_mask : 1;     // DM
    uint16_t zero_mask : 1;       // ZM
    uint16_t overflow_mask : 1;   // OM
    uint16_t underflow_mask : 1;  // UM
    uint16_t precision_mask : 1;  // PM
    uint16_t reserved : 2;
    uint16_t precision : 2;       // PC (0=24bit, 1=reserved, 2=53bit, 3=64bit)
    uint16_t rounding : 2;        // RC (0=nearest, 1=down, 2=up, 3=towards 0)
    uint16_t infinity : 1;        // X (0=proj, 1=affine)
    uint16_t reserved2 : 3;
    
    uint16_t AsUint16() const {
        return *(uint16_t*)this;
    }
    
    void FromUint16(uint16_t val) {
        *(uint16_t*)this = val;
    }
};

/**
 * 80-bit extended precision floating point value
 * Used by x87 FPU stack
 */
struct ExtendedDouble {
    uint64_t mantissa;
    uint16_t exponent_sign;  // 15 bits exponent + 1 sign bit
    
    double ToDouble() const {
        // Convert 80-bit extended to 64-bit double
        if (exponent_sign == 0 && mantissa == 0) {
            return 0.0;  // Zero
        }
        
        // Extract sign
        int sign = (exponent_sign >> 15) & 1;
        
        // Extract exponent (11 bits, stored in 15 bits of extended format)
        int exponent = (exponent_sign & 0x7FFF);
        
        // Build result: mantissa is normalized, exponent is biased
        double d = (double)mantissa;
        if (exponent > 0) {
            d *= std::pow(2.0, (double)(exponent - 63));
        }
        
        if (sign) {
            d = -d;
        }
        
        return d;
    }
    
    void FromDouble(double d) {
        // Convert 64-bit double to 80-bit extended
        if (d == 0.0) {
            mantissa = 0;
            exponent_sign = 0;
            return;
        }
        
        // Extract sign
        int sign = (d < 0) ? 1 : 0;
        d = std::abs(d);
        
        // Get exponent and normalize mantissa
        int exponent = 0;
        while (d >= 2.0) {
            d /= 2.0;
            exponent++;
        }
        while (d < 1.0 && d > 0) {
            d *= 2.0;
            exponent--;
        }
        
        // Store mantissa as 64-bit value
        mantissa = (uint64_t)(d * (1ULL << 63));
        
        // Store exponent + bias (80-bit bias = 16383)
        exponent_sign = ((exponent + 16383) & 0x7FFF) | ((sign & 1) << 15);
    }
    
    long double ToLongDouble() const {
        // Direct conversion on x86 platforms where long double = 80-bit
#if defined(__i386__) || defined(__x86_64__)
        long double result;
        std::memcpy(&result, this, sizeof(ExtendedDouble));
        return result;
#else
        // Fallback: convert via double
        return (long double)ToDouble();
#endif
    }
    
    void FromLongDouble(long double ld) {
        // Direct conversion on x86 platforms where long double = 80-bit
#if defined(__i386__) || defined(__x86_64__)
        std::memcpy(this, &ld, sizeof(ExtendedDouble));
#else
        // Fallback: convert from double
        FromDouble((double)ld);
#endif
    }
};

/**
 * Generic Floating Point Unit implementation
 * Abstracts FPU operations across platforms
 */
class FloatingPointUnit {
public:
    // Stack size for x87-style FPU
    static const int STACK_SIZE = 8;
    
    FloatingPointUnit();
    ~FloatingPointUnit();
    
    /**
     * Initialize FPU to default state
     */
    void Init();
    
    /**
     * Reset FPU (FINIT instruction)
     */
    void Reset();
    
    /**
     * Save FPU state (FSAVE/FSTENV)
     */
    void SaveState(void* state_buffer);
    
    /**
     * Restore FPU state (FRSTOR/FLDENV)
     */
    void RestoreState(const void* state_buffer);
    
    // Stack operations
    void Push(ExtendedDouble value);
    ExtendedDouble Pop();
    ExtendedDouble Peek(int index);  // index relative to TOP
    void SetStackValue(int index, ExtendedDouble value);
    
    // Status and control
    FPUStatusWord GetStatusWord() const { return fStatusWord; }
    void SetStatusWord(FPUStatusWord sw) { fStatusWord = sw; }
    
    FPUControlWord GetControlWord() const { return fControlWord; }
    void SetControlWord(FPUControlWord cw) { fControlWord = cw; }
    
    // Clear flags
    void ClearExceptions();
    void SetException(uint16_t flag);
    
    // Arithmetic operations (delegate to host FPU when possible)
    ExtendedDouble Add(ExtendedDouble a, ExtendedDouble b);
    ExtendedDouble Subtract(ExtendedDouble a, ExtendedDouble b);
    ExtendedDouble Multiply(ExtendedDouble a, ExtendedDouble b);
    ExtendedDouble Divide(ExtendedDouble a, ExtendedDouble b);
    ExtendedDouble SquareRoot(ExtendedDouble value);
    
    // Trigonometric
    ExtendedDouble Sin(ExtendedDouble value);
    ExtendedDouble Cos(ExtendedDouble value);
    ExtendedDouble Tan(ExtendedDouble value);
    ExtendedDouble ArcTan(ExtendedDouble value);
    
    // Logarithmic
    ExtendedDouble Log10(ExtendedDouble value);
    ExtendedDouble LogNatural(ExtendedDouble value);
    ExtendedDouble Power(ExtendedDouble base, ExtendedDouble exp);
    
    // Other operations
    ExtendedDouble Abs(ExtendedDouble value);
    ExtendedDouble Negate(ExtendedDouble value);
    ExtendedDouble Remainder(ExtendedDouble a, ExtendedDouble b);
    ExtendedDouble RoundToInt(ExtendedDouble value);
    
    // Comparisons (affect condition codes)
    void Compare(ExtendedDouble a, ExtendedDouble b);
    void Unordered(ExtendedDouble a, ExtendedDouble b);
    
    // Tag word (indicates stack slot content type)
    enum TagValue {
        TAG_VALID = 0,      // Valid number
        TAG_ZERO = 1,       // Zero
        TAG_SPECIAL = 2,    // NaN, infinity, denormal
        TAG_EMPTY = 3       // Empty
    };
    
    uint16_t GetTagWord() const { return fTagWord; }
    void SetTagWord(uint16_t tags) { fTagWord = tags; }
    
    TagValue GetTag(int index) const;
    void SetTag(int index, TagValue tag);
    
    // Check condition codes after comparison
    bool IsCondition0Set() const { return fStatusWord.condition_code & 0x1; }
    bool IsCondition1Set() const { return fStatusWord.condition_code & 0x2; }
    bool IsCondition2Set() const { return fStatusWord.condition_code & 0x4; }
    bool IsCondition3Set() const { return fStatusWord.condition_code & 0x8; }
    
private:
    ExtendedDouble fStack[STACK_SIZE];
    uint8_t fStackTop;
    
    FPUStatusWord fStatusWord;
    FPUControlWord fControlWord;
    uint16_t fTagWord;
    
    // Last instruction/operand pointers (for FSTENV/FSAVE)
    uint32_t fLastInstPtr;
    uint32_t fLastDataPtr;
    uint16_t fLastInstOpcode;
    
    void UpdateStackTop();
    void SetConditionCodes(uint8_t codes);
    
    // Helper to convert between ExtendedDouble and host double
    double ExtToDouble(const ExtendedDouble& ext) const;
    ExtendedDouble DoubleToExt(double d) const;
};

/**
 * FPU State Snapshot (for context switching)
 * Supports both x87 and modern SIMD state if needed
 */
struct FPUState {
    ExtendedDouble stack[FloatingPointUnit::STACK_SIZE];
    uint16_t status_word;
    uint16_t control_word;
    uint16_t tag_word;
    uint32_t last_inst_ptr;
    uint32_t last_data_ptr;
    uint16_t last_inst_opcode;
    
    // For x87 FSAVE format
    uint8_t opcode;
    uint32_t fds;  // FPU DS:FIP
    uint32_t ffo;  // FPU DS:FOP
};
