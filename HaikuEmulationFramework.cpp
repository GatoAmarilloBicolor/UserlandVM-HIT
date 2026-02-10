#include "HaikuEmulationFramework.h"
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <dlfcn.h>

namespace HaikuEmulation {

/////////////////////////////////////////////////////////////////////////////
// KitFactory Implementation
/////////////////////////////////////////////////////////////////////////////

std::map<uint32_t, KitFactory::KitCreator>& KitFactory::GetRegistry() {
    static std::map<uint32_t, KitCreator> registry;
    return registry;
}

std::unique_ptr<IEmulationKit> KitFactory::CreateKit(uint32_t kit_id) {
    auto& registry = GetRegistry();
    auto it = registry.find(kit_id);
    if (it != registry.end()) {
        return it->second();
    }
    return nullptr;
}

std::vector<uint32_t> KitFactory::GetAvailableKits() {
    auto& registry = GetRegistry();
    std::vector<uint32_t> kits;
    kits.reserve(registry.size());
    
    for (const auto& pair : registry) {
        kits.push_back(pair.first);
    }
    
    return kits;
}

bool KitFactory::IsKitAvailable(uint32_t kit_id) {
    auto& registry = GetRegistry();
    return registry.find(kit_id) != registry.end();
}

std::string KitFactory::GetKitName(uint32_t kit_id) {
    // Cache kit info to avoid creating temporary objects
    static std::map<uint32_t, std::string> name_cache;
    static std::mutex cache_mutex;
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = name_cache.find(kit_id);
        if (it != name_cache.end()) {
            return it->second;
        }
    }
    
    // Only create kit if not cached
    auto kit = CreateKit(kit_id);
    if (kit) {
        std::string name = kit->GetKitName();
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            name_cache[kit_id] = name;
        }
        return name;
    }
    return "Unknown";
}

std::string KitFactory::GetKitVersion(uint32_t kit_id) {
    // Cache kit info to avoid creating temporary objects
    static std::map<uint32_t, std::string> version_cache;
    static std::mutex cache_mutex;
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = version_cache.find(kit_id);
        if (it != version_cache.end()) {
            return it->second;
        }
    }
    
    // Only create kit if not cached
    auto kit = CreateKit(kit_id);
    if (kit) {
        std::string version = kit->GetKitVersion();
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            version_cache[kit_id] = version;
        }
        return version;
    }
    return "0.0.0";
}

std::vector<std::string> KitFactory::GetKitCapabilities(uint32_t kit_id) {
    // Cache kit info to avoid creating temporary objects
    static std::map<uint32_t, std::vector<std::string>> capabilities_cache;
    static std::mutex cache_mutex;
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = capabilities_cache.find(kit_id);
        if (it != capabilities_cache.end()) {
            return it->second;
        }
    }
    
    // Only create kit if not cached
    auto kit = CreateKit(kit_id);
    if (kit) {
        std::vector<std::string> capabilities = kit->GetCapabilities();
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            capabilities_cache[kit_id] = capabilities;
        }
        return capabilities;
    }
    return {};
}

/////////////////////////////////////////////////////////////////////////////
// SyscallRouter Implementation
/////////////////////////////////////////////////////////////////////////////

void SyscallRouter::RegisterKit(IEmulationKit* kit) {
    if (!kit) return;
    
    std::lock_guard<std::mutex> lock(router_mutex);
    registered_kits[kit->GetKitID()] = kit;
    
    // Register kit's syscalls
    auto supported_syscalls = kit->GetSupportedSyscalls();
    const char* kit_name = kit->GetKitName();
    uint32_t kit_id = kit->GetKitID();
    
    // Note: Maps don't have reserve(), but we avoid repeated string allocations
    
    for (uint32_t syscall_num : supported_syscalls) {
        SyscallInfo info;
        info.kit_id = kit_id;
        info.syscall_num = syscall_num;
        
        // Use more efficient string construction
        char name_buffer[64];
        snprintf(name_buffer, sizeof(name_buffer), "Syscall_%u", syscall_num);
        info.name = name_buffer;
        
        char desc_buffer[128];
        snprintf(desc_buffer, sizeof(desc_buffer), "Syscall %u for %s", syscall_num, kit_name);
        info.description = desc_buffer;
        
        info.is_async = false;
        info.timeout_ms = 5000;
        
        syscall_registry[kit_id][syscall_num] = std::move(info);
    }
    
    printf("[SyscallRouter] Registered kit: %s (ID: %d) with %zu syscalls\n",
           kit->GetKitName(), kit->GetKitID(), supported_syscalls.size());
}

void SyscallRouter::UnregisterKit(uint32_t kit_id) {
    std::lock_guard<std::mutex> lock(router_mutex);
    
    registered_kits.erase(kit_id);
    syscall_registry.erase(kit_id);
    syscall_stats.erase(kit_id);
    
    printf("[SyscallRouter] Unregistered kit: ID %d\n", kit_id);
}

bool SyscallRouter::RouteSyscall(uint32_t combined_syscall, uint32_t* args, uint32_t* result) {
    uint32_t kit_id = ExtractKitID(combined_syscall);
    uint32_t syscall_num = ExtractSyscallNum(combined_syscall);
    return RouteSyscall(kit_id, syscall_num, args, result);
}

bool SyscallRouter::RouteSyscall(uint32_t kit_id, uint32_t syscall_num, uint32_t* args, uint32_t* result) {
    auto start_time = std::chrono::high_resolution_clock::now();
    bool success = false;
    
    {
        std::lock_guard<std::mutex> lock(router_mutex);
        auto it = registered_kits.find(kit_id);
        if (it == registered_kits.end()) {
            printf("[SyscallRouter] Unknown kit ID: %d\n", kit_id);
            return false;
        }
        
        IEmulationKit* kit = it->second;
        if (!kit->IsInitialized()) {
            printf("[SyscallRouter] Kit not initialized: %s\n", kit->GetKitName());
            return false;
        }
        
        success = kit->HandleSyscall(syscall_num, args, result);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    UpdateStats(kit_id, syscall_num, success, duration.count());
    
    return success;
}

SyscallRouter::SyscallInfo SyscallRouter::GetSyscallInfo(uint32_t kit_id, uint32_t syscall_num) const {
    std::lock_guard<std::mutex> lock(router_mutex);
    
    auto kit_it = syscall_registry.find(kit_id);
    if (kit_it != syscall_registry.end()) {
        auto syscall_it = kit_it->second.find(syscall_num);
        if (syscall_it != kit_it->second.end()) {
            return syscall_it->second;
        }
    }
    
    return {};
}

std::vector<SyscallRouter::SyscallInfo> SyscallRouter::GetAllSyscalls() const {
    std::lock_guard<std::mutex> lock(router_mutex);
    
    std::vector<SyscallInfo> all_syscalls;
    for (const auto& kit_pair : syscall_registry) {
        for (const auto& syscall_pair : kit_pair.second) {
            all_syscalls.push_back(syscall_pair.second);
        }
    }
    
    return all_syscalls;
}

std::vector<SyscallRouter::SyscallInfo> SyscallRouter::GetKitSyscalls(uint32_t kit_id) const {
    std::lock_guard<std::mutex> lock(router_mutex);
    
    std::vector<SyscallInfo> kit_syscalls;
    auto kit_it = syscall_registry.find(kit_id);
    if (kit_it != syscall_registry.end()) {
        for (const auto& syscall_pair : kit_it->second) {
            kit_syscalls.push_back(syscall_pair.second);
        }
    }
    
    return kit_syscalls;
}

SyscallRouter::SyscallStats SyscallRouter::GetSyscallStats(uint32_t kit_id, uint32_t syscall_num) const {
    std::lock_guard<std::mutex> lock(router_mutex);
    
    auto kit_it = syscall_stats.find(kit_id);
    if (kit_it != syscall_stats.end()) {
        auto syscall_it = kit_it->second.find(syscall_num);
        if (syscall_it != kit_it->second.end()) {
            return syscall_it->second;
        }
    }
    
    return {};
}

std::map<uint32_t, SyscallRouter::SyscallStats> SyscallRouter::GetAllStats() const {
    std::lock_guard<std::mutex> lock(router_mutex);
    
    // Flatten the nested map to match the expected return type
    std::map<uint32_t, SyscallStats> flattened_stats;
    for (const auto& kit_pair : syscall_stats) {
        uint32_t kit_id = kit_pair.first;
        for (const auto& syscall_pair : kit_pair.second) {
            uint32_t combined_syscall = (kit_id << 24) | (syscall_pair.first & 0x00FFFFFF);
            flattened_stats[combined_syscall] = syscall_pair.second;
        }
    }
    
    return flattened_stats;
}

void SyscallRouter::ResetStats() {
    std::lock_guard<std::mutex> lock(router_mutex);
    syscall_stats.clear();
}

uint32_t SyscallRouter::ExtractKitID(uint32_t combined_syscall) const {
    return (combined_syscall >> 24) & 0xFF;
}

uint32_t SyscallRouter::ExtractSyscallNum(uint32_t combined_syscall) const {
    return combined_syscall & 0x00FFFFFF;
}

void SyscallRouter::UpdateStats(uint32_t kit_id, uint32_t syscall_num, bool success, uint64_t time_us) {
    std::lock_guard<std::mutex> lock(router_mutex);
    
    SyscallStats& stats = syscall_stats[kit_id][syscall_num];
    stats.call_count++;
    stats.total_time_us += time_us;
    
    if (success) {
        stats.success_count++;
    } else {
        stats.error_count++;
    }
    
    stats.average_time_us = (double)stats.total_time_us / stats.call_count;
}

/////////////////////////////////////////////////////////////////////////////
// ConfigurationManager Implementation
/////////////////////////////////////////////////////////////////////////////

bool ConfigurationManager::LoadProfile(const std::string& profile_name) {
    std::lock_guard<std::mutex> lock(config_mutex);
    
    auto it = profiles.find(profile_name);
    if (it == profiles.end()) {
        printf("[ConfigManager] Profile not found: %s\n", profile_name.c_str());
        return false;
    }
    
    current_profile = profile_name;
    const Profile& profile = it->second;
    
    // Apply system settings
    for (const auto& setting : profile.settings) {
        system_settings[setting.first] = setting.second;
    }
    
    printf("[ConfigManager] Loaded profile: %s\n", profile_name.c_str());
    return true;
}

bool ConfigurationManager::SaveProfile(const std::string& profile_name) {
    std::lock_guard<std::mutex> lock(config_mutex);
    
    Profile& profile = profiles[profile_name];
    profile.name = profile_name;
    profile.settings = system_settings;
    
    printf("[ConfigManager] Saved profile: %s\n", profile_name.c_str());
    return true;
}

bool ConfigurationManager::DeleteProfile(const std::string& profile_name) {
    std::lock_guard<std::mutex> lock(config_mutex);
    
    auto it = profiles.find(profile_name);
    if (it != profiles.end()) {
        profiles.erase(it);
        if (current_profile == profile_name) {
            current_profile.clear();
        }
        printf("[ConfigManager] Deleted profile: %s\n", profile_name.c_str());
        return true;
    }
    
    return false;
}

std::vector<std::string> ConfigurationManager::GetAvailableProfiles() const {
    std::lock_guard<std::mutex> lock(config_mutex);
    
    std::vector<std::string> profile_names;
    profile_names.reserve(profiles.size());
    
    for (const auto& pair : profiles) {
        profile_names.push_back(pair.first);
    }
    
    return profile_names;
}

std::string ConfigurationManager::GetSetting(const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(config_mutex);
    
    auto it = system_settings.find(key);
    if (it != system_settings.end()) {
        return it->second;
    }
    
    return default_value;
}

void ConfigurationManager::SetSetting(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(config_mutex);
    system_settings[key] = value;
}

bool ConfigurationManager::HasSetting(const std::string& key) const {
    std::lock_guard<std::mutex> lock(config_mutex);
    return system_settings.find(key) != system_settings.end();
}

std::map<std::string, std::string> ConfigurationManager::GetKitConfig(uint32_t kit_id) const {
    std::lock_guard<std::mutex> lock(config_mutex);
    
    auto it = kit_settings.find(kit_id);
    if (it != kit_settings.end()) {
        return it->second;
    }
    
    return {};
}

void ConfigurationManager::SetKitConfig(uint32_t kit_id, const std::map<std::string, std::string>& config) {
    std::lock_guard<std::mutex> lock(config_mutex);
    kit_settings[kit_id] = config;
}

void ConfigurationManager::SetSystemConfig(const std::map<std::string, std::string>& config) {
    std::lock_guard<std::mutex> lock(config_mutex);
    system_settings = config;
}

std::map<std::string, std::string> ConfigurationManager::GetSystemConfig() const {
    std::lock_guard<std::mutex> lock(config_mutex);
    return system_settings;
}

bool ConfigurationManager::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        printf("[ConfigManager] Cannot open config file: %s\n", filename.c_str());
        return false;
    }
    
    std::lock_guard<std::mutex> lock(config_mutex);
    
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            system_settings[key] = value;
        }
    }
    
    printf("[ConfigManager] Loaded config from: %s\n", filename.c_str());
    return true;
}

bool ConfigurationManager::SaveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        printf("[ConfigManager] Cannot create config file: %s\n", filename.c_str());
        return false;
    }
    
    std::lock_guard<std::mutex> lock(config_mutex);
    
    for (const auto& setting : system_settings) {
        file << setting.first << "=" << setting.second << std::endl;
    }
    
    printf("[ConfigManager] Saved config to: %s\n", filename.c_str());
    return true;
}

/////////////////////////////////////////////////////////////////////////////
// PluginSystem Implementation
/////////////////////////////////////////////////////////////////////////////

bool PluginSystem::LoadPlugin(const std::string& plugin_path) {
    void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
    if (!handle) {
        printf("[PluginSystem] Failed to load plugin: %s - %s\n", plugin_path.c_str(), dlerror());
        return false;
    }
    
    // Look for plugin registration function
    typedef void (*RegisterPluginFunc)();
    RegisterPluginFunc register_plugin = (RegisterPluginFunc)dlsym(handle, "RegisterPlugin");
    
    if (!register_plugin) {
        printf("[PluginSystem] Plugin missing RegisterPlugin function: %s\n", plugin_path.c_str());
        dlclose(handle);
        return false;
    }
    
    PluginInfo info;
    info.path = plugin_path;
    info.handle = handle;
    info.loaded = true;
    
    // Call plugin registration
    register_plugin();
    
    // Extract plugin name from path
    size_t last_slash = plugin_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        info.name = plugin_path.substr(last_slash + 1);
    } else {
        info.name = plugin_path;
    }
    
    std::lock_guard<std::mutex> lock(plugin_mutex);
    loaded_plugins[info.name] = info;
    
    printf("[PluginSystem] Loaded plugin: %s from %s\n", info.name.c_str(), plugin_path.c_str());
    return true;
}

bool PluginSystem::UnloadPlugin(const std::string& plugin_name) {
    std::lock_guard<std::mutex> lock(plugin_mutex);
    
    auto it = loaded_plugins.find(plugin_name);
    if (it == loaded_plugins.end()) {
        return false;
    }
    
    PluginInfo& info = it->second;
    if (info.handle) {
        dlclose(info.handle);
    }
    
    loaded_plugins.erase(it);
    printf("[PluginSystem] Unloaded plugin: %s\n", plugin_name.c_str());
    return true;
}

std::vector<PluginSystem::PluginInfo> PluginSystem::GetLoadedPlugins() const {
    std::lock_guard<std::mutex> lock(plugin_mutex);
    
    std::vector<PluginInfo> plugins;
    plugins.reserve(loaded_plugins.size());
    
    for (const auto& pair : loaded_plugins) {
        plugins.push_back(pair.second);
    }
    
    return plugins;
}

/////////////////////////////////////////////////////////////////////////////
// EmulationEngine Implementation
/////////////////////////////////////////////////////////////////////////////

bool EmulationEngine::Initialize() {
    std::lock_guard<std::mutex> lock(engine_mutex);
    
    if (initialized) {
        return true;
    }
    
    printf("[EmulationEngine] Initializing Universal Haiku Emulation Framework...\n");
    
    // Load configuration
    std::string config_file = config_manager.GetSetting("config_file", "haiku_emulation.conf");
    if (!config_manager.LoadFromFile(config_file)) {
        printf("[EmulationEngine] Using default configuration\n");
    }
    
    // Auto-discover and load plugins
    std::string plugin_path = config_manager.GetSetting("plugin_path", "./plugins");
    if (plugin_system.LoadAllPlugins(plugin_path)) {
        printf("[EmulationEngine] Loaded plugins from: %s\n", plugin_path.c_str());
    }
    
    // Initialize performance monitoring if enabled
    std::string perf_monitoring = config_manager.GetSetting("performance_monitoring", "false");
    performance_monitoring = (perf_monitoring == "true");
    
    if (performance_monitoring) {
        printf("[EmulationEngine] Performance monitoring enabled\n");
    }
    
    initialized = true;
    printf("[EmulationEngine] ✅ Universal Haiku Emulation Framework initialized\n");
    
    return true;
}

void EmulationEngine::Shutdown() {
    std::lock_guard<std::mutex> lock(engine_mutex);
    
    if (!initialized) {
        return;
    }
    
    printf("[EmulationEngine] Shutting down Universal Haiku Emulation Framework...\n");
    
    // Unload all kits
    for (auto& pair : loaded_kits) {
        if (pair.second) {
            pair.second->Shutdown();
        }
    }
    loaded_kits.clear();
    
    // Unload all plugins
    auto plugins = plugin_system.GetLoadedPlugins();
    for (const auto& plugin : plugins) {
        plugin_system.UnloadPlugin(plugin.name);
    }
    
    initialized = false;
    printf("[EmulationEngine] ✅ Framework shut down\n");
}

bool EmulationEngine::IsInitialized() const {
    return initialized.load();
}

bool EmulationEngine::LoadKit(uint32_t kit_id) {
    std::lock_guard<std::mutex> lock(engine_mutex);
    
    if (loaded_kits.find(kit_id) != loaded_kits.end()) {
        return true; // Already loaded
    }
    
    auto kit = KitFactory::CreateKit(kit_id);
    if (!kit) {
        printf("[EmulationEngine] Failed to create kit: ID %d\n", kit_id);
        return false;
    }
    
    // Configure kit
    auto kit_config = config_manager.GetKitConfig(kit_id);
    if (!kit_config.empty()) {
        kit->Configure(kit_config);
    }
    
    // Initialize kit
    if (!kit->Initialize()) {
        printf("[EmulationEngine] Failed to initialize kit: %s\n", kit->GetKitName());
        return false;
    }
    
    // Register with syscall router
    syscall_router.RegisterKit(kit.get());
    
    loaded_kits[kit_id] = std::move(kit);
    printf("[EmulationEngine] ✅ Loaded kit: %s (ID: %d)\n", 
           loaded_kits[kit_id]->GetKitName(), kit_id);
    
    return true;
}

bool EmulationEngine::UnloadKit(uint32_t kit_id) {
    std::lock_guard<std::mutex> lock(engine_mutex);
    
    auto it = loaded_kits.find(kit_id);
    if (it == loaded_kits.end()) {
        return false;
    }
    
    IEmulationKit* kit = it->second.get();
    printf("[EmulationEngine] Unloading kit: %s\n", kit->GetKitName());
    
    // Unregister from syscall router
    syscall_router.UnregisterKit(kit_id);
    
    // Shutdown kit
    kit->Shutdown();
    
    // Remove from loaded kits
    loaded_kits.erase(it);
    
    return true;
}

std::vector<uint32_t> EmulationEngine::GetLoadedKits() const {
    std::lock_guard<std::mutex> lock(engine_mutex);
    
    std::vector<uint32_t> kit_ids;
    kit_ids.reserve(loaded_kits.size());
    
    for (const auto& pair : loaded_kits) {
        kit_ids.push_back(pair.first);
    }
    
    return kit_ids;
}

IEmulationKit* EmulationEngine::GetKit(uint32_t kit_id) {
    std::lock_guard<std::mutex> lock(engine_mutex);
    
    auto it = loaded_kits.find(kit_id);
    if (it != loaded_kits.end()) {
        return it->second.get();
    }
    
    return nullptr;
}

bool EmulationEngine::HandleSyscall(uint32_t combined_syscall, uint32_t* args, uint32_t* result) {
    if (!initialized) {
        printf("[EmulationEngine] Engine not initialized\n");
        return false;
    }
    
    return syscall_router.RouteSyscall(combined_syscall, args, result);
}

void EmulationEngine::EnablePerformanceMonitoring(bool enable) {
    performance_monitoring = enable;
}

bool EmulationEngine::IsPerformanceMonitoringEnabled() const {
    return performance_monitoring.load();
}

bool EmulationEngine::SaveEngineState(const std::string& filename) {
    // Implementation for saving complete engine state
    printf("[EmulationEngine] Saving engine state to: %s\n", filename.c_str());
    return true; // Placeholder
}

bool EmulationEngine::LoadEngineState(const std::string& filename) {
    // Implementation for loading complete engine state
    printf("[EmulationEngine] Loading engine state from: %s\n", filename.c_str());
    return true; // Placeholder
}

} // namespace HaikuEmulation