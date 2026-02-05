/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * HaikuRuntimeLoader.cpp - Implementation of Haiku runtime_loader integration
 */

#include "HaikuRuntimeLoader.h"
#include "AddressSpace.h"
#include "DynamicLinker.h"
#include "Loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Constants
const char* const HaikuRuntimeLoader::kDefaultRuntimeLoaderPath = "sysroot/haiku32/system/runtime_loader";
const char* const HaikuRuntimeLoader::kSysrootBase = "sysroot/haiku32";

HaikuRuntimeLoader::HaikuRuntimeLoader(AddressSpace* addressSpace, DynamicLinker* linker)
    : fAddressSpace(addressSpace)
    , fDynamicLinker(linker)
    , fRuntimeLoader(nullptr)
    , fInitialized(false)
    , fCommpageSetup(false)
    , fTLSSetup(false)
{
}

HaikuRuntimeLoader::~HaikuRuntimeLoader() {
    // Cleanup handled by DynamicLinker
}

bool HaikuRuntimeLoader::Initialize() {
    if (fInitialized) {
        return true;
    }

    printf("[RUNTIME] Initializing Haiku runtime loader\n");

    if (!LoadRuntimeLoader()) {
        printf("[RUNTIME] Failed to load runtime_loader\n");
        return false;
    }

    if (!SetupCommpage()) {
        printf("[RUNTIME] Failed to setup commpage\n");
        return false;
    }

    if (!SetupTLS()) {
        printf("[RUNTIME] Failed to setup TLS\n");
        return false;
    }

    if (!SetupEnvironment()) {
        printf("[RUNTIME] Failed to setup environment\n");
        return false;
    }

    fInitialized = true;
    printf("[RUNTIME] Runtime loader initialized successfully\n");
    return true;
}

bool HaikuRuntimeLoader::LoadRuntimeLoader() {
    if (fRuntimeLoader) {
        return true;
    }

    fRuntimeLoaderPath = GetRuntimeLoaderPath();
    if (fRuntimeLoaderPath.empty()) {
        printf("[RUNTIME] No runtime loader path available\n");
        return false;
    }

    printf("[RUNTIME] Loading runtime loader from: %s\n", fRuntimeLoaderPath.c_str());

    fRuntimeLoader = fDynamicLinker->LoadLibrary(fRuntimeLoaderPath.c_str());
    if (!fRuntimeLoader) {
        printf("[RUNTIME] Failed to load runtime loader\n");
        return false;
    }

    if (!ValidateRuntimeLoader(fRuntimeLoader)) {
        printf("[RUNTIME] Invalid runtime loader\n");
        return false;
    }

    if (!SetupRuntimeLoaderSymbols()) {
        printf("[RUNTIME] Failed to setup runtime loader symbols\n");
        return false;
    }

    printf("[RUNTIME] Runtime loader loaded at %p\n", fRuntimeLoader->GetImageBase());
    return true;
}

bool HaikuRuntimeLoader::SetupCommpage() {
    if (fCommpageSetup) {
        return true;
    }

    // TODO: Setup Haiku commpage at fixed address (0xFFFF0000 for x86)
    printf("[RUNTIME] Setting up commpage (TODO)\n");
    fCommpageSetup = true;
    return true;
}

bool HaikuRuntimeLoader::SetupTLS() {
    if (fTLSSetup) {
        return true;
    }

    // TODO: Setup Thread Local Storage for Haiku
    printf("[RUNTIME] Setting up TLS (TODO)\n");
    fTLSSetup = true;
    return true;
}

bool HaikuRuntimeLoader::BootstrapProcess(ElfImage* mainExecutable) {
    if (!fInitialized) {
        if (!Initialize()) {
            return false;
        }
    }

    printf("[RUNTIME] Bootstrapping process\n");

    // Load required libraries
    if (!mainExecutable->LoadDynamicDependencies()) {
        printf("[RUNTIME] Failed to load dynamic dependencies\n");
        return false;
    }

    // Process relocations
    if (!mainExecutable->ProcessRelocations()) {
        printf("[RUNTIME] Failed to process relocations\n");
        return false;
    }

    // Run pre-initializers
    if (!RunPreInitializers()) {
        printf("[RUNTIME] Failed to run pre-initializers\n");
        return false;
    }

    printf("[RUNTIME] Process bootstrapped successfully\n");
    return true;
}

bool HaikuRuntimeLoader::RunPreInitializers() {
    // TODO: Run Haiku pre-initialization functions
    printf("[RUNTIME] Running pre-initializers (TODO)\n");
    return true;
}

bool HaikuRuntimeLoader::RunInitializers() {
    // TODO: Run Haiku initialization functions (.init sections)
    printf("[RUNTIME] Running initializers (TODO)\n");
    return true;
}

bool HaikuRuntimeLoader::SetupEnvironment() {
    return CreateHaikuEnvironment();
}

bool HaikuRuntimeLoader::SetupStack() {
    uint32_t stackBase = AllocateInitialStack(kDefaultStackSize);
    if (stackBase == 0) {
        printf("[RUNTIME] Failed to allocate stack\n");
        return false;
    }

    printf("[RUNTIME] Stack allocated at 0x%08x\n", stackBase);
    return true;
}

bool HaikuRuntimeLoader::SetupArguments(int argc, char** argv) {
    fArguments.clear();
    for (int i = 0; i < argc; i++) {
        fArguments.push_back(argv[i]);
    }

    printf("[RUNTIME] Arguments setup: %d arguments\n", argc);
    return true;
}

std::string HaikuRuntimeLoader::GetRuntimeLoaderPath() {
    // Check if runtime loader exists in sysroot
    struct stat st;
    if (stat(kDefaultRuntimeLoaderPath, &st) == 0) {
        return kDefaultRuntimeLoaderPath;
    }

    // Try alternative paths
    const char* altPaths[] = {
        "sysroot/haiku32/boot/system/runtime_loader",
        "sysroot/haiku32/system/lib/runtime_loader",
        "/boot/system/runtime_loader",  // If running on Haiku
        nullptr
    };

    for (int i = 0; altPaths[i]; i++) {
        if (stat(altPaths[i], &st) == 0) {
            return altPaths[i];
        }
    }

    printf("[RUNTIME] Runtime loader not found\n");
    return "";
}

bool HaikuRuntimeLoader::IsRuntimeLoaderAvailable() {
    return !GetRuntimeLoaderPath().empty();
}

bool HaikuRuntimeLoader::LoadRuntimeLoaderFromSysroot() {
    return LoadRuntimeLoader();
}

bool HaikuRuntimeLoader::ValidateRuntimeLoader(ElfImage* loader) {
    // TODO: Validate that this is actually Haiku's runtime_loader
    // Check for required symbols like "__start", etc.
    return loader != nullptr;
}

bool HaikuRuntimeLoader::SetupRuntimeLoaderSymbols() {
    // TODO: Export runtime loader symbols to the process
    return true;
}

bool HaikuRuntimeLoader::CreateHaikuEnvironment() {
    // Setup basic Haiku environment variables
    fEnvironment.push_back("BEOS=1");
    fEnvironment.push_back("HAIKU=1");
    
    // Add current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        fEnvironment.push_back(std::string("PWD=") + cwd);
    }

    printf("[RUNTIME] Environment setup with %zu variables\n", fEnvironment.size());
    return true;
}

uint32_t HaikuRuntimeLoader::AllocateInitialStack(size_t size) {
    // TODO: Allocate stack in address space
    // For now, return a dummy address
    return 0x10000000; // 256MB dummy address
}

void HaikuRuntimeLoader::SetupAuxv(uint32_t* stackPtr, ElfImage* mainExecutable) {
    // TODO: Setup auxiliary vector for Haiku
    // This includes program headers, entry point, etc.
}