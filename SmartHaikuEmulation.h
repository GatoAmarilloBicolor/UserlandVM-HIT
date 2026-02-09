#pragma once

#include "HaikuEmulationFramework.h"
#include "kits/ModularInterfaceKit.h"
#include "kits/ModularMediaKit.h"
#include "kits/ModularNetworkKit.h"
#include "DynamicLinker.h"

/////////////////////////////////////////////////////////////////////////////
// Example: Smart Haiku OS Emulation using Universal Framework
/////////////////////////////////////////////////////////////////////////////

namespace HaikuEmulation {

class SmartHaikuEmulation {
public:
    static SmartHaikuEmulation& Instance() {
        static SmartHaikuEmulation instance;
        return instance;
    }
    
    // Initialization
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const;
    
    // Intelligent kit loading
    bool LoadRequiredKits(const std::vector<std::string>& requirements);
    bool LoadAllKits();
    bool UnloadKit(const std::string& kit_name);
    std::vector<std::string> GetLoadedKits() const;
    
    // Smart syscall handling
    bool HandleHaikuSyscall(uint32_t combined_syscall, uint32_t* args, uint32_t* result);
    
    // Performance optimization
    void EnablePerformanceOptimization(bool enable);
    bool IsPerformanceOptimizationEnabled() const;
    
    // Auto-configuration
    bool AutoConfigure();
    bool LoadConfigurationProfile(const std::string& profile_name);
    bool SaveConfigurationProfile(const std::string& profile_name);
    
    // Plugin management
    bool LoadPlugin(const std::string& plugin_path);
    bool UnloadPlugin(const std::string& plugin_name);
    std::vector<std::string> GetLoadedPlugins() const;
    
    // State management
    bool SaveState(const std::string& filename);
    bool LoadState(const std::string& filename);
    
    // Monitoring and diagnostics
    void EnableMonitoring(bool enable);
    bool IsMonitoringEnabled() const;
    std::string GetSystemStatus() const;
    std::map<std::string, std::string> GetPerformanceMetrics() const;

private:
    SmartHaikuEmulation() : initialized(false), performance_optimization(false), monitoring(false) {}
    ~SmartHaikuEmulation() { Shutdown(); }
    
    bool initialized;
    bool performance_optimization;
    bool monitoring;
    
    // Kit availability detection
    std::map<std::string, bool> kit_availability;
    void DetectKitAvailability();
    
    // Smart configuration
    std::map<std::string, std::string> smart_config;
    void ApplySmartConfiguration();
    
    // Performance optimization
    void OptimizeSyscallRouting();
    void OptimizeMemoryUsage();
    void OptimizeKitLoading();
    
    // Prevent copying
    SmartHaikuEmulation(const SmartHaikuEmulation&) = delete;
    SmartHaikuEmulation& operator=(const SmartHaikuEmulation&) = delete;
};

/////////////////////////////////////////////////////////////////////////////
// Usage Examples and Integration
/////////////////////////////////////////////////////////////////////////////

namespace Examples {

// Example 1: Basic Usage
class BasicHaikuEmulation {
public:
    bool Initialize() {
        // Initialize the universal framework
        if (!HAIKU_EMULATION_ENGINE.Initialize()) {
            return false;
        }
        
        // Load required kits
        if (!HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularInterfaceKit::GetStaticKitID())) {
            return false;
        }
        
        if (!HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularMediaKit::GetStaticKitID())) {
            return false;
        }
        
        if (!HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularNetworkKit::GetStaticKitID())) {
            return false;
        }
        
        return true;
    }
    
    bool HandleSyscall(uint32_t combined_syscall, uint32_t* args, uint32_t* result) {
        return HAIKU_EMULATION_ENGINE.HandleSyscall(combined_syscall, args, result);
    }
};

// Example 2: Configuration-Driven Usage
class ConfigurableHaikuEmulation {
public:
    bool Initialize(const std::string& config_file) {
        // Load configuration
        if (!HAIKU_CONFIG_MANAGER.LoadFromFile(config_file)) {
            return false;
        }
        
        // Initialize engine
        if (!HAIKU_EMULATION_ENGINE.Initialize()) {
            return false;
        }
        
        // Auto-configure based on requirements
        std::string requirements = HAIKU_CONFIG("required_kits", "interface,media");
        return LoadKitsBasedOnRequirements(requirements);
    }
    
private:
    bool LoadKitsBasedOnRequirements(const std::string& requirements) {
        std::vector<std::string> required_kits;
        // Parse requirements string
        
        for (const auto& kit : required_kits) {
            if (kit == "interface") {
                HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularInterfaceKit::GetStaticKitID());
            } else if (kit == "media") {
                HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularMediaKit::GetStaticKitID());
            } else if (kit == "network") {
                HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularNetworkKit::GetStaticKitID());
            }
        }
        
        return true;
    }
};

// Example 3: Plugin-Extended Usage
class PluginExtendedHaikuEmulation {
public:
    bool Initialize() {
        if (!HAIKU_EMULATION_ENGINE.Initialize()) {
            return false;
        }
        
        // Load core kits
        LoadCoreKits();
        
        // Auto-discover and load plugins
        std::string plugin_path = HAIKU_CONFIG("plugin_path", "./plugins");
        HAIKU_EMULATION_ENGINE.GetPluginSystem().LoadAllPlugins(plugin_path);
        
        return true;
    }
    
    bool HandleCustomSyscall(uint32_t kit_id, uint32_t syscall_num, uint32_t* args, uint32_t* result) {
        return HAIKU_EMULATION_ENGINE.GetSyscallRouter().RouteSyscall(kit_id, syscall_num, args, result);
    }

private:
    void LoadCoreKits() {
        HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularInterfaceKit::GetStaticKitID());
        HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularMediaKit::GetStaticKitID());
        HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularNetworkKit::GetStaticKitID());
    }
};

// Example 4: Performance-Optimized Usage
class PerformanceOptimizedHaikuEmulation {
public:
    bool Initialize() {
        if (!HAIKU_EMULATION_ENGINE.Initialize()) {
            return false;
        }
        
        // Enable performance monitoring
        HAIKU_EMULATION_ENGINE.EnablePerformanceMonitoring(true);
        
        // Load kits with optimization
        LoadKitsWithOptimization();
        
        // Optimize syscall routing
        OptimizeSyscallRouting();
        
        return true;
    }
    
    void OptimizeForWorkload(const std::string& workload_type) {
        if (workload_type == "gui_intensive") {
            // Prioritize InterfaceKit
            OptimizeForGUI();
        } else if (workload_type == "audio_intensive") {
            // Prioritize MediaKit
            OptimizeForAudio();
        } else if (workload_type == "network_intensive") {
            // Prioritize NetworkKit
            OptimizeForNetwork();
        }
    }
    
    std::map<std::string, std::string> GetPerformanceReport() {
        std::map<std::string, std::string> report;
        
        // Get syscall statistics
        auto stats = HAIKU_EMULATION_ENGINE.GetSyscallRouter().GetAllStats();
        for (const auto& pair : stats) {
            uint32_t kit_id = pair.first;
            const auto& kit_stats = pair.second;
            
            std::string kit_prefix = "kit_" + std::to_string(kit_id) + "_";
            report[kit_prefix + "call_count"] = std::to_string(kit_stats.call_count);
            report[kit_prefix + "average_time_us"] = std::to_string(kit_stats.average_time_us);
            report[kit_prefix + "success_rate"] = std::to_string(
                (double)kit_stats.success_count / kit_stats.call_count * 100.0) + "%";
        }
        
        return report;
    }

private:
    void LoadKitsWithOptimization() {
        // Pre-load frequently used kits
        HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularInterfaceKit::GetStaticKitID());
        HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularMediaKit::GetStaticKitID());
        HAIKU_EMULATION_ENGINE.LoadKit(HaikuEmulation::ModularNetworkKit::GetStaticKitID());
    }
    
    void OptimizeSyscallRouting() {
        // Enable fast path for common syscalls
        // This would be implemented based on usage patterns
    }
    
    void OptimizeForGUI() {
        // Optimize for GUI workloads
        // Pre-allocate GUI resources, enable hardware acceleration, etc.
    }
    
    void OptimizeForAudio() {
        // Optimize for audio workloads
        // Pre-allocate audio buffers, enable real-time processing, etc.
    }
    
    void OptimizeForNetwork() {
        // Optimize for network workloads
        // Pre-allocate network buffers, enable connection pooling, etc.
    }
};

} // namespace Examples

} // namespace HaikuEmulation

/////////////////////////////////////////////////////////////////////////////
// Integration Macros for Easy Usage
/////////////////////////////////////////////////////////////////////////////

#define HAIKU_SMART_EMULATION HaikuEmulation::SmartHaikuEmulation::Instance()

#define HAIKU_INIT_SMART_EMULATION() HAIKU_SMART_EMULATION.Initialize()

#define HAIKU_HANDLE_SMART_SYSCALL(syscall, args, result) \
    HAIKU_SMART_EMULATION.HandleHaikuSyscall(syscall, args, result)

#define HAIKU_AUTO_CONFIGURE() HAIKU_SMART_EMULATION.AutoConfigure()

#define HAIKU_LOAD_REQUIRED_KITS(requirements) \
    HAIKU_SMART_EMULATION.LoadRequiredKits(requirements)

#define HAIKU_ENABLE_PERFORMANCE_OPTIMIZATION() \
    HAIKU_SMART_EMULATION.EnablePerformanceOptimization(true)