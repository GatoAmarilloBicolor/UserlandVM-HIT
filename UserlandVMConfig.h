/*
 * UserlandVM Configuration
 * Detects and configures system-specific library paths
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>
#include <stdio.h>

// System detection and configuration
class UserlandVMConfig {
public:
    struct SystemInfo {
        std::string systemType;        // "Haiku", "Linux", "macOS", "Windows"
        std::string architecture;     // "x86", "x86_64", "arm64", "riscv64"
        std::string distribution;     // "Haiku R1", "Ubuntu", "Debian", "Fedora", etc.
        std::string version;          // OS version
        bool isCompatible;          // Can run x86-32 binaries
    };

    struct LibraryConfig {
        std::string libraryName;      // "libroot.so", "libbe.so", etc.
        std::vector<std::string> searchPaths;
        std::string exactPath;        // Full path if found
        std::string version;
        bool isSystemLibrary;        // Pre-installed vs user-provided
        std::string soname;         // Library soname
        size_t size;                 // Library file size
    };

    struct SysrootConfig {
        std::string rootPath;         // Base sysroot path
        std::string architecture;     // Target architecture ("x86", "x86_64")
        std::map<std::string, std::string> libraryPaths;
        std::map<std::string, std::string> binaryPaths;
        bool isValid;                  // Sysroot is valid and accessible
        std::string version;          // Sysroot version
    };

    UserlandVMConfig();
    ~UserlandVMConfig() = default;

    // Initialization
    bool Initialize(const char* configPath = nullptr);
    bool LoadFromEnvironment();
    bool DetectSystem();
    bool FindSysroot();
    bool ScanLibraries();

    // Configuration access
    const SystemInfo& GetSystemInfo() const;
    const SysrootConfig& GetSysrootConfig() const;
    const LibraryConfig* FindLibrary(const std::string& name) const;
    std::vector<std::string> GetLibrarySearchPaths() const;
    std::string GetExecutablePath(const std::string& name) const;

    // Runtime configuration
    bool SetLibraryPath(const std::string& libraryName, const std::string& path);
    bool AddSearchPath(const std::string& path);
    bool SetSysroot(const std::string& path);
    bool SetArchitecture(const std::string& arch);

    // Validation
    bool ValidateConfiguration();
    bool ValidateSysroot();
    bool ValidateLibrary(const std::string& name);

    // Save/Load
    bool SaveConfig(const char* configPath = nullptr);
    bool LoadConfig(const char* configPath = nullptr);
    void PrintConfiguration() const;

    // Environment helpers
    static std::string GetHomeDirectory();
    static std::string GetExecutablePath();
    static std::string GetCurrentDirectory();
    static bool FileExists(const std::string& path);
    static bool DirectoryExists(const std::string& path);

private:
    SystemInfo fSystemInfo;
    SysrootConfig fSysrootConfig;
    std::map<std::string, LibraryConfig> fLibraries;
    std::vector<std::string> fSearchPaths;
    std::string fConfigFile;

    // System-specific library detection
    bool DetectHaikuSystem();
    bool DetectLinuxSystem();
    bool DetectMacOSSystem();
    bool DetectWindowsSystem();

    // Haiku-specific configuration
    bool ConfigureHaikuLibraries();
    bool ScanHaikuSysroot(const std::string& basePath);
    bool FindHaikuLibraries();

    // Linux-specific configuration
    bool ConfigureLinuxLibraries();
    bool ScanLinuxSysroot(const std::string& basePath);
    bool FindLinuxLibraries();

    // Common library scanning
    bool ScanDirectoryForLibraries(const std::string& dirPath, const std::vector<std::string>& libraryNames);
    bool VerifyLibrary(const std::string& path, LibraryConfig& config);
    bool ReadLibraryInfo(const std::string& path, LibraryConfig& config);

    // Path manipulation
    std::string JoinPath(const std::string& base, const std::string& relative) const;
    std::string NormalizePath(const std::string& path) const;
    std::string GetParentDirectory(const std::string& path) const;

    // Configuration file handling
    bool CreateDefaultConfig();
    std::string GenerateConfigContent() const;
    bool ParseConfigContent(const std::string& content);

    // Constants for library detection
    static const std::vector<std::string> kHaikuCoreLibraries;
    static const std::vector<std::string> kLinuxCoreLibraries;
    static const std::vector<std::string> kCommonBinaryNames;

    // Target architecture libraries (for cross-compilation)
    struct TargetArchitecture {
        std::string name;
        std::string gccTriple;
        std::vector<std::string> libraryDirs;
        std::vector<std::string> binaryDirs;
    };

    static const std::map<std::string, TargetArchitecture> kTargetArchitectures;
};

// Platform-specific configurations
class HaikuConfig {
public:
    static const char* const kHaikuLibraryPaths[];
    static const char* const kHaikuBinaryPaths[];
    static const char* const kHaikuCoreLibraries[];
    static const std::map<std::string, std::string> kHaikuLibraryVersions;
};

class LinuxConfig {
public:
    static const std::vector<std::string> kStandardLibraryPaths;
    static const std::vector<std::string> kSystemLibraryPaths;
    static const std::vector<std::string> kLibrarySearchPaths;
    static const std::map<std::string, std::string> kLinuxLibraryPackages;
};

// Configuration constants
namespace UserlandVMConstants {
    // Default configurations for different systems
    extern const char* const DEFAULT_HAIKU_SYSROOT;
    extern const char* const DEFAULT_LINUX_SYSROOT;
    extern const char* const DEFAULT_CONFIG_FILE;
    
    // Supported architectures
    extern const char* const ARCH_X86;
    extern const char* const ARCH_X86_64;
    extern const char* const ARCH_ARM64;
    extern const char* const ARCH_RISCV64;
    
    // Critical library names
    extern const char* const LIB_ROOT;
    extern const char* const LIB_BE;
    extern const char* const LIB_NETWORK;
    extern const char* const LIB_MEDIA;
    extern const char* const LIB_TRACKER;
    extern const char* const LIB_GAME;
    extern const char* const LIB_OPENGL;
    extern const char* const LIB_STORAGE;
    extern const char* const LIB_DEVICE;
    extern const char* const LIB_INPUT;
    extern const char* const LIB_TEXTENCODING;
    
    // Configuration file sections
    extern const char* const SECTION_SYSTEM;
    extern const char* const SECTION_SYSROOT;
    extern const char* const SECTION_LIBRARIES;
    extern const char* const SECTION_PATHS;
    
    // Environment variables
    extern const char* const ENV_USERLANDVM_HOME;
    extern const char* const ENV_USERLANDVM_SYSROOT;
    extern const char* const ENV_USERLANDVM_ARCH;
    
    // Default file names
    extern const char* const CONFIG_FILENAME;
    extern const char* const SYSROOT_MARKER;
    extern const char* const LIBRARY_CACHE_FILE;
}