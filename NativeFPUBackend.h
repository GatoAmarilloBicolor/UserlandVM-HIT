/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Native x87 FPU Backend
 * Direct access to host x87 FPU for arithmetic operations
 * Only available on x86/x86-64 hosts with x87 support
 */

#pragma once

#include <stdint.h>
#include <cmath>

/**
 * Extended double precision value (80-bit)
 * Used for x87 FPU operations
 */
struct ExtendedDouble {
    uint64_t mantissa;
    uint16_t exponent_sign;
};

/**
 * Base class for FPU backend implementations
 * Architecture-independent interface
 */
class FPUBackend {
public:
    virtual ~FPUBackend() = default;
    
    // Arithmetic operations
    virtual ExtendedDouble Add(ExtendedDouble a, ExtendedDouble b) = 0;
    virtual ExtendedDouble Subtract(ExtendedDouble a, ExtendedDouble b) = 0;
    virtual ExtendedDouble Multiply(ExtendedDouble a, ExtendedDouble b) = 0;
    virtual ExtendedDouble Divide(ExtendedDouble a, ExtendedDouble b) = 0;
    virtual ExtendedDouble SquareRoot(ExtendedDouble value) = 0;
    
    // Trigonometric
    virtual ExtendedDouble Sin(ExtendedDouble value) = 0;
    virtual ExtendedDouble Cos(ExtendedDouble value) = 0;
    virtual ExtendedDouble Tan(ExtendedDouble value) = 0;
    
    // Logarithmic
    virtual ExtendedDouble LogNatural(ExtendedDouble value) = 0;
    virtual ExtendedDouble Log10(ExtendedDouble value) = 0;
    virtual ExtendedDouble Power(ExtendedDouble base, ExtendedDouble exp) = 0;
};

/**
 * Native x87 FPU Backend
 * For x86/x86-64 hosts with native x87 support
 * 
 * Uses inline assembly to access the host x87 FPU directly
 * Provides near-zero overhead for arithmetic operations
 */
class NativeFPUBackend : public FPUBackend {
public:
    NativeFPUBackend();
    ~NativeFPUBackend() override;
    
    /**
     * Check if x87 is available on this platform
     */
    static bool IsAvailable();
    
    // Arithmetic operations
    ExtendedDouble Add(ExtendedDouble a, ExtendedDouble b) override;
    ExtendedDouble Subtract(ExtendedDouble a, ExtendedDouble b) override;
    ExtendedDouble Multiply(ExtendedDouble a, ExtendedDouble b) override;
    ExtendedDouble Divide(ExtendedDouble a, ExtendedDouble b) override;
    ExtendedDouble SquareRoot(ExtendedDouble value) override;
    
    // Trigonometric
    ExtendedDouble Sin(ExtendedDouble value) override;
    ExtendedDouble Cos(ExtendedDouble value) override;
    ExtendedDouble Tan(ExtendedDouble value) override;
    
    // Logarithmic
    ExtendedDouble LogNatural(ExtendedDouble value) override;
    ExtendedDouble Log10(ExtendedDouble value) override;
    ExtendedDouble Power(ExtendedDouble base, ExtendedDouble exp) override;
    
private:
    /**
     * Helper functions for x87 operations
     * Each uses inline assembly to access host FPU directly
     */
    
    // Low-level helper: loads extended double onto x87 stack
    static inline void LoadExtended(const ExtendedDouble& value);
    
    // Low-level helper: stores x87 stack top as extended double
    static inline ExtendedDouble StoreExtended();
    
    // Two-operand FPU instruction
    static inline ExtendedDouble BinaryOp(ExtendedDouble a, ExtendedDouble b,
                                         const char* instruction);
    
    // Single-operand FPU instruction
    static inline ExtendedDouble UnaryOp(ExtendedDouble value,
                                        const char* instruction);
};

/**
 * Software FPU Backend
 * Fallback for non-x86 platforms
 * Uses host C math library for arithmetic
 */
class SoftwareFPUBackend : public FPUBackend {
public:
    SoftwareFPUBackend();
    ~SoftwareFPUBackend() override;
    
    // Arithmetic operations using C math library
    ExtendedDouble Add(ExtendedDouble a, ExtendedDouble b) override;
    ExtendedDouble Subtract(ExtendedDouble a, ExtendedDouble b) override;
    ExtendedDouble Multiply(ExtendedDouble a, ExtendedDouble b) override;
    ExtendedDouble Divide(ExtendedDouble a, ExtendedDouble b) override;
    ExtendedDouble SquareRoot(ExtendedDouble value) override;
    
    // Trigonometric
    ExtendedDouble Sin(ExtendedDouble value) override;
    ExtendedDouble Cos(ExtendedDouble value) override;
    ExtendedDouble Tan(ExtendedDouble value) override;
    
    // Logarithmic
    ExtendedDouble LogNatural(ExtendedDouble value) override;
    ExtendedDouble Log10(ExtendedDouble value) override;
    ExtendedDouble Power(ExtendedDouble base, ExtendedDouble exp) override;
    
private:
    // Conversion helpers
    static double ExtToDouble(const ExtendedDouble& ext);
    static ExtendedDouble DoubleToExt(double d);
};

/**
 * Factory function to get the best available FPU backend
 * Returns NativeFPUBackend if x87 is available
 * Returns SoftwareFPUBackend as fallback
 */
FPUBackend* CreateOptimalFPUBackend();
