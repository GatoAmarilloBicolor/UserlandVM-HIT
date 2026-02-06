/*
 * UserlandVM Configuration Utility
 * Easy access to system configuration
 */

#pragma once

#include "UserlandVMConfig.h"
#include <iostream>
#include <memory>

// Configuration utility for easy access
class UserlandVM {
public:
    // Singleton access
    static UserlandVM& GetInstance();
    
    // Quick configuration access
    static bool Initialize();
    static bool IsConfigured();
    static bool IsHaikuSystem();
    static bool IsLinuxSystem();
    static bool Is32BitCompatible();
    
    // Library access
    static std::string GetSysrootPath();
    static std::string GetLibraryPath(const std::string& libraryName);
    static std::string GetExecutablePath(const std::string& executableName);
    static std::vector<std::string> GetLibrarySearchPaths();
    
    // System information
    static std::string GetSystemType();
    static std::string GetArchitecture();
    static std::string GetDistribution();
    static std::string GetVersion();
    
    // Configuration management
    static bool SetSysroot(const std::string& path);
    static bool AddLibraryPath(const std::string& path);
    static bool SetArchitecture(const std::string& arch);
    static bool LoadConfig(const char* configPath = nullptr);
    static bool SaveConfig(const char* configPath = nullptr);
    static void PrintConfiguration();
    static void PrintSystemInfo();
    static void PrintLibraryInfo(const std::string& libraryName = "");
    
    // Validation
    static bool ValidateConfiguration();
    static bool ValidateSysroot();
    static bool ValidateLibrary(const std::string& libraryName);
    
    // Convenience methods for 32-bit execution
    static bool ConfigureFor32BitExecution();
    static bool Has32BitLibraries();
    static bool Is32BitSysrootAvailable();
    static bool Setup32BitEnvironment();

private:
    UserlandVM() = default;
    ~UserlandVM() = default;
    
    static std::unique_ptr<UserlandVMConfig> fConfig;
    static bool fInitialized;
    
    static UserlandVMConfig& GetConfig();
    static bool EnsureInitialized();
};

// Inline implementations
inline UserlandVM& UserlandVM::GetInstance()
{
    static UserlandVM instance;
    return instance;
}

inline bool UserlandVM::Initialize()
{
    return EnsureInitialized();
}

inline bool UserlandVM::IsConfigured()
{
    return EnsureInitialized() && GetConfig().GetSysrootConfig().isValid;
}

inline bool UserlandVM::IsHaikuSystem()
{
    return EnsureInitialized() && GetConfig().GetSystemInfo().systemType == "Haiku";
}

inline bool UserlandVM::IsLinuxSystem()
{
    return EnsureInitialized() && GetConfig().GetSystemInfo().systemType == "Linux";
}

inline bool UserlandVM::Is32BitCompatible()
{
    return EnsureInitialized() && GetConfig().GetSystemInfo().isCompatible;
}

inline std::string UserlandVM::GetSysrootPath()
{
    if (EnsureInitialized()) {
        return GetConfig().GetSysrootConfig().rootPath;
    }
    return "";
}

inline std::string UserlandVM::GetLibraryPath(const std::string& libraryName)
{
    if (EnsureInitialized()) {
        const auto* lib = GetConfig().FindLibrary(libraryName);
        return lib ? lib->exactPath : "";
    }
    return "";
}

inline std::vector<std::string> UserlandVM::GetLibrarySearchPaths()
{
    return EnsureInitialized() ? GetConfig().GetLibrarySearchPaths() : std::vector<std::string>();
}

inline std::string UserlandVM::GetSystemType()
{
    return EnsureInitialized() ? GetConfig().GetSystemInfo().systemType : "";
}

inline std::string UserlandVM::GetArchitecture()
{
    return EnsureInitialized() ? GetConfig().GetSystemInfo().architecture : "";
}

inline std::string UserlandVM::GetDistribution()
{
    return EnsureInitialized() ? GetConfig().GetSystemInfo().distribution : "";
}

inline std::string UserlandVM::GetVersion()
{
    return EnsureInitialized() ? GetConfig().GetSystemInfo().version : "";
}

// Convenience class for 32-bit execution
class UserlandVM32Bit : public UserlandVM {
public:
    static bool Initialize();
    static bool Is32BitSysrootAvailable();
    static bool Setup32BitEnvironment();
    
    // 32-bit specific access
    static std::string Get32BitSysroot();
    static std::string Get32BitLibraryPath(const std::string& libraryName);
    static bool HasAll32BitLibraries();
    static bool Validate32BitConfiguration();
    static void Print32BitConfiguration();
};