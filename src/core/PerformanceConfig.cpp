/*
 * PerformanceConfig.cpp - Implementation of performance configuration
 */

#include "PerformanceConfig.h"

// Static member initialization
bool PerformanceConfig::verbose_logging = false;
bool PerformanceConfig::production_mode = false;
bool PerformanceConfig::debug_mode = false;
bool PerformanceConfig::instruction_cache = true;
bool PerformanceConfig::optimized_mode = false;

void PerformanceConfig::Initialize() {
    // Check environment variables for configuration
    verbose_logging = (getenv("USERLANDVM_VERBOSE") != nullptr);
    production_mode = (getenv("USERLANDVM_PRODUCTION") != nullptr);
    debug_mode = (getenv("USERLANDVM_DEBUG") != nullptr);
    
    // Default production optimizations on unless debug is enabled
    optimized_mode = production_mode || !debug_mode;
    
    printf("[PERF] Config: verbose=%s, production=%s, debug=%s, optimized=%s\n",
           verbose_logging ? "ON" : "OFF",
           production_mode ? "ON" : "OFF", 
           debug_mode ? "ON" : "OFF",
           optimized_mode ? "ON" : "OFF");
}
