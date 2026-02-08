/**
 * @file UltimateUserlandVM.h
 * @brief Ultimate UserlandVM with complete ET_DYN integration and 4GB support
 */

#pragma once

#include "UnifiedDefinitionsCorrected.h"
#include "EnhancedDirectAddressSpace.h"
#include "CompleteETDynRelocator.h"
#include "ETDynIntegration.h"
#include "AlmightyOpcodeHandler.h"
#include <memory>
#include <string>

class UltimateUserlandVM {
public:
    struct VMConfig {
        uint32_t memory_size_gb;
        bool enable_et_dyn_relocation;
        bool enable_performance_monitoring;
        bool enable_debug_logging;
        uint32_t et_dyn_load_base;
        bool use_opcode_handler;
        
        VMConfig() : memory_size_gb(4),
                     enable_et_dyn_relocation(true),
                     enable_performance_monitoring(false),
                     enable_debug_logging(false),
                     et_dyn_load_base(0x08000000),
                     use_opcode_handler(true) {}
    };

    struct ExecutionResult {
        bool success;
        uint32_t exit_code;
        std::string error_message;
        uint64_t cycles_executed;
        uint32_t instructions_executed;
    };

private:
    VMConfig fConfig;
    
    // Core VM components
    std::unique_ptr<EnhancedDirectAddressSpace> fAddressSpace;
    std::unique_ptr<CompleteETDynRelocator> fRelocator;
    std::unique_ptr<ETDynIntegration> fETDynIntegration;
    std::unique_ptr<AlmightyOpcodeHandler> fOpcodeHandler;
    
    // VM state
    bool fInitialized;
    bool fBinaryLoaded;
    uint32_t fEntryAddress;
    uint32_t fCurrentEIP;
    
    // Performance tracking
    uint64_t fTotalCycles;
    uint32_t fInstructionCount;

public:
    UltimateUserlandVM(const VMConfig& config = VMConfig());
    virtual ~UltimateUserlandVM();
    
    // VM lifecycle
    status_t Initialize();
    status_t Shutdown();
    
    // Binary loading
    ExecutionResult LoadBinary(const void* binary_data, size_t binary_size);
    ExecutionResult ExecuteLoadedBinary();
    ExecutionResult ExecuteFromAddress(uint32_t start_address);
    
    // ET_DYN specific loading
    ExecutionResult LoadETDynBinary(const void* binary_data, size_t binary_size);
    
    // Memory management
    status_t AllocateMemory(uint32_t size, uint32_t* allocated_address);
    status_t SetMemoryProtection(uint32_t address, size_t size, uint32_t protection);
    status_t ReadMemory(uint32_t address, void* buffer, size_t size);
    status_t WriteMemory(uint32_t address, const void* buffer, size_t size);
    
    // Execution control
    ExecutionResult ExecuteInstruction();
    ExecutionResult ExecuteInstructions(uint32_t count);
    ExecutionResult ExecuteUntilAddress(uint32_t stop_address);
    
    // Debug and introspection
    void DumpMemoryLayout();
    void DumpExecutionState();
    void DumpRelocationInfo();
    void DumpPerformanceStats();
    
    // Configuration
    void SetConfig(const VMConfig& config) { fConfig = config; }
    const VMConfig& GetConfig() const { return fConfig; }
    
    // Status
    bool IsInitialized() const { return fInitialized; }
    bool IsBinaryLoaded() const { return fBinaryLoaded; }
    uint32_t GetEntryAddress() const { return fEntryAddress; }
    uint32_t GetCurrentEIP() const { return fCurrentEIP; }

private:
    // Internal helper methods
    status_t CreateAddressSpace();
    status_t InitializeRelocator();
    status_t InitializeETDynIntegration();
    status_t InitializeOpcodeHandler();
    
    // Address space helpers
    uint32_t CalculateLoadAddress(const void* binary_data, size_t binary_size);
    bool ValidateAddress(uint32_t address, size_t size = 1);
    
    // Execution helpers
    status_t SetupExecutionEnvironment();
    status_t CleanupExecutionEnvironment();
    ExecutionResult HandleExecutionError(const std::string& error);
    
    // Logging helpers
    void LogInfo(const std::string& message);
    void LogError(const std::string& error);
    void LogDebug(const std::string& message);
    
    // Constants
    static const uint32_t DEFAULT_STACK_BASE = 0xF0000000;
    static const uint32_t DEFAULT_STACK_SIZE = 0x00100000;  // 1MB
    static const uint32_t MIN_MEMORY_SIZE = 0x10000000;     // 256MB
    static const uint32_t MAX_MEMORY_SIZE = 0x100000000;    // 4GB
};

// Utility class for VM creation and management
class VMFactory {
public:
    static std::unique_ptr<UltimateUserlandVM> CreateStandardVM();
    static std::unique_ptr<UltimateUserlandVM> CreateETDynVM();
    static std::unique_ptr<UltimateUserlandVM> CreateDebugVM();
    static std::unique_ptr<UltimateUserlandVM> CreatePerformanceVM();
};