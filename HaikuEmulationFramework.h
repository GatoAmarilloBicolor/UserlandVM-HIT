#pragma once

#include "PlatformTypes.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <mutex>
#include <atomic>

/////////////////////////////////////////////////////////////////////////////
// Universal Haiku OS Emulation Framework
// Modular, reusable, and intelligent system architecture
/////////////////////////////////////////////////////////////////////////////

namespace HaikuEmulation {

// Forward declarations
class IEmulationKit;
class KitManager;
class SyscallRouter;
class ConfigurationManager;

/////////////////////////////////////////////////////////////////////////////
// Core Interfaces - Universal abstractions for all kits
/////////////////////////////////////////////////////////////////////////////

class IEmulationKit {
public:
    virtual ~IEmulationKit() = default;
    
    // Kit identification
    virtual const char* GetKitName() const = 0;
    virtual const char* GetKitVersion() const = 0;
    virtual uint32_t GetKitID() const = 0;
    
    // Lifecycle management
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsInitialized() const = 0;
    
    // Capability reporting
    virtual std::vector<std::string> GetCapabilities() const = 0;
    virtual bool HasCapability(const std::string& capability) const = 0;
    
    // Syscall handling
    virtual bool HandleSyscall(uint32_t syscall_num, uint32_t* args, uint32_t* result) = 0;
    virtual std::vector<uint32_t> GetSupportedSyscalls() const = 0;
    
    // Configuration
    virtual bool Configure(const std::map<std::string, std::string>& config) = 0;
    virtual std::map<std::string, std::string> GetConfiguration() const = 0;
    
    // State management
    virtual bool SaveState(void** data, size_t* size) = 0;
    virtual bool LoadState(const void* data, size_t size) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// Kit Factory - Intelligent kit creation and management
/////////////////////////////////////////////////////////////////////////////

class KitFactory {
public:
    // Kit registration
    template<typename T>
    static void RegisterKit() {
        GetRegistry()[T::GetStaticKitID()] = []() -> std::unique_ptr<IEmulationKit> {
            return std::make_unique<T>();
        };
    }
    
    // Kit creation
    static std::unique_ptr<IEmulationKit> CreateKit(uint32_t kit_id);
    static std::vector<uint32_t> GetAvailableKits();
    static bool IsKitAvailable(uint32_t kit_id);
    
    // Kit information
    static std::string GetKitName(uint32_t kit_id);
    static std::string GetKitVersion(uint32_t kit_id);
    static std::vector<std::string> GetKitCapabilities(uint32_t kit_id);

private:
    using KitCreator = std::function<std::unique_ptr<IEmulationKit>()>;
    static std::map<uint32_t, KitCreator>& GetRegistry();
};

/////////////////////////////////////////////////////////////////////////////
// Universal Kit Base - Smart base class for all kits
/////////////////////////////////////////////////////////////////////////////

template<uint32_t KitID, const char* KitName, const char* KitVersion>
class UniversalKit : public IEmulationKit {
public:
    // Static identification
    static constexpr uint32_t GetStaticKitID() { return KitID; }
    static constexpr const char* GetStaticKitName() { return KitName; }
    static constexpr const char* GetStaticKitVersion() { return KitVersion; }
    
    // IEmulationKit implementation
    const char* GetKitName() const override { return KitName; }
    const char* GetKitVersion() const override { return KitVersion; }
    uint32_t GetKitID() const override { return KitID; }
    
    bool IsInitialized() const override { return initialized.load(); }
    
    std::vector<std::string> GetCapabilities() const override {
        std::lock_guard<std::mutex> lock(capabilities_mutex);
        return capabilities;
    }
    
    bool HasCapability(const std::string& capability) const override {
        std::lock_guard<std::mutex> lock(capabilities_mutex);
        return std::find(capabilities.begin(), capabilities.end(), capability) != capabilities.end();
    }
    
    bool Configure(const std::map<std::string, std::string>& config) override {
        std::lock_guard<std::mutex> lock(config_mutex);
        configuration = config;
        return OnConfigure(config);
    }
    
    std::map<std::string, std::string> GetConfiguration() const override {
        std::lock_guard<std::mutex> lock(config_mutex);
        return configuration;
    }

protected:
    // Protected constructor for inheritance
    UniversalKit() : initialized(false) {}
    
    // Capability management
    void AddCapability(const std::string& capability) {
        std::lock_guard<std::mutex> lock(capabilities_mutex);
        capabilities.push_back(capability);
    }
    
    void RemoveCapability(const std::string& capability) {
        std::lock_guard<std::mutex> lock(capabilities_mutex);
        capabilities.erase(
            std::remove(capabilities.begin(), capabilities.end(), capability),
            capabilities.end()
        );
    }
    
    // State management
    void SetInitialized(bool state) { initialized = state; }
    
    // Virtual hooks for subclasses
    virtual bool OnConfigure(const std::map<std::string, std::string>& config) { return true; }
    virtual void OnInitialized() {}
    virtual void OnShutdown() {}

private:
    std::atomic<bool> initialized;
    mutable std::mutex capabilities_mutex;
    std::vector<std::string> capabilities;
    mutable std::mutex config_mutex;
    std::map<std::string, std::string> configuration;
};

/////////////////////////////////////////////////////////////////////////////
// Syscall Router - Intelligent syscall routing and management
/////////////////////////////////////////////////////////////////////////////

class SyscallRouter {
public:
    struct SyscallInfo {
        uint32_t kit_id;
        uint32_t syscall_num;
        std::string name;
        std::string description;
        std::vector<std::string> parameters;
        bool is_async;
        uint32_t timeout_ms;
    };
    
    // Kit registration
    void RegisterKit(IEmulationKit* kit);
    void UnregisterKit(uint32_t kit_id);
    
    // Syscall routing
    bool RouteSyscall(uint32_t combined_syscall, uint32_t* args, uint32_t* result);
    bool RouteSyscall(uint32_t kit_id, uint32_t syscall_num, uint32_t* args, uint32_t* result);
    
    // Syscall information
    SyscallInfo GetSyscallInfo(uint32_t kit_id, uint32_t syscall_num) const;
    std::vector<SyscallInfo> GetAllSyscalls() const;
    std::vector<SyscallInfo> GetKitSyscalls(uint32_t kit_id) const;
    
    // Performance monitoring
    struct SyscallStats {
        uint64_t call_count;
        uint64_t total_time_us;
        uint64_t success_count;
        uint64_t error_count;
        double average_time_us;
    };
    
    SyscallStats GetSyscallStats(uint32_t kit_id, uint32_t syscall_num) const;
    std::map<uint32_t, SyscallStats> GetAllStats() const;
    void ResetStats();

private:
    std::map<uint32_t, IEmulationKit*> registered_kits;
    std::map<uint32_t, std::map<uint32_t, SyscallInfo>> syscall_registry;
    std::map<uint32_t, std::map<uint32_t, SyscallStats>> syscall_stats;
    mutable std::mutex router_mutex;
    
    uint32_t ExtractKitID(uint32_t combined_syscall) const;
    uint32_t ExtractSyscallNum(uint32_t combined_syscall) const;
    void UpdateStats(uint32_t kit_id, uint32_t syscall_num, bool success, uint64_t time_us);
};

/////////////////////////////////////////////////////////////////////////////
// Configuration Manager - Intelligent configuration system
/////////////////////////////////////////////////////////////////////////////

class ConfigurationManager {
public:
    // Configuration profiles
    struct Profile {
        std::string name;
        std::string description;
        std::map<std::string, std::string> settings;
        std::vector<uint32_t> enabled_kits;
    };
    
    // Profile management
    bool LoadProfile(const std::string& profile_name);
    bool SaveProfile(const std::string& profile_name);
    bool DeleteProfile(const std::string& profile_name);
    std::vector<std::string> GetAvailableProfiles() const;
    
    // Configuration access
    std::string GetSetting(const std::string& key, const std::string& default_value = "") const;
    void SetSetting(const std::string& key, const std::string& value);
    bool HasSetting(const std::string& key) const;
    
    // Kit-specific configuration
    std::map<std::string, std::string> GetKitConfig(uint32_t kit_id) const;
    void SetKitConfig(uint32_t kit_id, const std::map<std::string, std::string>& config);
    
    // System configuration
    void SetSystemConfig(const std::map<std::string, std::string>& config);
    std::map<std::string, std::string> GetSystemConfig() const;
    
    // Persistence
    bool LoadFromFile(const std::string& filename);
    bool SaveToFile(const std::string& filename);

private:
    std::map<std::string, Profile> profiles;
    std::string current_profile;
    std::map<std::string, std::string> system_settings;
    std::map<uint32_t, std::map<std::string, std::string>> kit_settings;
    mutable std::mutex config_mutex;
};

/////////////////////////////////////////////////////////////////////////////
// Plugin System - Dynamic loading and unloading of kits
/////////////////////////////////////////////////////////////////////////////

class PluginSystem {
public:
    struct PluginInfo {
        std::string name;
        std::string path;
        std::string version;
        std::vector<uint32_t> provided_kits;
        void* handle;
        bool loaded;
    };
    
    // Plugin management
    bool LoadPlugin(const std::string& plugin_path);
    bool UnloadPlugin(const std::string& plugin_name);
    std::vector<PluginInfo> GetLoadedPlugins() const;
    
    // Plugin information
    PluginInfo GetPluginInfo(const std::string& plugin_name) const;
    bool IsPluginLoaded(const std::string& plugin_name) const;
    
    // Auto-discovery
    std::vector<std::string> DiscoverPlugins(const std::string& search_path) const;
    bool LoadAllPlugins(const std::string& search_path);

private:
    std::map<std::string, PluginInfo> loaded_plugins;
    mutable std::mutex plugin_mutex;
};

/////////////////////////////////////////////////////////////////////////////
// Main Emulation Engine - Universal coordination
/////////////////////////////////////////////////////////////////////////////

class EmulationEngine {
public:
    static EmulationEngine& Instance() {
        static EmulationEngine instance;
        return instance;
    }
    
    // Engine lifecycle
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const;
    
    // Kit management
    bool LoadKit(uint32_t kit_id);
    bool UnloadKit(uint32_t kit_id);
    std::vector<uint32_t> GetLoadedKits() const;
    IEmulationKit* GetKit(uint32_t kit_id);
    
    // Syscall handling
    bool HandleSyscall(uint32_t combined_syscall, uint32_t* args, uint32_t* result);
    
    // Component access
    SyscallRouter& GetSyscallRouter() { return syscall_router; }
    ConfigurationManager& GetConfigManager() { return config_manager; }
    PluginSystem& GetPluginSystem() { return plugin_system; }
    
    // Performance monitoring
    void EnablePerformanceMonitoring(bool enable);
    bool IsPerformanceMonitoringEnabled() const;
    
    // State management
    bool SaveEngineState(const std::string& filename);
    bool LoadEngineState(const std::string& filename);

private:
    EmulationEngine() : initialized(false), performance_monitoring(false) {}
    ~EmulationEngine() { Shutdown(); }
    
    std::atomic<bool> initialized;
    std::atomic<bool> performance_monitoring;
    
    std::map<uint32_t, std::unique_ptr<IEmulationKit>> loaded_kits;
    mutable std::mutex engine_mutex;
    
    SyscallRouter syscall_router;
    ConfigurationManager config_manager;
    PluginSystem plugin_system;
    
    // Prevent copying
    EmulationEngine(const EmulationEngine&) = delete;
    EmulationEngine& operator=(const EmulationEngine&) = delete;
};

/////////////////////////////////////////////////////////////////////////////
// Utility Macros - Easy registration and usage
/////////////////////////////////////////////////////////////////////////////

#define HAIKU_REGISTER_KIT(KitClass) \
    namespace { \
        struct KitClass##_Registrar { \
            KitClass##_Registrar() { \
                HaikuEmulation::KitFactory::RegisterKit<KitClass>(); \
            } \
        }; \
        static KitClass##_Registrar g_##KitClass##_registrar; \
    }

#define HAIKU_EMULATION_ENGINE HaikuEmulation::EmulationEngine::Instance()

#define HAIKU_HANDLE_SYSCALL(combined_syscall, args, result) \
    HAIKU_EMULATION_ENGINE.HandleSyscall(combined_syscall, args, result)

#define HAIKU_GET_KIT(kit_id) HAIKU_EMULATION_ENGINE.GetKit(kit_id)

#define HAIKU_CONFIG(key, default_value) \
    HAIKU_EMULATION_ENGINE.GetConfigManager().GetSetting(key, default_value)

} // namespace HaikuEmulation