#pragma once

#include "ExecutionEngine.h"
#include "EnhancedInterpreterX86_32.h"
#include "SimpleSyscallDispatcher.h"
#include "ETDynRelocator.h"
#include "PerformanceOptimizer.h"
#include "MemoryAnalyzer.h"
#include <memory>
#include <vector>
#include <string>

// Unified VM Execution Engine
// Integrates all enhanced components into a cohesive system

class UnifiedExecutionEngine : public ExecutionEngine {
private:
    // Core components
    std::unique_ptr<EnhancedInterpreterX86_32> fInterpreter;
    std::unique_ptr<SimpleSyscallDispatcher> fSyscallDispatcher;
    std::unique_ptr<ETDynRelocator> fRelocator;
    std::unique_ptr<PerformanceOptimizer> fPerformanceOptimizer;
    std::unique_ptr<MemoryAnalyzer> fMemoryAnalyzer;
    
    // Execution state
    bool fIsRunning;
    bool fIsPaused;
    bool fIsDebugging;
    uint64_t fExecutionStartTime;
    uint64_t fTotalInstructionsExecuted;
    uint32_t fCurrentBreakpoint;
    
    // Multi-threading support
    std::vector<std::thread> fWorkerThreads;
    std::mutex fExecutionMutex;
    std::condition_variable fExecutionCondition;
    
    // Configuration
    struct ExecutionConfig {
        bool enable_performance_profiling;
        bool enable_memory_analysis;
        bool enable_debugging;
        bool enable_optimizations;
        size_t max_memory_usage;
        uint32_t timeout_ms;
        uint32_t instruction_limit;
    } fConfig;
    
public:
    UnifiedExecutionEngine();
    ~UnifiedExecutionEngine() override;
    
    // Core execution interface
    status_t Initialize() override;
    status_t Cleanup() override;
    status_t LoadProgram(const std::string& filename);
    status_t Execute() override;
    status_t Pause();
    status_t Resume();
    status_t Stop();
    status_t SingleStep();
    
    // Enhanced features
    status_t LoadETDynProgram(const std::string& filename);
    status_t SetBreakpoint(uint32_t address);
    status_t RemoveBreakpoint(uint32_t address);
    status_t EnablePerformanceProfiling(bool enable);
    status_t EnableMemoryAnalysis(bool enable);
    status_t EnableDebugMode(bool enable);
    
    // Analysis and reporting
    void PrintExecutionReport();
    void PrintPerformanceReport();
    void PrintMemoryReport();
    void ExportPerformanceData(const std::string& filename);
    void ExportMemoryAnalysis(const std::string& filename);
    
    // Configuration
    void SetConfiguration(const ExecutionConfig& config);
    ExecutionConfig GetConfiguration() const { return fConfig; }
    
    // State queries
    bool IsRunning() const { return fIsRunning; }
    bool IsPaused() const { return fIsPaused; }
    bool IsDebugging() const { return fIsDebugging; }
    uint64_t GetExecutionTime() const;
    uint64_t GetInstructionCount() const { return fTotalInstructionsExecuted; }
    
protected:
    // Virtual interface implementation
    status_t ExecuteInstruction(GuestContext& context) override;
    
private:
    // Internal execution methods
    status_t ExecuteWithInterpreter(GuestContext& context);
    status_t ExecuteWithOptimizations(GuestContext& context);
    status_t ExecuteInDebugMode(GuestContext& context);
    
    // Component integration
    status_t InitializeInterpreter();
    status_t InitializeSyscalls();
    status_t InitializeRelocator();
    status_t InitializePerformanceOptimizer();
    status_t InitializeMemoryAnalyzer();
    
    // Multi-threading support
    void StartWorkerThreads(size_t thread_count);
    void StopWorkerThreads();
    void WorkerThreadLoop(size_t thread_id);
    
    // Debug support
    status_t HandleBreakpoint(GuestContext& context);
    status_t DebugStep(GuestContext& context);
    void PrintDebugInfo(GuestContext& context);
    
    // Performance optimization
    void OptimizeExecution();
    status_t ApplyOptimizations(GuestContext& context);
    
    // Error handling
    void HandleExecutionError(status_t error, const std::string& context);
    void HandleTimeout();
    void HandleMemoryExhaustion();
    
    // Synchronization
    void LockExecution() { fExecutionMutex.lock(); }
    void UnlockExecution() { fExecutionMutex.unlock(); }
    void WaitForWorkers();
    void NotifyWorkers();
};

// Execution Engine Factory
class ExecutionEngineFactory {
public:
    enum EngineType {
        UNIFIED_OPTIMIZED,
        UNIFIED_DEBUG,
        UNIFIED_PERFORMANCE,
        LEGACY_BASIC
    };
    
    static std::unique_ptr<ExecutionEngine> CreateEngine(EngineType type);
    static std::vector<std::string> GetSupportedEngines();
    static std::string GetEngineDescription(EngineType type);
};

// Advanced Execution Features
class JITCompiler {
public:
    struct JITCodeBlock {
        uint8_t* code;
        size_t size;
        uint32_t entry_point;
        bool is_active;
        
        JITCodeBlock() : code(nullptr), size(0), entry_point(0), is_active(false) {}
    };
    
    JITCompiler();
    ~JITCompiler();
    
    status_t CompileBlock(const std::vector<uint32_t>& instructions, JITCodeBlock& block);
    status_t ExecuteBlock(JITCodeBlock& block);
    void InvalidateBlock(JITCodeBlock& block);
    void ClearCache();
    
private:
    std::vector<JITCodeBlock> fCodeCache;
    void* fExecutableMemory;
    size_t fMemorySize;
};

// Thread Pool for parallel execution
class ThreadPool {
private:
    std::vector<std::thread> fThreads;
    std::queue<std::function<void()>> fTaskQueue;
    std::mutex fQueueMutex;
    std::condition_variable fCondition;
    bool fShutdown;
    
public:
    ThreadPool(size_t thread_count);
    ~ThreadPool();
    
    void SubmitTask(std::function<void()> task);
    void Shutdown();
    size_t GetQueueSize() const;
    
private:
    void WorkerLoop();
};

// Performance monitoring and auto-tuning
class PerformanceMonitor {
private:
    struct PerformanceSnapshot {
        double cpu_usage;
        size_t memory_usage;
        double instructions_per_second;
        double syscalls_per_second;
        uint64_t cache_hit_rate;
        uint64_t timestamp;
    };
    
    std::vector<PerformanceSnapshot> fSnapshots;
    std::thread fMonitoringThread;
    std::atomic<bool> fMonitoringActive;
    
public:
    PerformanceMonitor();
    ~PerformanceMonitor();
    
    void StartMonitoring();
    void StopMonitoring();
    PerformanceSnapshot GetCurrentSnapshot();
    std::vector<PerformanceSnapshot> GetHistory();
    void PrintPerformanceTrends();
    
private:
    void MonitoringLoop();
    PerformanceSnapshot CollectSnapshot();
    void AnalyzeTrends();
};

// Macros for easy integration
#define UNIFIED_ENGINE_EXECUTE(engine, filename) \
    do { \
        auto unified_engine = std::static_pointer_cast<UnifiedExecutionEngine>(engine); \
        unified_engine->LoadProgram(filename); \
        unified_engine->Execute(); \
    } while(0)

#define UNIFIED_ENGINE_DEBUG_STEP(engine) \
    do { \
        auto unified_engine = std::static_pointer_cast<UnifiedExecutionEngine>(engine); \
        unified_engine->SingleStep(); \
    } while(0)

#define UNIFIED_ENGINE_SET_BREAKPOINT(engine, addr) \
    do { \
        auto unified_engine = std::static_pointer_cast<UnifiedExecutionEngine>(engine); \
        unified_engine->SetBreakpoint(addr); \
    } while(0)