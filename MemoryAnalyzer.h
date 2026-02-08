#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

// Advanced Memory Management and Analysis System
// Provides comprehensive memory tracking, optimization, and leak detection

struct MemoryBlock {
    uint8_t* address;
    size_t size;
    const char* file;
    int line;
    uint64_t timestamp;
    bool is_allocated;
    uint32_t magic;
    
    MemoryBlock() : address(nullptr), size(0), file(nullptr), line(0),
                   timestamp(0), is_allocated(false), magic(0xDEADBEEF) {}
};

struct MemoryStats {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
    size_t free_count;
    size_t fragmentation_count;
    double allocation_rate;
    double deallocation_rate;
    
    MemoryStats() : total_allocated(0), total_freed(0), current_usage(0),
                    peak_usage(0), allocation_count(0), free_count(0),
                    fragmentation_count(0), allocation_rate(0.0), deallocation_rate(0.0) {}
};

struct MemoryRegion {
    uint32_t start_addr;
    uint32_t end_addr;
    size_t size;
    uint32_t permissions;
    std::string name;
    bool is_code;
    bool is_data;
    bool is_stack;
    bool is_heap;
    uint64_t access_count;
    uint64_t last_access;
    
    MemoryRegion() : start_addr(0), end_addr(0), size(0), permissions(0),
                   is_code(false), is_data(false), is_stack(false), is_heap(false),
                   access_count(0), last_access(0) {}
};

class MemoryAnalyzer {
private:
    std::vector<MemoryBlock> fMemoryBlocks;
    std::unordered_map<uint8_t*, size_t> fBlockIndex;
    std::vector<MemoryRegion> fMemoryRegions;
    std::atomic<MemoryStats> fStats;
    std::mutex fMutex;
    
    // Memory optimization data
    std::unordered_map<uint32_t, uint8_t> fAccessCache;
    std::vector<std::pair<uint32_t, size_t>> fHotPages;
    std::vector<std::pair<uint32_t, uint32_t>> fAccessPatterns;
    
    // Configuration
    static constexpr size_t MAX_BLOCKS = 10000;
    static constexpr size_t CACHE_SIZE = 1024;
    static constexpr size_t HOT_PAGE_LIMIT = 256;
    static constexpr uint32_t BLOCK_MAGIC = 0xDEADBEEF;
    static constexpr uint32_t FREED_MAGIC = 0xFEEDFACE;
    
public:
    MemoryAnalyzer();
    ~MemoryAnalyzer();
    
    // Core memory management
    uint8_t* Allocate(size_t size, const char* file = nullptr, int line = 0);
    void Deallocate(uint8_t* ptr, const char* file = nullptr, int line = 0);
    uint8_t* Reallocate(uint8_t* ptr, size_t new_size, const char* file = nullptr, int line = 0);
    
    // Memory region management
    void RegisterRegion(uint32_t start_addr, size_t size, uint32_t permissions, const std::string& name);
    void UnregisterRegion(uint32_t start_addr);
    void MarkAccess(uint32_t addr, size_t size);
    
    // Analysis and optimization
    void AnalyzeMemoryUsage();
    void OptimizeMemoryLayout();
    void DetectMemoryLeaks();
    void ProfileMemoryAccess();
    
    // Statistics and reporting
    MemoryStats GetStats() const { return fStats.load(); }
    void PrintMemoryReport();
    void ExportMemoryMap(const std::string& filename);
    std::vector<MemoryRegion> GetMemoryRegions() const { return fMemoryRegions; }
    
    // Memory optimization utilities
    void* GetOptimizedPointer(uint32_t addr);
    bool IsMemoryHot(uint32_t addr);
    void PrefetchMemory(uint32_t addr, size_t size);
    
    // Advanced features
    void CompactMemory();
    void DefragmentMemory();
    size_t EstimateFragmentation();
    std::vector<uint32_t> FindUnusedRegions();
    
private:
    // Internal helpers
    size_t GetBlockSize(uint8_t* ptr);
    void UpdateStats(size_t allocated, size_t freed);
    void ValidateBlock(const MemoryBlock& block);
    size_t CalculateOverhead();
    void OptimizeAccessPatterns();
    
    // Cache operations
    bool IsInCache(uint32_t addr);
    void UpdateCache(uint32_t addr);
    void InvalidateCache(uint32_t addr);
    
    // Hot page tracking
    void TrackHotPage(uint32_t page_addr);
    std::vector<uint32_t> GetHotPages();
    
    // Memory leak detection
    std::vector<MemoryBlock> FindLeakedBlocks();
    void ReportLeaks(const std::vector<MemoryBlock>& leaks);
    
    // Thread safety
    void Lock() { fMutex.lock(); }
    void Unlock() { fMutex.unlock(); }
};

// Smart pointer for automatic memory management
template<typename T>
class ManagedPtr {
private:
    T* fPtr;
    MemoryAnalyzer* fAnalyzer;
    
public:
    ManagedPtr(MemoryAnalyzer* analyzer, T* ptr = nullptr) 
        : fPtr(ptr), fAnalyzer(analyzer) {}
    
    ~ManagedPtr() {
        if (fPtr && fAnalyzer) {
            fAnalyzer->Deallocate(reinterpret_cast<uint8_t*>(fPtr));
        }
    }
    
    // Prevent copying
    ManagedPtr(const ManagedPtr&) = delete;
    ManagedPtr& operator=(const ManagedPtr&) = delete;
    
    // Move semantics
    ManagedPtr(ManagedPtr&& other) noexcept 
        : fPtr(other.fPtr), fAnalyzer(other.fAnalyzer) {
        other.fPtr = nullptr;
    }
    
    ManagedPtr& operator=(ManagedPtr&& other) noexcept {
        if (this != &other) {
            if (fPtr && fAnalyzer) {
                fAnalyzer->Deallocate(reinterpret_cast<uint8_t*>(fPtr));
            }
            fPtr = other.fPtr;
            other.fPtr = nullptr;
        }
        return *this;
    }
    
    T* get() const { return fPtr; }
    T& operator*() const { return *fPtr; }
    T* operator->() const { return fPtr; }
    explicit operator bool() const { return fPtr != nullptr; }
    
    T* release() {
        T* temp = fPtr;
        fPtr = nullptr;
        return temp;
    }
};

// Memory pool for efficient allocation
class MemoryPool {
private:
    uint8_t* fPool;
    size_t fPoolSize;
    size_t fBlockSize;
    std::vector<bool> fUsedBlocks;
    size_t fNextFree;
    
public:
    MemoryPool(size_t pool_size, size_t block_size);
    ~MemoryPool();
    
    uint8_t* Allocate();
    void Deallocate(uint8_t* ptr);
    void Reset();
    size_t GetAvailableBlocks() const;
    size_t GetTotalBlocks() const { return fPoolSize / fBlockSize; }
    
private:
    size_t GetBlockIndex(uint8_t* ptr);
    void MarkBlockUsed(size_t index, bool used);
};

// Memory optimization macros
#define ANALYZE_ALLOC(size) \
    static MemoryAnalyzer* _mem_analyzer = nullptr; \
    if (!_mem_analyzer) _mem_analyzer = new MemoryAnalyzer(); \
    _mem_analyzer->Allocate(size, __FILE__, __LINE__);

#define ANALYZE_FREE(ptr) \
    if (_mem_analyzer) _mem_analyzer->Deallocate(ptr, __FILE__, __LINE__);

#define ANALYZE_REALLOC(ptr, size) \
    if (_mem_analyzer) _mem_analyzer->Reallocate(ptr, size, __FILE__, __LINE__);

#define ANALYZE_TRACK_ACCESS(addr, size) \
    if (_mem_analyzer) _mem_analyzer->MarkAccess(addr, size);

// Memory access pattern analyzer
class AccessPatternAnalyzer {
private:
    std::unordered_map<uint32_t, std::vector<uint64_t>> fAccessTimes;
    std::unordered_map<uint32_t, std::vector<uint32_t>> fAccessSizes;
    std::unordered_map<uint32_t, uint32_t> fAccessCounts;
    
public:
    void RecordAccess(uint32_t addr, size_t size);
    void AnalyzePatterns();
    std::vector<uint32_t> PredictNextAccesses(uint32_t current_addr);
    bool IsSequentialAccess(uint32_t addr);
    bool IsRandomAccess(uint32_t addr);
    void PrintPatternReport();
    
private:
    double CalculateEntropy(uint32_t addr);
    std::vector<uint32_t> FindSequentialRanges();
};

// Memory defragmentation system
class MemoryDefragmenter {
public:
    struct DefragmentResult {
        size_t blocks_moved;
        size_t bytes_freed;
        double fragmentation_before;
        double fragmentation_after;
        size_t largest_free_block;
    };
    
    DefragmentResult Defragment(MemoryAnalyzer* analyzer);
    void AnalyzeFragmentation(MemoryAnalyzer* analyzer);
    
private:
    size_t CalculateExternalFragmentation(const std::vector<MemoryBlock>& blocks);
    size_t CalculateInternalFragmentation(const std::vector<MemoryBlock>& blocks);
    std::vector<MemoryBlock> FindMovableBlocks(MemoryAnalyzer* analyzer);
};