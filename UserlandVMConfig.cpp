/*
 * UserlandVM Configuration Implementation
 * Auto-detects system and configures sysroot paths
 */

#include "UserlandVMConfig.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>

// System detection and library configuration
UserlandVMConfig::UserlandVMConfig()
{
    fConfigFile = "";
}

bool UserlandVMConfig::Initialize(const char* configPath)
{
    printf("[CONFIG] Initializing UserlandVM Configuration\n");
    
    // Detect system information
    if (!DetectSystem()) {
        printf("[CONFIG] ERROR: Failed to detect system\n");
        return false;
    }
    
    printf("[CONFIG] System: %s %s (%s)\n", 
           fSystemInfo.systemType.c_str(),
           fSystemInfo.architecture.c_str(),
           fSystemInfo.isCompatible ? "Compatible" : "Incompatible");
    
    if (!fSystemInfo.isCompatible) {
        printf("[CONFIG] ERROR: System is not compatible with x86-32 execution\n");
        return false;
    }
    
    // Load configuration from environment
    LoadFromEnvironment();
    
    // Find sysroot
    if (!FindSysroot()) {
        printf("[CONFIG] ERROR: Failed to find valid sysroot\n");
        return false;
    }
    
    printf("[CONFIG] Sysroot: %s (%s)\n", 
           fSysrootConfig.rootPath.c_str(),
           fSysrootConfig.architecture.c_str());
    
    // Scan libraries
    if (!ScanLibraries()) {
        printf("[CONFIG] WARNING: Library scanning failed\n");
    }
    
    // Validate configuration
    if (!ValidateConfiguration()) {
        printf("[CONFIG] ERROR: Configuration validation failed\n");
        return false;
    }
    
    printf("[CONFIG] Configuration initialized successfully\n");
    return true;
}

bool UserlandVMConfig::LoadFromEnvironment()
{
    // Check environment variables
    const char* homeEnv = getenv(ENV_USERLANDVM_HOME);
    if (homeEnv) {
        fSearchPaths.push_back(std::string(homeEnv) + "/lib");
        fSearchPaths.push_back(std::string(homeEnv) + "/bin");
    }
    
    const char* sysrootEnv = getenv(ENV_USERLANDVM_SYSROOT);
    if (sysrootEnv) {
        fSysrootConfig.rootPath = sysrootEnv;
        printf("[CONFIG] Using sysroot from environment: %s\n", sysrootEnv);
        return true;
    }
    
    const char* archEnv = getenv(ENV_USERLANDVM_ARCH);
    if (archEnv) {
        fSysrootConfig.architecture = archEnv;
        printf("[CONFIG] Using architecture from environment: %s\n", archEnv);
    }
    
    return false;
}

bool UserlandVMConfig::DetectSystem()
{
    // Detect operating system
#if defined(__HAIKU__)
    return DetectHaikuSystem();
#elif defined(__linux__)
    return DetectLinuxSystem();
#elif defined(__APPLE__)
    return DetectMacOSSystem();
#elif defined(_WIN32)
    return DetectWindowsSystem();
#else
    // Default to Linux
    return DetectLinuxSystem();
#endif
}

bool UserlandVMConfig::DetectHaikuSystem()
{
    printf("[CONFIG] Detecting Haiku system\n");
    
    fSystemInfo.systemType = "Haiku";
    fSystemInfo.isCompatible = true;
    
    // Get Haiku version
    FILE* versionFile = fopen("/boot/system/settings/haiku/version", "r");
    if (versionFile) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), versionFile)) {
            fSystemInfo.version = std::string(buffer);
            fSystemInfo.version.erase(fSystemInfo.version.find_last_not_of("\r\n"));
        }
        fclose(versionFile);
    }
    
    // Detect architecture
    fSystemInfo.architecture = ARCH_X86;  // Assume x86 for UserlandVM
    
    fSystemInfo.distribution = "Haiku R1";
    
    printf("[CONFIG] Detected Haiku %s (x86)\n", fSystemInfo.version.c_str());
    return true;
}

bool UserlandVMConfig::DetectLinuxSystem()
{
    printf("[CONFIG] Detecting Linux system\n");
    
    fSystemInfo.systemType = "Linux";
    
    // Get system info
    FILE* osRelease = fopen("/etc/os-release", "r");
    if (osRelease) {
        char line[256];
        while (fgets(line, sizeof(line), osRelease)) {
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                fSystemInfo.distribution = std::string(line + 12);
                fSystemInfo.distribution.erase(fSystemInfo.distribution.find_last_not_of("\r\n"));
                
                // Clean up quotes
                if (fSystemInfo.distribution[0] == '"') {
                    fSystemInfo.distribution = fSystemInfo.distribution.substr(1, fSystemInfo.distribution.length() - 2);
                }
                break;
            }
        }
        fclose(osRelease);
    }
    
    // Detect architecture
    fSystemInfo.architecture = ARCH_X86;  // Default for UserlandVM
    fSystemInfo.isCompatible = true;
    
    printf("[CONFIG] Detected Linux %s (x86)\n", fSystemInfo.distribution.c_str());
    return true;
}

bool UserlandVMConfig::FindSysroot()
{
    printf("[CONFIG] Searching for valid sysroot...\n");
    
    // Environment variable takes precedence
    if (!fSysrootConfig.rootPath.empty()) {
        return true;
    }
    
    // Look for sysroot markers
    std::vector<std::string> possibleRoots = {
        "sysroot",
        "sysroot/haiku32",
        "../sysroot/haiku32",
        "../../sysroot/haiku32",
        "/boot/home/src/UserlandVM-HIT/sysroot/haiku32",
        "/usr/local/share/userlandvm/sysroot/haiku32",
        GetHomeDirectory() + "/.userlandvm/sysroot/haiku32"
    };
    
    for (const std::string& rootPath : possibleRoots) {
        if (ScanHaikuSysroot(rootPath)) {
            fSysrootConfig.rootPath = rootPath;
            fSysrootConfig.isValid = true;
            printf("[CONFIG] Found valid sysroot: %s\n", rootPath.c_str());
            return true;
        }
    }
    
    printf("[CONFIG] No valid sysroot found\n");
    return false;
}

bool UserlandVMConfig::ScanHaikuSysroot(const std::string& basePath)
{
    std::string fullPath = NormalizePath(basePath);
    
    // Check if directory exists
    struct stat statInfo;
    if (stat(fullPath.c_str(), &statInfo) != 0 || !S_ISDIR(statInfo.st_mode)) {
        return false;
    }
    
    // Check for sysroot marker
    std::string markerPath = JoinPath(fullPath, SYSROOT_MARKER);
    if (FileExists(markerPath)) {
        printf("[CONFIG] Found sysroot marker: %s\n", markerPath.c_str());
    }
    
    // Check for essential directories
    std::vector<std::string> requiredDirs = {
        "lib",
        "system/lib",
        "boot/system/lib",
        "bin",
        "system/bin"
    };
    
    bool hasRequiredDirs = true;
    for (const std::string& dir : requiredDirs) {
        std::string dirPath = JoinPath(fullPath, dir);
        if (DirectoryExists(dirPath)) {
            printf("[CONFIG] Found directory: %s\n", dirPath.c_str());
        } else {
            printf("[CONFIG] Missing directory: %s\n", dirPath.c_str());
            hasRequiredDirs = false;
        }
    }
    
    // Check for critical libraries
    std::vector<std::string> criticalLibs = {
        "libroot.so",
        "libbe.so",
        "libnetwork.so"
    };
    
    bool hasCriticalLibs = true;
    for (const std::string& lib : criticalLibs) {
        bool found = false;
        
        for (const std::string& searchPath : kHaikuLibraryPaths) {
            std::string libPath = JoinPath(fullPath, searchPath);
            libPath = JoinPath(libPath, lib);
            
            if (FileExists(libPath)) {
                printf("[CONFIG] Found library: %s\n", libPath.c_str());
                found = true;
                break;
            }
        }
        
        if (!found) {
            printf("[CONFIG] Missing library: %s\n", lib.c_str());
            hasCriticalLibs = false;
        }
    }
    
    fSysrootConfig.isValid = hasRequiredDirs && hasCriticalLibs;
    fSysrootConfig.architecture = ARCH_X86;
    fSysrootConfig.version = "UserlandVM-HAiku32";
    
    return fSysrootConfig.isValid;
}

bool UserlandVMConfig::ScanLibraries()
{
    printf("[CONFIG] Scanning libraries in sysroot...\n");
    
    if (!fSysrootConfig.isValid || fSysrootConfig.rootPath.empty()) {
        return false;
    }
    
    // Get library list for current system
    const std::vector<std::string>* libraryNames;
    if (fSystemInfo.systemType == "Haiku") {
        libraryNames = &kHaikuCoreLibraries;
    } else {
        libraryNames = &kLinuxCoreLibraries;
    }
    
    fLibraries.clear();
    
    // Scan all library paths
    for (const std::string& searchPath : kHaikuLibraryPaths) {
        std::string fullPath = JoinPath(fSysrootConfig.rootPath, searchPath);
        
        if (!DirectoryExists(fullPath)) {
            printf("[CONFIG] Library directory not found: %s\n", fullPath.c_str());
            continue;
        }
        
        printf("[CONFIG] Scanning library directory: %s\n", fullPath.c_str());
        
        for (const std::string& libName : *libraryNames) {
            LibraryConfig config;
            config.libraryName = libName;
            config.searchPaths.push_back(fullPath);
            
            std::string libPath = JoinPath(fullPath, libName);
            if (VerifyLibrary(libPath, config)) {
                fLibraries[libName] = config;
                fSysrootConfig.libraryPaths[libName] = libPath;
                printf("[CONFIG] Found library: %s -> %s\n", libName.c_str(), libPath.c_str());
            }
        }
    }
    
    return true;
}

bool UserlandVMConfig::VerifyLibrary(const std::string& path, LibraryConfig& config)
{
    struct stat statInfo;
    if (stat(path.c_str(), &statInfo) != 0) {
        return false;
    }
    
    config.exactPath = path;
    config.size = statInfo.st_size;
    config.isSystemLibrary = true;
    
    // Read library info (simplified)
    return true;
}

bool UserlandVMConfig::ValidateConfiguration()
{
    printf("[CONFIG] Validating configuration...\n");
    
    if (!fSysrootConfig.isValid) {
        printf("[CONFIG] ERROR: Invalid sysroot\n");
        return false;
    }
    
    if (fLibraries.find(LIB_ROOT) == fLibraries.end()) {
        printf("[CONFIG] ERROR: Missing critical library: %s\n", LIB_ROOT);
        return false;
    }
    
    if (fLibraries.find(LIB_BE) == fLibraries.end()) {
        printf("[CONFIG] ERROR: Missing critical library: %s\n", LIB_BE);
        return false;
    }
    
    printf("[CONFIG] Configuration validation passed\n");
    return true;
}

const UserlandVMConfig::SystemInfo& UserlandVMConfig::GetSystemInfo() const
{
    return fSystemInfo;
}

const UserlandVMConfig::SysrootConfig& UserlandVMConfig::GetSysrootConfig() const
{
    return fSysrootConfig;
}

const UserlandVMConfig::LibraryConfig* UserlandVMConfig::FindLibrary(const std::string& name) const
{
    auto it = fLibraries.find(name);
    return (it != fLibraries.end()) ? &it->second : nullptr;
}

std::vector<std::string> UserlandVMConfig::GetLibrarySearchPaths() const
{
    std::vector<std::string> allPaths = fSearchPaths;
    
    // Add sysroot library paths
    for (const auto& [name, config] : fLibraries) {
        for (const std::string& path : config.searchPaths) {
            if (std::find(allPaths.begin(), allPaths.end(), path) == allPaths.end()) {
                allPaths.push_back(path);
            }
        }
    }
    
    return allPaths;
}

std::string UserlandVMConfig::GetExecutablePath(const std::string& name) const
{
    // Check if executable exists in sysroot
    if (fSysrootConfig.isValid) {
        for (const std::string& binPath : kHaikuBinaryPaths) {
            std::string fullPath = JoinPath(fSysrootConfig.rootPath, binPath);
            std::string executablePath = JoinPath(fullPath, name);
            
            if (FileExists(executablePath)) {
                return executablePath;
            }
        }
    }
    
    return "";  // Not found
}

// Platform configuration constants
const char* const UserlandVMConfig::DEFAULT_HAIKU_SYSROOT = "sysroot/haiku32";
const char* const UserlandVMConfig::DEFAULT_LINUX_SYSROOT = "/usr/lib/x86_64-linux-gnu";
const char* const UserlandVMConfig::DEFAULT_CONFIG_FILE = ".userlandvm_config";

const char* const UserlandVMConfig::ENV_USERLANDVM_HOME = "USERLANDVM_HOME";
const char* const UserlandVMConfig::ENV_USERLANDVM_SYSROOT = "USERLANDVM_SYSROOT";
const char* const UserlandVMConfig::ENV_USERLANDVM_ARCH = "USERLANDVM_ARCH";

const char* const UserlandVMConfig::ARCH_X86 = "x86";
const char* const UserlandVMConfig::ARCH_X86_64 = "x86_64";
const char* const UserlandVMConfig::ARCH_ARM64 = "arm64";
const char* const UserlandVMConfig::ARCH_RISCV64 = "riscv64";

// Critical library names
const char* const UserlandVMConfig::LIB_ROOT = "libroot.so";
const char* const UserlandVMConfig::LIB_BE = "libbe.so";
const char* const UserlandVMConfig::LIB_NETWORK = "libnetwork.so";
const char* const UserlandVMConfig::LIB_MEDIA = "libmedia.so";
const char* const UserlandVMConfig::LIB_TRACKER = "libtracker.so";
const char* const UserlandVMConfig::LIB_GAME = "libgame.so";
const char* const UserlandVMConfig::LIB_OPENGL = "libGL.so";
const char* const UserlandVMConfig::LIB_STORAGE = "libstorage.so";
const char* const UserlandVMConfig::LIB_DEVICE = "libdevice.so";
const char* const UserlandVMConfig::LIB_INPUT = "libinput.so";
const char* const UserlandVMConfig::LIB_TEXTENCODING = "libtextencoding.so";

const char* const UserlandVMConfig::SECTION_SYSTEM = "system";
const char* const UserlandVMConfig::SECTION_SYSROOT = "sysroot";
const char* const UserlandVMConfig::SECTION_LIBRARIES = "libraries";
const char* const UserlandVMConfig::SECTION_PATHS = "paths";
const char* const UserlandVMConfig::CONFIG_FILENAME = "userlandvm_config";
const char* const UserlandVMConfig::SYSROOT_MARKER = ".sysroot_valid";

// Haiku library paths
const char* const HaikuConfig::kHaikuLibraryPaths[] = {
    "lib",
    "system/lib",
    "boot/system/lib",
    "develop/lib/x86"
    "packages/lib"
};

const char* const HaikuConfig::kHaikuBinaryPaths[] = {
    "bin",
    "system/bin",
    "boot/system/bin",
    "develop/tools/x86"
};

const char* const HaikuConfig::kHaikuCoreLibraries[] = {
    "libroot.so",
    "libbe.so",
    "libnetwork.so",
    "libmedia.so",
    "libtracker.so",
    "libgame.so",
    "libdevice.so",
    "libinput.so",
    "libtextencoding.so",
    "libtranslation.so",
    "libz.so",
    "libpthread.so",
    "libm.so",
    "librt.so",
    "ld.so"
};

std::map<std::string, std::string> HaikuConfig::kHaikuLibraryVersions;

// Linux library configurations
const std::vector<std::string> LinuxConfig::kStandardLibraryPaths = {
    "/usr/lib",
    "/usr/lib/x86_64-linux-gnu",
    "/lib",
    "/lib64"
};

const std::vector<std::string> LinuxConfig::kSystemLibraryPaths = {
    "/lib/x86_64-linux-gnu",
    "/usr/lib/x86_64-linux-gnu",
    "/lib",
    "/usr/lib"
};

// Core library definitions
const std::vector<std::string> UserlandVMConfig::kHaikuCoreLibraries = {
    "libroot.so",
    "libbe.so", 
    "libnetwork.so",
    "libmedia.so",
    "libtracker.so",
    "libgame.so",
    "libdevice.so",
    "libinput.so",
    "libtextencoding.so"
};

const std::vector<std::string> UserlandVMConfig::kLinuxCoreLibraries = {
    "ld-linux.so.2",
    "libc.so.6",
    "libm.so.6",
    "libpthread.so.0",
    "libz.so.1",
    "librt.so.1",
    "libdl.so.2"
};

// Target architecture definitions
const std::map<std::string, UserlandVMConfig::TargetArchitecture> UserlandVMConfig::kTargetArchitectures = {
    {"x86", {"x86", "i686-linux-gnu", {"lib", "lib64"}, {"bin", "bin64"}}},
    {"x86_64", {"x86_64", "x86_64-linux-gnu", {"lib64", "lib"}, {"bin", "lib64"}}},
    {"arm64", {"aarch64-linux-gnu", {"lib", "lib64"}, {"lib", "lib64"}, {"bin"}}},
    {"riscv64", {"riscv64-linux-gnu", {"lib", "l64"}, {"lib", "l64"}, {"bin"}}}
};

// Helper functions
std::string UserlandVMConfig::JoinPath(const std::string& base, const std::string& relative) const
{
    if (base.empty()) return relative;
    if (relative.empty()) return base;
    
    bool needsSeparator = (base.back() != '/');
    return base + (needsSeparator ? "/" : "") + relative;
}

std::string UserlandVMConfig::NormalizePath(const std::string& path) const
{
    std::string normalized = path;
    
    // Replace backslashes with forward slashes
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    // Remove duplicate slashes
    normalized.erase(std::unique(normalized.begin(), normalized.end(), 
                        [](char a, char b) { return a == '/' && b == '/'; }), 
                 normalized.end());
    
    return normalized;
}

std::string UserlandVMConfig::GetParentDirectory(const std::string& path) const
{
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return ".";
    }
    return path.substr(0, lastSlash);
}

std::string UserlandVMConfig::GetHomeDirectory()
{
    const char* home = getenv("HOME");
    return home ? std::string(home) : ".";
}

std::string UserlandVMConfig::GetExecutablePath()
{
    char buffer[4096];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::string(buffer);
    }
    return "";
}

std::string UserlandVMConfig::GetCurrentDirectory()
{
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
        return std::string(buffer);
    }
    return ".";
}

bool UserlandVMConfig::FileExists(const std::string& path)
{
    struct stat statInfo;
    return stat(path.c_str(), &statInfo) == 0 && S_ISREG(statInfo.st_mode);
}

bool UserlandVMConfig::DirectoryExists(const std::string& path)
{
    struct stat statInfo;
    return stat(path.c_str(), &statInfo) == 0 && S_ISDIR(statInfo.st_mode);
}

void UserlandVMConfig::PrintConfiguration() const
{
    printf("=== UserlandVM Configuration ===\n");
    printf("System: %s %s (%s)\n", 
           fSystemInfo.systemType.c_str(),
           fSystemInfo.architecture.c_str(),
           fSystemInfo.version.c_str());
    printf("Distribution: %s\n", fSystemInfo.distribution.c_str());
    printf("Compatible: %s\n", fSystemInfo.isCompatible ? "Yes" : "No");
    printf("Sysroot: %s\n", fSysrootConfig.rootPath.c_str());
    printf("Architecture: %s\n", fSysrootConfig.architecture.c_str());
    printf("Valid: %s\n", fSysrootConfig.isValid ? "Yes" : "No");
    
    printf("\nLibraries:\n");
    for (const auto& [name, config] : fLibraries) {
        printf("  %s: %s\n", name.c_str(), config.exactPath.c_str());
    }
    
    printf("\nSearch Paths:\n");
    for (const std::string& path : fSearchPaths) {
        printf("  %s\n", path.c_str());
    }
    
    printf("===============================\n");
}