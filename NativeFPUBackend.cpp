/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Native x87 FPU Backend Implementation
 */

#include "NativeFPUBackend.h"
#include <cstring>
#include <memory>

// ============================================================================
// NativeFPUBackend - x87 Native Implementation
// ============================================================================

NativeFPUBackend::NativeFPUBackend()
{
    // Detect and initialize x87 if available
}

NativeFPUBackend::~NativeFPUBackend()
{
}

bool NativeFPUBackend::IsAvailable()
{
#ifdef __i386__
    return true;  // x87 always available on i386
#elif defined(__x86_64__)
    return true;  // x87 always available on x86-64
#else
    return false;
#endif
}

ExtendedDouble NativeFPUBackend::Add(ExtendedDouble a, ExtendedDouble b)
{
#if defined(__i386__) || defined(__x86_64__)
    long double lda, ldb, result;
    
    // Convert to long double (x87 native format)
    std::memcpy(&lda, &a, sizeof(ExtendedDouble));
    std::memcpy(&ldb, &b, sizeof(ExtendedDouble));
    
    // x87 will use native hardware
    result = lda + ldb;
    
    // Convert back
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    // Fallback for non-x86
    return SoftwareFPUBackend().Add(a, b);
#endif
}

ExtendedDouble NativeFPUBackend::Subtract(ExtendedDouble a, ExtendedDouble b)
{
#if defined(__i386__) || defined(__x86_64__)
    long double lda, ldb, result;
    std::memcpy(&lda, &a, sizeof(ExtendedDouble));
    std::memcpy(&ldb, &b, sizeof(ExtendedDouble));
    
    result = lda - ldb;
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().Subtract(a, b);
#endif
}

ExtendedDouble NativeFPUBackend::Multiply(ExtendedDouble a, ExtendedDouble b)
{
#if defined(__i386__) || defined(__x86_64__)
    long double lda, ldb, result;
    std::memcpy(&lda, &a, sizeof(ExtendedDouble));
    std::memcpy(&ldb, &b, sizeof(ExtendedDouble));
    
    result = lda * ldb;
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().Multiply(a, b);
#endif
}

ExtendedDouble NativeFPUBackend::Divide(ExtendedDouble a, ExtendedDouble b)
{
#if defined(__i386__) || defined(__x86_64__)
    long double lda, ldb, result;
    std::memcpy(&lda, &a, sizeof(ExtendedDouble));
    std::memcpy(&ldb, &b, sizeof(ExtendedDouble));
    
    result = lda / ldb;
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().Divide(a, b);
#endif
}

ExtendedDouble NativeFPUBackend::SquareRoot(ExtendedDouble value)
{
#if defined(__i386__) || defined(__x86_64__)
    long double ld, result;
    std::memcpy(&ld, &value, sizeof(ExtendedDouble));
    
    result = sqrtl(ld);
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().SquareRoot(value);
#endif
}

ExtendedDouble NativeFPUBackend::Sin(ExtendedDouble value)
{
#if defined(__i386__) || defined(__x86_64__)
    long double ld, result;
    std::memcpy(&ld, &value, sizeof(ExtendedDouble));
    
    result = sinl(ld);
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().Sin(value);
#endif
}

ExtendedDouble NativeFPUBackend::Cos(ExtendedDouble value)
{
#if defined(__i386__) || defined(__x86_64__)
    long double ld, result;
    std::memcpy(&ld, &value, sizeof(ExtendedDouble));
    
    result = cosl(ld);
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().Cos(value);
#endif
}

ExtendedDouble NativeFPUBackend::Tan(ExtendedDouble value)
{
#if defined(__i386__) || defined(__x86_64__)
    long double ld, result;
    std::memcpy(&ld, &value, sizeof(ExtendedDouble));
    
    result = tanl(ld);
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().Tan(value);
#endif
}

ExtendedDouble NativeFPUBackend::LogNatural(ExtendedDouble value)
{
#if defined(__i386__) || defined(__x86_64__)
    long double ld, result;
    std::memcpy(&ld, &value, sizeof(ExtendedDouble));
    
    result = logl(ld);
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().LogNatural(value);
#endif
}

ExtendedDouble NativeFPUBackend::Log10(ExtendedDouble value)
{
#if defined(__i386__) || defined(__x86_64__)
    long double ld, result;
    std::memcpy(&ld, &value, sizeof(ExtendedDouble));
    
    result = log10l(ld);
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().Log10(value);
#endif
}

ExtendedDouble NativeFPUBackend::Power(ExtendedDouble base, ExtendedDouble exp)
{
#if defined(__i386__) || defined(__x86_64__)
    long double ldb, lde, result;
    std::memcpy(&ldb, &base, sizeof(ExtendedDouble));
    std::memcpy(&lde, &exp, sizeof(ExtendedDouble));
    
    result = powl(ldb, lde);
    
    ExtendedDouble ext;
    std::memcpy(&ext, &result, sizeof(ExtendedDouble));
    return ext;
#else
    return SoftwareFPUBackend().Power(base, exp);
#endif
}

// ============================================================================
// SoftwareFPUBackend - Software Fallback Implementation
// ============================================================================

SoftwareFPUBackend::SoftwareFPUBackend()
{
}

SoftwareFPUBackend::~SoftwareFPUBackend()
{
}

double SoftwareFPUBackend::ExtToDouble(const ExtendedDouble& ext)
{
    if (ext.exponent_sign == 0 && ext.mantissa == 0) {
        return 0.0;
    }
    
    int sign = (ext.exponent_sign >> 15) & 1;
    int exponent = (ext.exponent_sign & 0x7FFF) - 16383;
    uint64_t mantissa = ext.mantissa;
    
    double d = (double)mantissa;
    if (exponent != -16383) {
        d *= std::pow(2.0, (double)(exponent - 63));
    }
    
    if (sign) {
        d = -d;
    }
    
    return d;
}

ExtendedDouble SoftwareFPUBackend::DoubleToExt(double d)
{
    ExtendedDouble ext;
    
    if (d == 0.0) {
        ext.mantissa = 0;
        ext.exponent_sign = 0;
        return ext;
    }
    
    int sign = (d < 0) ? 1 : 0;
    d = std::abs(d);
    
    int exponent = 0;
    while (d >= 2.0) {
        d /= 2.0;
        exponent++;
    }
    while (d < 1.0 && d > 0) {
        d *= 2.0;
        exponent--;
    }
    
    ext.mantissa = (uint64_t)(d * (1ULL << 63));
    ext.exponent_sign = ((exponent + 16383) & 0x7FFF) | ((sign & 1) << 15);
    
    return ext;
}

ExtendedDouble SoftwareFPUBackend::Add(ExtendedDouble a, ExtendedDouble b)
{
    return DoubleToExt(ExtToDouble(a) + ExtToDouble(b));
}

ExtendedDouble SoftwareFPUBackend::Subtract(ExtendedDouble a, ExtendedDouble b)
{
    return DoubleToExt(ExtToDouble(a) - ExtToDouble(b));
}

ExtendedDouble SoftwareFPUBackend::Multiply(ExtendedDouble a, ExtendedDouble b)
{
    return DoubleToExt(ExtToDouble(a) * ExtToDouble(b));
}

ExtendedDouble SoftwareFPUBackend::Divide(ExtendedDouble a, ExtendedDouble b)
{
    return DoubleToExt(ExtToDouble(a) / ExtToDouble(b));
}

ExtendedDouble SoftwareFPUBackend::SquareRoot(ExtendedDouble value)
{
    return DoubleToExt(std::sqrt(ExtToDouble(value)));
}

ExtendedDouble SoftwareFPUBackend::Sin(ExtendedDouble value)
{
    return DoubleToExt(std::sin(ExtToDouble(value)));
}

ExtendedDouble SoftwareFPUBackend::Cos(ExtendedDouble value)
{
    return DoubleToExt(std::cos(ExtToDouble(value)));
}

ExtendedDouble SoftwareFPUBackend::Tan(ExtendedDouble value)
{
    return DoubleToExt(std::tan(ExtToDouble(value)));
}

ExtendedDouble SoftwareFPUBackend::LogNatural(ExtendedDouble value)
{
    return DoubleToExt(std::log(ExtToDouble(value)));
}

ExtendedDouble SoftwareFPUBackend::Log10(ExtendedDouble value)
{
    return DoubleToExt(std::log10(ExtToDouble(value)));
}

ExtendedDouble SoftwareFPUBackend::Power(ExtendedDouble base, ExtendedDouble exp)
{
    return DoubleToExt(std::pow(ExtToDouble(base), ExtToDouble(exp)));
}

// ============================================================================
// Factory Function
// ============================================================================

FPUBackend* CreateOptimalFPUBackend()
{
    if (NativeFPUBackend::IsAvailable()) {
        return new NativeFPUBackend();
    } else {
        return new SoftwareFPUBackend();
    }
}
