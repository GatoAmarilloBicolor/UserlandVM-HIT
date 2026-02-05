/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * HaikuRuntimeLoader.h - Haiku runtime_loader integration
 */

#ifndef HAIKU_RUNTIME_LOADER_H
#define HAIKU_RUNTIME_LOADER_H

#include <memory>
#include <string>
#include <vector>

// Forward declarations
class AddressSpace;
class DynamicLinker;
class ElfImage;

class HaikuRuntimeLoader {
public:
    HaikuRuntimeLoader(AddressSpace* addressSpace, DynamicLinker* linker);
    ~HaikuRuntimeLoader();

    // Runtime loader initialization
    bool Initialize();
    bool LoadRuntimeLoader();
    bool SetupCommpage();
    bool SetupTLS();
    
    // Process bootstrap
    bool BootstrapProcess(ElfImage* mainExecutable);
    bool RunPreInitializers();
    bool RunInitializers();
    
    // Runtime loader path resolution
    std::string GetRuntimeLoaderPath();
    bool IsRuntimeLoaderAvailable();
    
    // Haiku-specific setup
    bool SetupEnvironment();
    bool SetupStack();
    bool SetupArguments(int argc, char** argv);

private:
    AddressSpace* fAddressSpace;
    DynamicLinker* fDynamicLinker;
    ElfImage* fRuntimeLoader;
    
    std::string fRuntimeLoaderPath;
    std::vector<std::string> fEnvironment;
    std::vector<std::string> fArguments;
    
    bool fInitialized;
    bool fCommpageSetup;
    bool fTLSSetup;
    
    // Internal helper methods
    bool LoadRuntimeLoaderFromSysroot();
    bool ValidateRuntimeLoader(ElfImage* loader);
    bool SetupRuntimeLoaderSymbols();
    bool CreateHaikuEnvironment();
    uint32_t AllocateInitialStack(size_t size);
    void SetupAuxv(uint32_t* stackPtr, ElfImage* mainExecutable);
    
    // Constants
    static const char* const kDefaultRuntimeLoaderPath;
    static const char* const kSysrootBase;
    static const size_t kDefaultStackSize = 8 * 1024 * 1024; // 8MB
};

#endif // HAIKU_RUNTIME_LOADER_H