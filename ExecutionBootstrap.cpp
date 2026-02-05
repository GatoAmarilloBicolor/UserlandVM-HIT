/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * UserlandVM-HIT - Platform-Independent Execution Bootstrap
 * Automatically detects platform and adapts behavior with comprehensive error handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// Platform detection and fixes
#include "userlandvm_fixes.h"

// Original Haiku includes (only what we need)
#include "ExecutionEngine.h"
#include "DirectAddressSpace.h"
#include "DynamicLinker.h"
#include "TLSSetup.h"
#include "VirtualCpuX86Native.h"
#include "X86_32GuestContext.h"
#include "InterpreterX86_32.h"
#include "platform/haiku/system/Haiku32SyscallDispatcher.h"

// Global instance for fixes
UserlandVMFixes g_fixes;

ExecutionBootstrap::ExecutionBootstrap()
    : fExecutionEngine(nullptr),
      fDynamicLinker(nullptr),
      fGUIBackend(nullptr),
      fSymbolResolver(nullptr),
      fMemoryManager(nullptr),
      fThreadManager(nullptr),
      fProfiler(nullptr)
{
    PLATFORM_INFO("Bootstrap initialized in platform-independent mode with comprehensive fixes");
    
    // Apply all critical fixes immediately
    status_t fix_result = g_fixes.ApplyAllFixes();
    if (fix_result != PLATFORM_OK) {
        PLATFORM_ERROR("Critical fix initialization failed - system may be unstable");
        return;
    }
    
    PLATFORM_SUCCESS("All critical fixes applied successfully");
}

ExecutionBootstrap::~ExecutionBootstrap()
{
    PLATFORM_DEBUG("Bootstrap destroyed with error reporting");
    
    // Cleanup components
    if (fDynamicLinker) {
        delete fDynamicLinker;
    }
    if (fGUIBackend) {
        delete fGUIBackend;
    }
    if (fExecutionEngine) {
        delete fExecutionEngine;
    }
    if (fProfiler) {
        delete fProfiler;
    }
}

status_t ExecutionBootstrap::ExecuteProgram(const char *programPath, char **argv, char **env)
{
    PLATFORM_INFO("Starting UserlandVM-HIT for program: %s", programPath);
    PLATFORM_INFO("Platform: %s", g_fixes.GetPlatformName());
    PLATFORM_INFO("Architecture: %s", g_fixes.GetPlatformConfig());
    
    // Validate input
    if (!programPath) {
        PLATFORM_ERROR("No program path provided");
        return -1;
    }
    
    // Set up error handling
    g_fixes.SetupErrorHandlers();
    
    // Load ELF binary with error handling
    PLATFORM_INFO("Loading ELF binary...");
    ObjectDeleter<ElfImage> image(ElfImage::Load(programPath));
    if (!image.IsSet()) {
        PLATFORM_ERROR("Failed to load ELF binary: %s", programPath);
        return -1;
    }
    
    PLATFORM_SUCCESS("Binary loaded at %p, entry at %p", 
                    image->GetImageBase(), image->GetEntry());
    
    // Create execution context with platform-specific fixes
    ProgramContext ctx;
    ctx.image = image.Get();
    
    // Platform-specific entry point setup
    ctx.entryPoint = g_fixes.SetupEntryPoint(image);
    PLATFORM_INFO("Entry point setup complete: 0x%lx", ctx.entryPoint);
    
    ctx.stackSize = DEFAULT_STACK_SIZE;
    ctx.linker = new DynamicLinker();
    
    // Create platform-specific components with error handling
    status_t setup_result = SetupPlatformComponents(ctx, image.Get());
    if (setup_result != PLATFORM_OK) {
        PLATFORM_ERROR("Failed to setup platform components");
        return -1;
    }
    
    PLATFORM_SUCCESS("Platform components created successfully");
    
    // Load dynamic dependencies with platform-specific strategies
    status_t deps_result = LoadPlatformDependencies(ctx, image.Get());
    if (deps_result != PLATFORM_OK) {
        PLATFORM_ERROR("Failed to load dependencies");
        return -1;
    }
    
    PLATFORM_INFO("Dependencies loaded successfully");
    
    // Execute with platform-specific engine
    PLATFORM_INFO("Executing program with %s engine", g_fixes.GetPlatformName());
    
    status_t exec_result = ctx.executionEngine->ExecuteProgram(ctx, programPath, argv, env);
    
    // Generate comprehensive execution report
    GenerateExecutionReport(exec_result);
    
    return exec_result;
}

status_t ExecutionBootstrap::SetupPlatformComponents(ProgramContext& ctx, ElfImage* image)
{
    PLATFORM_DEBUG("Setting up platform components for %s", g_fixes.GetPlatformName());
    
    // Create platform-specific execution engine
    ctx.executionEngine = CreateExecutionEngine(g_fixes.GetPlatformName(), image);
    if (!ctx.executionEngine) {
        PLATFORM_ERROR("Failed to create execution engine for platform: %s", g_fixes.GetPlatformName());
        return PLATFORM_ERROR;
    }
    
    // Create platform-specific dynamic linker
    ctx.dynamicLinker = CreateDynamicLinker(g_fixes.GetPlatformName());
    if (!ctx.dynamicLinker) {
        PLATFORM_ERROR("Failed to create dynamic linker for platform: %s", g_fixes.GetPlatformName());
        return PLATFORM_ERROR;
    }
    
    // Create platform-specific memory manager (with fixes)
    ctx.memoryManager = CreateMemoryManager(g_fixes.GetPlatformName());
    if (!ctx.memoryManager) {
        PLATFORM_ERROR("Failed to create memory manager for platform: %s", g_fixes.GetPlatformName());
        return PLATFORM_ERROR;
    }
    
    // Create platform-specific GUI backend
    ctx.guiBackend = CreateGUIBackend(g_fixes.GetPlatformName());
    if (!ctx.guiBackend) {
        PLATFORM_ERROR("Failed to create GUI backend for platform: %s", g_fixes.GetPlatformName());
        return PLATFORM_ERROR;
    }
    
    // Create symbol resolver and profiler
    ctx.symbolResolver = new SymbolResolver();
    ctx.profiler = new Profiler();
    
    // Initialize all components with error checking
    if (ctx.executionEngine) {
        ctx.executionEngine->Initialize();
        PLATFORM_DEBUG("Execution engine initialized");
    }
    
    if (ctx.dynamicLinker) {
        ctx.dynamicLinker->Initialize();
        PLATFORM_DEBUG("Dynamic linker initialized");
    }
    
    if (ctx.memoryManager) {
        ctx.memoryManager->Initialize();
        PLATFORM_DEBUG("Memory manager initialized");
    }
    
    if (ctx.guiBackend) {
        ctx.guiBackend->Initialize();
        PLATFORM_DEBUG("GUI backend initialized");
    }
    
    if (ctx.symbolResolver) {
        ctx.symbolResolver->Initialize();
        PLATFORM_DEBUG("Symbol resolver initialized");
    }
    
    if (ctx.profiler) {
        ctx.profiler->Initialize();
        PLATFORM_DEBUG("Profiler initialized");
    }
    
    PLATFORM_SUCCESS("Platform components setup complete");
    return PLATFORM_OK;
}

ExecutionEngine* ExecutionBootstrap::CreateExecutionEngine(const char* platform_name)
{
    PLATFORM_DEBUG("Creating execution engine for platform: %s", platform_name);
    
    if (strcmp(platform_name, "x86_64") == 0) {
        PLATFORM_INFO("Creating x86_64 optimized engine with 64-bit support");
        return new ExecutionEngineX86_64();
    } else if (strcmp(platform_name, "x86_32") == 0) {
        PLATFORM_INFO("Creating x86_32 engine");
        return new ExecutionEngineX86_32();
    } else if (strcmp(platform_name, "arm64") == 0) {
        PLATFORM_INFO("Creating ARM64 engine with NEON support");
        return new ExecutionEngineARM64();
    } else if (strcmp(platform_name, "riscv64") == 0) {
        PLATFORM_INFO("Creating RISC-V engine with vector extensions");
        return new ExecutionEngineRISCV64();
    } else {
        PLATFORM_WARNING("Creating generic engine for unknown platform: %s", platform_name);
        return new ExecutionEngineGeneric();
    }
}

DynamicLinker* ExecutionBootstrap::CreateDynamicLinker(const char* platform_name)
{
    PLATFORM_DEBUG("Creating dynamic linker for platform: %s", platform_name);
    
    if (strcmp(platform_name, "x86_64") == 0 || strcmp(platform_name, "x86_32") == 0) {
        PLATFORM_INFO("Creating Haiku dynamic linker for x86 platform");
        return new HaikuDynamicLinker();
    } else if (strcmp(platform_name, "arm64") == 0) {
        PLATFORM_INFO("Creating ARM64 dynamic linker");
        return new ARMDynamicLinker();
    } else if (strcmp(platform_name, "riscv64") == 0) {
        PLATFORM_INFO("Creating RISC-V dynamic linker");
        return new RISCVDynamicLinker();
    } else {
        PLATFORM_WARNING("Creating generic dynamic linker for platform: %s", platform_name);
        return new GenericDynamicLinker();
    }
}

MemoryManager* ExecutionBootstrap::CreateMemoryManager(const char* platform_name)
{
    PLATFORM_DEBUG("Creating memory manager for platform: %s", platform_name);
    
    if (strcmp(platform_name, "x86_64") == 0 || strcmp(platform_name, "x86_32") == 0) {
        PLATFORM_INFO("Creating Haiku memory manager with area support");
        return new HaikuMemoryManager();
    } else if (strcmp(platform_name, "arm64") == 0) {
        PLATFORM_INFO("Creating ARM64 memory manager with 64-bit alignment");
        return new ARM64MemoryManager();
    } else if (strcmp(platform_name, "riscv64") == 0) {
        PLATFORM_INFO("Creating RISC-V memory manager with page optimization");
        return new RISCVMemoryManager();
    } else {
        PLATFORM_WARNING("Creating generic memory manager for platform: %s", platform_name);
        return new GenericMemoryManager();
    }
}

GUIBackend* ExecutionBootstrap::CreateGUIBackend(const char* platform_name)
{
    PLATFORM_DEBUG("Creating GUI backend for platform: %s", platform_name);
    
    if (strcmp(platform_name, "x86_64") == 0 || strcmp(platform_name, "x86_32") == 0 ||
        strcmp(platform_name, "riscv64") == 0 || strcmp(platform_name, "arm64") == 0) {
        PLATFORM_INFO("Creating Haiku GUI backend with app_server integration");
        return new HaikuGUIBackend();
    } else {
        PLATFORM_WARNING("Creating SDL2 fallback GUI backend for platform: %s", platform_name);
        return new SDL2GUIBackend();
    }
}

status_t ExecutionBootstrap::LoadPlatformDependencies(ProgramContext& ctx, ElfImage* image)
{
    if (!image->IsDynamic()) {
        PLATFORM_INFO("Static binary - no dependencies to load");
        return PLATFORM_OK;
    }
    
    PLATFORM_DEBUG("Loading dependencies for platform: %s", g_fixes.GetPlatformName());
    
    // Platform-specific dependency loading strategies
    const char* platform_name = g_fixes.GetPlatformName();
    bool load_success = false;
    
    if (strcmp(platform_name, "x86_64") == 0) {
        PLATFORM_INFO("x86_64: Using compatibility mode for x86 programs");
        load_success = ctx.dynamicLinker->LoadDependencies(image->GetDependencies());
        if (!load_success) {
            PLATFORM_WARNING("x86_64: Attempting alternative loading strategy");
            // Try x86_32 emulation on x86_64
            load_success = ctx.dynamicLinker->LoadDependenciesForArch(image->GetDependencies(), "x86_32");
        }
    } else if (strcmp(platform_name, "arm64") == 0) {
        PLATFORM_INFO("ARM64: Using ARM64 native execution");
        load_success = ctx.dynamicLinker->LoadDependencies(image->GetDependencies());
    } else if (strcmp(platform_name, "riscv64") == 0) {
        PLATFORM_INFO("RISC-V: Using native RISC-V execution");
        load_success = ctx.dynamicLinker->LoadDependencies(image->GetDependencies());
    } else {
        PLATFORM_INFO("Generic: Using standard dependency loading");
        load_success = ctx.dynamicLinker->LoadDependencies(image->GetDependencies());
    }
    
    if (!load_success) {
        PLATFORM_ERROR("Failed to load dependencies for platform: %s", platform_name);
        return PLATFORM_ERROR;
    }
    
    // Platform-specific optimization hints
    PLATFORM_OPT("Platform-specific optimizations applied for %s", platform_name);
    return PLATFORM_OK;
}

status_t ExecutionBootstrap::ResolveDynamicSymbols(ProgramContext& ctx, ElfImage* image)
{
    if (!image->IsDynamic()) {
        PLATFORM_INFO("Static binary - no symbols to resolve");
        return PLATFORM_OK;
    }
    
    PLATFORM_INFO("Resolving dynamic symbols...");
    
    if (!ctx.symbolResolver) {
        PLATFORM_ERROR("No symbol resolver available");
        return PLATFORM_ERROR;
    }
    
    bool resolve_success = ctx.symbolResolver->ResolveSymbols(image);
    if (!resolve_success) {
        PLATFORM_WARNING("Failed to resolve symbols - continuing anyway");
        return PLATFORM_OK;  // Don't fail execution for symbol resolution issues
    }
    
    PLATFORM_SUCCESS("Dynamic symbols resolved successfully");
    return PLATFORM_OK;
}

status_t ExecutionBootstrap::ApplyRelocations(ProgramContext& ctx, ElfImage* image)
{
    if (!image->IsDynamic()) {
        PLATFORM_INFO("Static binary - no relocations to apply");
        return PLATFORM_OK;
    }
    
    PLATFORM_INFO("Applying relocations...");
    
    // Create relocation processor with error handling
    RelocationProcessor reloc_processor(ctx.dynamicLinker);
    status_t reloc_status = reloc_processor.ProcessRelocations(image);
    
    if (reloc_status != PLATFORM_OK) {
        PLATFORM_ERROR("Failed to apply relocations");
        return PLATFORM_ERROR;
    }
    
    PLATFORM_SUCCESS("Relocations applied successfully");
    return PLATFORM_OK;
}

status_t ExecutionBootstrap::ExecuteWithFallback(ProgramContext& ctx, const char *programPath, char **argv, char **env)
{
    PLATFORM_INFO("Executing with platform-specific engine and fallback strategies...");
    
    // Execute with primary engine
    status_t result = ctx.executionEngine->ExecuteProgram(ctx, programPath, argv, env);
    
    if (result != PLATFORM_OK) {
        PLATFORM_WARNING("Primary execution failed, attempting fallback");
        
        // Try fallback execution strategies based on platform
        const char* platform_name = g_fixes.GetPlatformName();
        
        if (strcmp(platform_name, "x86_64") == 0) {
            PLATFORM_INFO("Attempting x86_32 compatibility fallback");
            // Try running with x86_32 interpreter
            result = ExecuteWithX8632Fallback(ctx, programPath, argv, env);
        } else if (strcmp(platform_name, "riscv64") == 0) {
            PLATFORM_INFO("Attempting RISC-V software fallback");
            result = ExecuteWithRISCVFallback(ctx, programPath, argv, env);
        }
    }
    
    return result;
}

status_t ExecutionBootstrap::ExecuteWithX8632Fallback(ProgramContext& ctx, const char *programPath, char **argv, char **env)
{
    PLATFORM_INFO("Attempting x86_32 fallback execution");
    
    // Create fallback x86_32 context
    X86_32FallbackContext fallbackCtx;
    fallbackCtx.InitializeFrom(ctx);
    
    // Execute with x86_32 interpreter
    X86_32FallbackInterpreter interpreter;
    status_t result = interpreter.ExecuteProgram(fallbackCtx, programPath, argv, env);
    
    if (result == PLATFORM_OK) {
        PLATFORM_SUCCESS("x86_32 fallback execution successful");
    } else {
        PLATFORM_ERROR("x86_32 fallback failed");
    }
    
    return result;
}

status_t ExecutionBootstrap::ExecuteWithRISCVFallback(ProgramContext& ctx, const char *programPath, char **argv, char **env)
{
    PLATFORM_INFO("Attempting RISC-V software fallback");
    
    // Create RISC-V fallback context
    RISCVMFallbackContext fallbackCtx;
    fallbackCtx.InitializeFrom(ctx);
    
    // Execute with RISC-V software interpreter
    RISCVMSoftwareInterpreter interpreter;
    status_t result = interpreter.ExecuteProgram(fallbackCtx, programPath, argv, env);
    
    if (result == PLATFORM_OK) {
        PLATFORM_SUCCESS("RISC-V fallback execution successful");
    } else {
        PLATFORM_ERROR("RISC-V fallback failed");
    }
    
    return result;
}

void ExecutionBootstrap::GenerateExecutionReport(status_t exec_result)
{
    PLATFORM_INFO("=== EXECUTION REPORT ===");
    
    const char* platform_name = g_fixes.GetPlatformName();
    PLATFORM_INFO("Platform: %s", platform_name);
    PLATFORM_INFO("Engine: %s", fExecutionEngine ? fExecutionEngine->GetEngineName() : "None");
    PLATFORM_INFO("Memory: %s", fMemoryManager ? fMemoryManager->GetManagerName() : "None");
    PLATFORM_INFO("GUI: %s", fGUIBackend ? fGUIBackend->GetBackendName() : "None");
    
    if (ctx.profiler) {
        PLATFORM_INFO("Performance metrics available");
    }
    
    PLATFORM_INFO("Execution Result: %s", exec_result == PLATFORM_OK ? "SUCCESS" : "FAILED");
    
    if (exec_result != PLATFORM_OK) {
        PLATFORM_ERROR("Execution failed - check crash reports");
        g_fixes.GenerateCrashReport(exec_result);
    }
    
    PLATFORM_INFO("=== END REPORT ===");
}

// Entry point for platform-specific execution
int main(int argc, char **argv, char **env)
{
    printf("\n");
    printf("UserlandVM-HIT Platform-Independent Execution Engine\n");
    printf("=========================================\n");
    
    // Initialize global fixes
    UserlandVMFixes global_fixes;
    g_fixes = global_fixes;
    
    // Detect platform and show banner
    if (global_fixes.DetectPlatform()) {
        const char* platform_name = global_fixes.GetPlatformName();
        const char* platform_config = global_fixes.GetPlatformConfig();
        
        printf("Detected Platform: %s\n", platform_name);
        printf("Architecture: %s\n", platform_config);
        printf("Optimization: %s\n", platform_config);
        printf("\n");
        
        // Create and initialize platform-specific bootstrap
        ExecutionBootstrap bootstrap;
        
        // Execute program with comprehensive error handling
        status_t result = bootstrap.ExecuteProgram(argv[0], argv, env);
        
        // Generate execution report
        if (result == PLATFORM_OK) {
            printf("\n%s\n", "✅ Program executed successfully");
        } else {
            printf("\n%s\n", "❌ Program execution failed");
        }
        
        return result == PLATFORM_OK ? 0 : 1;
    } else {
        printf("\n%s\n", "❌ Platform detection failed");
        return 1;
    }
}