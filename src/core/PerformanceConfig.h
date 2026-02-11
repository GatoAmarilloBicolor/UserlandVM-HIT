/*
 * PerformanceConfig.h - Runtime Performance Configuration
 * 
 * Controls logging and performance features for production vs debug builds
 */

#pragma once

#include <cstdint>
#include <cstdlib>

class PerformanceConfig {
public:
    // Initialize configuration from environment variables
    static void Initialize();
    
    // Logging control
    static bool IsVerboseLoggingEnabled() { return verbose_logging; }
    static bool IsProductionModeEnabled() { return production_mode; }
    static bool IsDebugEnabled() { return debug_mode; }
    
    // Performance control
    static bool IsInstructionCacheEnabled() { return instruction_cache; }
    static bool IsOptimizedModeEnabled() { return optimized_mode; }
    
    // Runtime control
    static void SetVerboseLogging(bool enabled) { verbose_logging = enabled; }
    static void SetProductionMode(bool enabled) { production_mode = enabled; }
    static void SetDebugMode(bool enabled) { debug_mode = enabled; }
    
private:
    static bool verbose_logging;      // Detailed logging for debugging
    static bool production_mode;       // Production optimizations enabled
    static bool debug_mode;           // Debug-specific features enabled
    static bool instruction_cache;      // Instruction cache enabled
    static bool optimized_mode;        // General optimizations enabled
};

// Performance macros for conditional logging
#define LOG_VERBOSE(...) \
    do { if (PerformanceConfig::IsVerboseLoggingEnabled()) printf(__VA_ARGS__); } while(0)

#define LOG_DEBUG(...) \
    do { if (PerformanceConfig::IsDebugEnabled()) printf(__VA_ARGS__); } while(0)

#define LOG_ERROR(...) \
    do { printf(__VA_ARGS__); } while(0)  // Always log errors

#define LOG_GUI(...) \
    do { if (!PerformanceConfig::IsProductionModeEnabled()) printf(__VA_ARGS__); } while(0)

#define FAST_STRING_EQUALS(s1, s2) \
    ((s1)[0] == (s2)[0] && strcmp((s1), (s2)) == 0)

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x)   __builtin_expect(!!(x), 0)
