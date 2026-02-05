/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * FIXES for UserlandVM-HIT Critical Issues
 * Addresses memory management, platform detection, and structural problems
 */

#ifndef _USERLANDVM_FIXES_H
#define _USERLANDVM_FIXES_H

#include <stdint.h>

// Forward declarations for missing types
class ProgramContext;
class ExecutionEngine;
class DynamicLinker;
class GUIBackend;
class SymbolResolver;
class MemoryManager;
class Profiler;

// Platform-independent status codes
typedef enum {
    PLATFORM_OK = 0,
    PLATFORM_ERROR = -1,
    PLATFORM_WARNING = -2,
    PLATFORM_CRITICAL = -3
} platform_status_t;

// Critical fixes for UserlandVM-HIT critical issues
class UserlandVMFixes {
public:
    UserlandVMFixes();
    ~UserlandVMFixes() = default;
    
    // Fix 1: Memory Management - Ensure proper area initialization
    platform_status_t FixMemoryManagement();
    
    // Fix 2: Platform Detection - Ensure proper platform info
    platform_status_t FixPlatformDetection();
    
    // Fix 3: Structural Issues - Ensure proper component initialization
    platform_status_t FixStructuralIssues();
    
    // Fix 4: Error Handling - Proper error reporting
    platform_status_t FixErrorHandling();
    
    // Apply all fixes
    platform_status_t ApplyAllFixes();
    
    // Platform information methods
    const char* GetPlatformName();
    const char* GetPlatformConfig();
    
    // Error handling and reporting
    void SetupErrorHandlers();
    void GenerateCrashReport(platform_status_t error);
    
    // Component creation methods
    ExecutionEngine* CreateExecutionEngine(const char* platform_name);
    DynamicLinker* CreateDynamicLinker(const char* platform_name);
    MemoryManager* CreateMemoryManager(const char* platform_name);
    GUIBackend* CreateGUIBackend(const char* platform_name);

private:
    // Platform detection
    bool DetectPlatform(char* detected_platform);
    void StorePlatformInfo(const char* platform_name);
    
    // Memory management fixes
    bool ValidateMemoryState();
    void InitializeMemoryAreas();
    
    // Component validation
    bool ValidateComponentState();
    void InitializeComponents();
    
    // Error handling setup
    void InitializeSignalHandlers();
    void SetupCrashReporting();
    
    // Platform detection data
    char g_platformName[32];
    char g_platformConfig[64];
    bool g_platformDetected;
    
    // Error tracking
    int g_errorCount;
    platform_status_t g_lastError;
};

// Logging macros (simplified to avoid conflicts)
#define PLATFORM_DEBUG(...)    printf("[FIX][DEBUG] " __VA_ARGS__)
#define PLATFORM_INFO(...)     printf("[FIX][INFO] " __VA_ARGS__)
#define PLATFORM_SUCCESS(...)  printf("[FIX][SUCCESS] ✓ " __VA_ARGS__)
#define PLATFORM_WARNING(...)  printf("[FIX][WARNING] ⚠ " __VA_ARGS__)
#define PLATFORM_ERROR(...)    printf("[FIX][ERROR] ✗ " __VA_ARGS__)

#endif // _USERLANDVM_FIXES_H