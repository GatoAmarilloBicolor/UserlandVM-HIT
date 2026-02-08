#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

// Comprehensive Performance Profiling and Optimization System
// Provides detailed analysis of VM performance characteristics

struct PerformanceMetrics {
    std::string operation;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t duration_ns;
    uint64_t instruction_count;
    uint64_t memory_bytes_accessed;
    uint64_t syscall_count;
    double instructions_per_second;
    double cycles_per_instruction;
    size_t peak_memory_usage;
    size_t average_memory_usage;
    
    PerformanceMetrics() : start_time(0), end_time(0), duration_ns(0),
                          instruction_count(0), memory_bytes_accessed(0),
                          syscall_count(0), instructions_per_second(0.0),
                          cycles_per_instruction(0.0), peak_memory_usage(0),
                          average_memory_usage(0) {}
};

struct BenchmarkResult {
    std::string test_name;
    double native_time_ms;
    double vm_time_ms;
    double performance_ratio; // vm_time / native_time
    uint64_t instructions_executed;
    double native_ips;
    double vm_ips;
    double speedup_factor; // native_ips / vm_ips
    
    void PrintSummary() const {
        printf("=== %s BENCHMARK RESULTS ===\n", test_name.c_str());
        printf("Native Time:   %.3f ms (%.0f IPS)\n", native_time_ms, native_ips);
        printf("VM Time:       %.3f ms (%.0f IPS)\n", vm_time_ms, vm_ips);
        printf("Performance Ratio: %.3fx\n", performance_ratio);
        printf("Speedup Factor: %.3fx\n", speedup_factor);
        printf("Instructions:   %lu\n", instructions_executed);
        printf("=======================================\n\n");
    }
};

class PerformanceOptimizer {
private:
    std::vector<PerformanceMetrics> fMetrics;
    std::unordered_map<std::string, uint64_t> fOpcodeCounts;
    std::unordered_map<std::string, uint64_t> fSyscallCounts;
    uint64_t fStartTime;
    uint64_t fTotalInstructions;
    size_t fPeakMemoryUsage;
    size_t fCurrentMemoryUsage;
    
    // Performance optimization data
    std::vector<uint8_t> fInstructionCache;
    std::unordered_map<uint32_t, uint8_t> fOpcodeCache;
    std::vector<std::pair<uint32_t, uint32_t>> fJumpCache; // {jump_target, cache_line}
    
    static constexpr size_t CACHE_SIZE = 1024;
    static constexpr size_t JUMP_CACHE_SIZE = 64;
    
public:
    PerformanceOptimizer();
    ~PerformanceOptimizer();
    
    // Performance measurement
    void StartMeasurement(const std::string& operation);
    void EndMeasurement(const std::string& operation);
    void RecordInstruction(uint32_t opcode);
    void RecordSyscall(uint32_t syscall_num);
    void RecordMemoryAccess(uint32_t size);
    void UpdateMemoryUsage(size_t current_usage);
    
    // Optimization methods
    bool IsOpcodeCached(uint32_t opcode);
    void CacheOpcode(uint32_t opcode, uint8_t implementation);
    bool IsJumpTargetCached(uint32_t target);
    void CacheJumpTarget(uint32_t target, uint32_t cache_line);
    
    // Benchmarking
    BenchmarkResult RunBenchmark(const std::string& test_name, 
                             void (*native_test)(),
                             void (*vm_test)());
    
    void RunComprehensiveBenchmarks();
    
    // Analysis and optimization
    void AnalyzePerformance();
    void GenerateOptimizationReport();
    void OptimizeInstructionPath();
    void OptimizeMemoryAccessPattern();
    
    // Results and reporting
    void PrintPerformanceReport();
    void ExportCSVReport(const std::string& filename);
    std::vector<PerformanceMetrics> GetMetrics() const { return fMetrics; }
    
    // Static optimization utilities
    static uint64_t GetCurrentTimeNs();
    static double CalculateInstructionsPerSecond(uint64_t instructions, uint64_t duration_ns);
    static size_t EstimateOptimalCacheSize(uint64_t instruction_variety);
    static std::vector<uint32_t> IdentifyHotPaths(const std::vector<PerformanceMetrics>& metrics);
    
private:
    // Internal optimization helpers
    void OptimizeInstructionCache();
    void OptimizeBranchPrediction();
    void OptimizeMemoryPrefetching();
    void OptimizeSyscallDispatch();
    
    // Analysis helpers
    double CalculatePerformanceRatio();
    std::vector<std::string> IdentifyBottlenecks();
    std::unordered_map<std::string, double> CalculateOpcodeTimings();
    
    // Memory tracking
    void TrackMemoryAllocation(size_t size);
    void TrackMemoryDeallocation(size_t size);
    size_t GetEstimatedMemoryOverhead();
    
    // Benchmark helpers
    void RunCPUBenchmark();
    void RunMemoryBenchmark();
    void RunSyscallBenchmark();
    void RunIOBenchmark();
};

// Inline performance optimizations
class OptimizedOperations {
public:
    // Fast flag operations
    static inline uint32_t AddWithFlags(uint32_t a, uint32_t b, uint32_t& flags) {
        uint64_t result = (uint64_t)a + b;
        flags = 0;
        if (result & 0x100000000ULL) flags |= 0x1; // Carry
        flags |= ((result >> 32) & 0x80) ? 0x80 : 0; // Sign
        flags |= ((result >> 32) == 0) ? 0x40 : 0; // Zero
        return (uint32_t)result;
    }
    
    static inline uint32_t SubWithFlags(uint32_t a, uint32_t b, uint32_t& flags) {
        uint64_t result = (uint64_t)a - b;
        flags = 0;
        if (result & 0x100000000ULL) flags |= 0x1; // Borrow
        flags |= ((result >> 32) & 0x80) ? 0x80 : 0; // Sign
        flags |= ((result >> 32) == 0) ? 0x40 : 0; // Zero
        return (uint32_t)result;
    }
    
    // Optimized memory operations
    static inline void FastMemcpy(void* dst, const void* src, size_t n) {
        uint8_t* d = (uint8_t*)dst;
        const uint8_t* s = (const uint8_t*)src;
        
        // Unrolled copy for small sizes
        if (n >= 16) {
            while (n >= 16) {
                uint64_t a = *(uint64_t*)s;
                uint64_t b = *(uint64_t*)(s + 8);
                *(uint64_t*)d = a;
                *(uint64_t*)(d + 8) = b;
                s += 16;
                d += 16;
                n -= 16;
            }
        }
        
        // Handle remainder
        while (n--) {
            *d++ = *s++;
        }
    }
    
    // Optimized string operations
    static inline size_t FastStrlen(const char* str) {
        size_t len = 0;
        if (!str) return 0;
        
        // Word-aligned access for speed
        const uintptr_t* ptr = (const uintptr_t*)str;
        while (1) {
            uint64_t data = *ptr;
            if ((data & 0xFF) == 0) return len;
            if ((data & 0xFF00) == 0) return len + 1;
            if ((data & 0xFF0000) == 0) return len + 2;
            if ((data & 0xFF000000) == 0) return len + 3;
            if ((data & 0xFF00000000LL) == 0) return len + 4;
            if ((data & 0xFF0000000000LL) == 0) return len + 5;
            if ((data & 0xFF000000000000LL) == 0) return len + 6;
            if ((data & 0xFF00000000000000LL) == 0) return len + 7;
            ptr++;
            len += 8;
        }
    }
};

// Performance macros for instrumentation
#define PERF_START(name) \
    static PerformanceOptimizer* _perf = nullptr; \
    if (!_perf) _perf = new PerformanceOptimizer(); \
    _perf->StartMeasurement(name);

#define PERF_END(name) \
    if (_perf) _perf->EndMeasurement(name);

#define PERF_INSTRUCTION(opcode) \
    if (_perf) _perf->RecordInstruction(opcode);

#define PERF_SYSCALL(num) \
    if (_perf) _perf->RecordSyscall(num);

#define PERF_MEMORY_ACCESS(size) \
    if (_perf) _perf->RecordMemoryAccess(size);

// Auto-tuning system
class AutoTuner {
private:
    std::vector<double> fPerformanceHistory;
    double fCurrentPerformance;
    size_t fTuningWindow;
    
public:
    AutoTuner(size_t window_size = 100);
    
    void RecordPerformance(double performance);
    bool ShouldTune();
    double GetOptimalParameter();
    void Tune();
    
private:
    double CalculateTrend();
    double CalculateVariance();
};