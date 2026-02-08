#ifndef PERFORMANCE_PROFILER_H
#define PERFORMANCE_PROFILER_H

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <string>
#include <functional>
#include <atomic>
#include <algorithm>
#include <cstdio>

// Forward declarations to avoid circular dependencies
typedef uint32_t vm_pid_t;
typedef uint32_t tid_t;

// Performance event types
enum class PerformanceEventType {
    INSTRUCTION_EXECUTION,
    MEMORY_ACCESS,
    SYSCALL_EXECUTION,
    CONTEXT_SWITCH,
    CACHE_MISS,
    CACHE_HIT,
    BRANCH_MISPREDICTION,
    BRANCH_PREDICTION,
    FUNCTION_CALL,
    FUNCTION_RETURN,
    MEMORY_ALLOCATION,
    MEMORY_DEALLOCATION,
    THREAD_CREATION,
    THREAD_TERMINATION,
    SOCKET_OPERATION,
    FILE_OPERATION,
    CUSTOM_EVENT
};

// Performance data structure
struct PerformanceEvent {
    uint64_t timestamp;
    PerformanceEventType event_type;
    uint64_t thread_id;
    uint64_t process_id;
    uint64_t value;
    std::string description;
    
    // Simple event-specific data (avoid union with non-trivial types)
    uint32_t instruction_address;
    uint32_t instruction_size;
    uint32_t memory_address;
    uint32_t access_size;
    bool is_read;
    uint32_t syscall_number;
    int32_t return_value;
    std::string function_name;
    uint32_t function_address;
    uint32_t allocation_size;
    void* pointer;
    std::string operation_type;
    int32_t result_code;
    
    PerformanceEvent() : timestamp(0), event_type(PerformanceEventType::CUSTOM_EVENT),
                        thread_id(0), process_id(0), value(0),
                        instruction_address(0), instruction_size(0),
                        memory_address(0), access_size(0), is_read(false),
                        syscall_number(0), return_value(0),
                        function_address(0), allocation_size(0), pointer(nullptr),
                        result_code(0) {}
};

// Performance counter
class PerformanceCounter {
private:
    std::atomic<uint64_t> count_{0};
    std::atomic<uint64_t> total_value_{0};
    std::atomic<uint64_t> min_value_{UINT64_MAX};
    std::atomic<uint64_t> max_value_{0};
    std::atomic<uint64_t> last_update_{0};
    
    mutable std::mutex counter_mutex_;
    
public:
    void increment(uint64_t value = 1) {
        count_.fetch_add(1);
        total_value_.fetch_add(value);
        
        // Update min/max (simplified, not perfectly atomic)
        uint64_t current_value = value;
        if (current_value < min_value_.load()) {
            min_value_.store(current_value);
        }
        if (current_value > max_value_.load()) {
            max_value_.store(current_value);
        }
        
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        last_update_.store(now);
    }
    
    void reset() {
        count_.store(0);
        total_value_.store(0);
        min_value_.store(UINT64_MAX);
        max_value_.store(0);
        last_update_.store(0);
    }
    
    uint64_t get_count() const { return count_.load(); }
    uint64_t get_total_value() const { return total_value_.load(); }
    uint64_t get_min_value() const { 
        uint64_t min_val = min_value_.load();
        return (min_val == UINT64_MAX) ? 0 : min_val;
    }
    uint64_t get_max_value() const { return max_value_.load(); }
    uint64_t get_average_value() const {
        uint64_t cnt = count_.load();
        return cnt > 0 ? total_value_.load() / cnt : 0;
    }
    uint64_t get_last_update() const { return last_update_.load(); }
};

// Performance profiler main class
class PerformanceProfiler {
private:
    std::vector<PerformanceEvent> events_;
    std::unordered_map<PerformanceEventType, std::unique_ptr<PerformanceCounter>> counters_;
    std::unordered_map<std::string, std::unique_ptr<PerformanceCounter>> custom_counters_;
    
    mutable std::mutex profiler_mutex_;
    std::atomic<bool> profiling_enabled_{true};
    std::atomic<uint64_t> events_collected_{0};
    std::atomic<uint64_t> events_dropped_{0};
    
    // Configuration
    size_t max_events_buffer_size_;
    bool enable_detailed_events_;
    uint64_t sampling_interval_ns_;
    
    // Statistics
    struct {
        std::atomic<uint64_t> total_events{0};
        std::atomic<uint64_t> instruction_events{0};
        std::atomic<uint64_t> memory_events{0};
        std::atomic<uint64_t> syscall_events{0};
        std::atomic<uint64_t> context_switch_events{0};
        std::atomic<uint64_t> cache_events{0};
        std::atomic<uint64_t> function_events{0};
        std::atomic<uint64_t> thread_events{0};
        std::atomic<uint64_t> io_events{0};
    } event_stats_;
    
public:
    PerformanceProfiler(size_t max_buffer_size = 100000, bool detailed_events = true, 
                        uint64_t sampling_interval = 1000)
        : max_events_buffer_size_(max_buffer_size), 
          enable_detailed_events_(detailed_events),
          sampling_interval_ns_(sampling_interval) {
        
        // Initialize standard performance counters
        initialize_standard_counters();
    }
    
    // Event recording
    void record_event(const PerformanceEvent& event) {
        if (!profiling_enabled_.load()) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(profiler_mutex_);
        
        // Check buffer size
        if (events_.size() >= max_events_buffer_size_) {
            events_dropped_.fetch_add(1);
            // Remove oldest event to make space
            if (!events_.empty()) {
                events_.erase(events_.begin());
            }
        }
        
        events_.push_back(event);
        events_collected_.fetch_add(1);
        event_stats_.total_events.fetch_add(1);
        
        // Update event-specific statistics
        switch (event.event_type) {
            case PerformanceEventType::INSTRUCTION_EXECUTION:
                event_stats_.instruction_events.fetch_add(1);
                break;
            case PerformanceEventType::MEMORY_ACCESS:
                event_stats_.memory_events.fetch_add(1);
                break;
            case PerformanceEventType::SYSCALL_EXECUTION:
                event_stats_.syscall_events.fetch_add(1);
                break;
            case PerformanceEventType::CONTEXT_SWITCH:
                event_stats_.context_switch_events.fetch_add(1);
                break;
            case PerformanceEventType::CACHE_MISS:
            case PerformanceEventType::CACHE_HIT:
                event_stats_.cache_events.fetch_add(1);
                break;
            case PerformanceEventType::FUNCTION_CALL:
            case PerformanceEventType::FUNCTION_RETURN:
                event_stats_.function_events.fetch_add(1);
                break;
            case PerformanceEventType::THREAD_CREATION:
            case PerformanceEventType::THREAD_TERMINATION:
                event_stats_.thread_events.fetch_add(1);
                break;
            case PerformanceEventType::SOCKET_OPERATION:
            case PerformanceEventType::FILE_OPERATION:
                event_stats_.io_events.fetch_add(1);
                break;
            default:
                break;
        }
        
        // Update corresponding counter
        auto it = counters_.find(event.event_type);
        if (it != counters_.end()) {
            it->second->increment(event.value);
        }
    }
    
    // Convenience methods for common events
    void record_instruction_execution(uint64_t thread_id, uint64_t process_id, 
                                     uint32_t instruction_address, uint32_t instruction_size) {
        if (!enable_detailed_events_) return;
        
        PerformanceEvent event;
        event.timestamp = get_current_timestamp();
        event.event_type = PerformanceEventType::INSTRUCTION_EXECUTION;
        event.thread_id = thread_id;
        event.process_id = process_id;
        event.value = instruction_size;
        event.instruction_address = instruction_address;
        event.instruction_size = instruction_size;
        
        record_event(event);
    }
    
    void record_memory_access(uint64_t thread_id, uint64_t process_id,
                             uint32_t memory_address, uint32_t access_size, bool is_read) {
        if (!enable_detailed_events_) return;
        
        PerformanceEvent event;
        event.timestamp = get_current_timestamp();
        event.event_type = PerformanceEventType::MEMORY_ACCESS;
        event.thread_id = thread_id;
        event.process_id = process_id;
        event.value = access_size;
        event.memory_address = memory_address;
        event.access_size = access_size;
        event.is_read = is_read;
        
        record_event(event);
    }
    
    void record_syscall_execution(uint64_t thread_id, uint64_t process_id,
                                 uint32_t syscall_number, int32_t return_value) {
        PerformanceEvent event;
        event.timestamp = get_current_timestamp();
        event.event_type = PerformanceEventType::SYSCALL_EXECUTION;
        event.thread_id = thread_id;
        event.process_id = process_id;
        event.value = static_cast<uint64_t>(return_value);
        event.syscall_number = syscall_number;
        event.return_value = return_value;
        
        record_event(event);
    }
    
    void record_context_switch(uint64_t from_thread_id, uint64_t to_thread_id, 
                              uint64_t process_id) {
        PerformanceEvent event;
        event.timestamp = get_current_timestamp();
        event.event_type = PerformanceEventType::CONTEXT_SWITCH;
        event.thread_id = from_thread_id;
        event.process_id = process_id;
        event.value = to_thread_id;
        event.description = "Context switch from thread " + std::to_string(from_thread_id) + 
                           " to thread " + std::to_string(to_thread_id);
        
        record_event(event);
    }
    
    void record_cache_miss(uint64_t thread_id, uint64_t process_id) {
        PerformanceEvent event;
        event.timestamp = get_current_timestamp();
        event.event_type = PerformanceEventType::CACHE_MISS;
        event.thread_id = thread_id;
        event.process_id = process_id;
        event.value = 1;
        event.description = "Cache miss";
        
        record_event(event);
    }
    
    void record_cache_hit(uint64_t thread_id, uint64_t process_id) {
        PerformanceEvent event;
        event.timestamp = get_current_timestamp();
        event.event_type = PerformanceEventType::CACHE_HIT;
        event.thread_id = thread_id;
        event.process_id = process_id;
        event.value = 1;
        event.description = "Cache hit";
        
        record_event(event);
    }
    
    void record_function_call(uint64_t thread_id, uint64_t process_id,
                             const std::string& function_name, uint32_t function_address) {
        PerformanceEvent event;
        event.timestamp = get_current_timestamp();
        event.event_type = PerformanceEventType::FUNCTION_CALL;
        event.thread_id = thread_id;
        event.process_id = process_id;
        event.value = 1;
        event.description = "Function call: " + function_name;
        event.function_name = function_name;
        event.function_address = function_address;
        
        record_event(event);
    }
    
    void record_thread_creation(uint64_t thread_id, uint64_t process_id) {
        PerformanceEvent event;
        event.timestamp = get_current_timestamp();
        event.event_type = PerformanceEventType::THREAD_CREATION;
        event.thread_id = thread_id;
        event.process_id = process_id;
        event.value = 1;
        event.description = "Thread created";
        
        record_event(event);
    }
    
    void record_socket_operation(uint64_t thread_id, uint64_t process_id,
                                const std::string& operation, int32_t result_code) {
        PerformanceEvent event;
        event.timestamp = get_current_timestamp();
        event.event_type = PerformanceEventType::SOCKET_OPERATION;
        event.thread_id = thread_id;
        event.process_id = process_id;
        event.value = static_cast<uint64_t>(result_code);
        event.description = "Socket operation: " + operation;
        event.operation_type = operation;
        event.result_code = result_code;
        
        record_event(event);
    }
    
    // Custom counter management
    PerformanceCounter* get_counter(PerformanceEventType event_type) {
        std::lock_guard<std::mutex> lock(profiler_mutex_);
        auto it = counters_.find(event_type);
        return (it != counters_.end()) ? it->second.get() : nullptr;
    }
    
    PerformanceCounter* get_custom_counter(const std::string& name) {
        std::lock_guard<std::mutex> lock(profiler_mutex_);
        auto it = custom_counters_.find(name);
        if (it != custom_counters_.end()) {
            return it->second.get();
        }
        
        // Create new counter if it doesn't exist
        auto counter = std::make_unique<PerformanceCounter>();
        PerformanceCounter* ptr = counter.get();
        custom_counters_[name] = std::move(counter);
        return ptr;
    }
    
    void increment_custom_counter(const std::string& name, uint64_t value = 1) {
        PerformanceCounter* counter = get_custom_counter(name);
        if (counter) {
            counter->increment(value);
        }
    }
    
    // Performance analysis
    struct PerformanceReport {
        struct EventStats {
            uint64_t total_events;
            uint64_t instruction_events;
            uint64_t memory_events;
            uint64_t syscall_events;
            uint64_t context_switch_events;
            uint64_t cache_events;
            uint64_t function_events;
            uint64_t thread_events;
            uint64_t io_events;
        } event_stats;
        
        struct CounterStats {
            uint64_t count;
            uint64_t total_value;
            uint64_t min_value;
            uint64_t max_value;
            uint64_t average_value;
        };
        
        std::unordered_map<PerformanceEventType, CounterStats> counters;
        std::unordered_map<std::string, CounterStats> custom_counters;
        
        uint64_t events_collected;
        uint64_t events_dropped;
        double collection_rate;
        bool profiling_enabled;
        size_t buffer_utilization;
    };
    
    PerformanceReport generate_report() const {
        std::lock_guard<std::mutex> lock(profiler_mutex_);
        
        PerformanceReport report;
        
        // Event statistics
        report.event_stats.total_events = event_stats_.total_events.load();
        report.event_stats.instruction_events = event_stats_.instruction_events.load();
        report.event_stats.memory_events = event_stats_.memory_events.load();
        report.event_stats.syscall_events = event_stats_.syscall_events.load();
        report.event_stats.context_switch_events = event_stats_.context_switch_events.load();
        report.event_stats.cache_events = event_stats_.cache_events.load();
        report.event_stats.function_events = event_stats_.function_events.load();
        report.event_stats.thread_events = event_stats_.thread_events.load();
        report.event_stats.io_events = event_stats_.io_events.load();
        
        // Counter statistics
        for (const auto& pair : counters_) {
            PerformanceEventType type = pair.first;
            const PerformanceCounter* counter = pair.second.get();
            
            PerformanceReport::CounterStats stats;
            stats.count = counter->get_count();
            stats.total_value = counter->get_total_value();
            stats.min_value = counter->get_min_value();
            stats.max_value = counter->get_max_value();
            stats.average_value = counter->get_average_value();
            
            report.counters[type] = stats;
        }
        
        // Custom counter statistics
        for (const auto& pair : custom_counters_) {
            const std::string& name = pair.first;
            const PerformanceCounter* counter = pair.second.get();
            
            PerformanceReport::CounterStats stats;
            stats.count = counter->get_count();
            stats.total_value = counter->get_total_value();
            stats.min_value = counter->get_min_value();
            stats.max_value = counter->get_max_value();
            stats.average_value = counter->get_average_value();
            
            report.custom_counters[name] = stats;
        }
        
        // Overall statistics
        report.events_collected = events_collected_.load();
        report.events_dropped = events_dropped_.load();
        report.collection_rate = report.events_dropped > 0 ? 
            static_cast<double>(report.events_dropped) / report.events_collected : 0.0;
        report.profiling_enabled = profiling_enabled_.load();
        report.buffer_utilization = static_cast<double>(events_.size()) / max_events_buffer_size_;
        
        return report;
    }
    
    void print_report() const {
        auto report = generate_report();
        
        printf("\n=== PERFORMANCE PROFILER REPORT ===\n");
        printf("Profiling Enabled: %s\n", report.profiling_enabled ? "YES" : "NO");
        printf("Events Collected: %lu\n", report.events_collected);
        printf("Events Dropped: %lu\n", report.events_dropped);
        printf("Collection Rate: %.2f%%\n", report.collection_rate * 100.0);
        printf("Buffer Utilization: %.1f%%\n", report.buffer_utilization * 100.0);
        
        printf("\n--- Event Statistics ---\n");
        printf("Total Events: %lu\n", report.event_stats.total_events);
        printf("Instruction Events: %lu\n", report.event_stats.instruction_events);
        printf("Memory Events: %lu\n", report.event_stats.memory_events);
        printf("Syscall Events: %lu\n", report.event_stats.syscall_events);
        printf("Context Switch Events: %lu\n", report.event_stats.context_switch_events);
        printf("Cache Events: %lu\n", report.event_stats.cache_events);
        printf("Function Events: %lu\n", report.event_stats.function_events);
        printf("Thread Events: %lu\n", report.event_stats.thread_events);
        printf("I/O Events: %lu\n", report.event_stats.io_events);
        
        printf("\n--- Performance Counters ---\n");
        for (const auto& pair : report.counters) {
            PerformanceEventType type = pair.first;
            const auto& stats = pair.second;
            
            const char* type_name = get_event_type_name(type);
            printf("%s: count=%lu, total=%lu, min=%lu, max=%lu, avg=%lu\n",
                   type_name, stats.count, stats.total_value, stats.min_value, 
                   stats.max_value, stats.average_value);
        }
        
        if (!report.custom_counters.empty()) {
            printf("\n--- Custom Counters ---\n");
            for (const auto& pair : report.custom_counters) {
                const std::string& name = pair.first;
                const auto& stats = pair.second;
                
                printf("%s: count=%lu, total=%lu, min=%lu, max=%lu, avg=%lu\n",
                       name.c_str(), stats.count, stats.total_value, stats.min_value,
                       stats.max_value, stats.average_value);
            }
        }
        
        printf("=====================================\n\n");
    }
    
    // Control operations
    void enable_profiling() { profiling_enabled_.store(true); }
    void disable_profiling() { profiling_enabled_.store(false); }
    bool is_profiling_enabled() const { return profiling_enabled_.load(); }
    
    void reset_all_counters() {
        std::lock_guard<std::mutex> lock(profiler_mutex_);
        
        for (auto& pair : counters_) {
            pair.second->reset();
        }
        
        for (auto& pair : custom_counters_) {
            pair.second->reset();
        }
        
        events_.clear();
        events_collected_.store(0);
        events_dropped_.store(0);
        
        // Reset event statistics
        event_stats_.total_events.store(0);
        event_stats_.instruction_events.store(0);
        event_stats_.memory_events.store(0);
        event_stats_.syscall_events.store(0);
        event_stats_.context_switch_events.store(0);
        event_stats_.cache_events.store(0);
        event_stats_.function_events.store(0);
        event_stats_.thread_events.store(0);
        event_stats_.io_events.store(0);
    }
    
    void clear_events() {
        std::lock_guard<std::mutex> lock(profiler_mutex_);
        events_.clear();
    }
    
    // Configuration
    void set_max_buffer_size(size_t size) { max_events_buffer_size_ = size; }
    void set_detailed_events(bool enabled) { enable_detailed_events_ = enabled; }
    void set_sampling_interval(uint64_t interval_ns) { sampling_interval_ns_ = interval_ns; }
    
    size_t get_max_buffer_size() const { return max_events_buffer_size_; }
    bool is_detailed_events_enabled() const { return enable_detailed_events_; }
    uint64_t get_sampling_interval() const { return sampling_interval_ns_; }
    
private:
    void initialize_standard_counters() {
        counters_[PerformanceEventType::INSTRUCTION_EXECUTION] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::MEMORY_ACCESS] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::SYSCALL_EXECUTION] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::CONTEXT_SWITCH] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::CACHE_MISS] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::CACHE_HIT] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::FUNCTION_CALL] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::FUNCTION_RETURN] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::THREAD_CREATION] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::THREAD_TERMINATION] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::SOCKET_OPERATION] = std::make_unique<PerformanceCounter>();
        counters_[PerformanceEventType::FILE_OPERATION] = std::make_unique<PerformanceCounter>();
    }
    
    uint64_t get_current_timestamp() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    
    const char* get_event_type_name(PerformanceEventType type) const {
        switch (type) {
            case PerformanceEventType::INSTRUCTION_EXECUTION: return "Instruction Execution";
            case PerformanceEventType::MEMORY_ACCESS: return "Memory Access";
            case PerformanceEventType::SYSCALL_EXECUTION: return "Syscall Execution";
            case PerformanceEventType::CONTEXT_SWITCH: return "Context Switch";
            case PerformanceEventType::CACHE_MISS: return "Cache Miss";
            case PerformanceEventType::CACHE_HIT: return "Cache Hit";
            case PerformanceEventType::BRANCH_MISPREDICTION: return "Branch Misprediction";
            case PerformanceEventType::BRANCH_PREDICTION: return "Branch Prediction";
            case PerformanceEventType::FUNCTION_CALL: return "Function Call";
            case PerformanceEventType::FUNCTION_RETURN: return "Function Return";
            case PerformanceEventType::MEMORY_ALLOCATION: return "Memory Allocation";
            case PerformanceEventType::MEMORY_DEALLOCATION: return "Memory Deallocation";
            case PerformanceEventType::THREAD_CREATION: return "Thread Creation";
            case PerformanceEventType::THREAD_TERMINATION: return "Thread Termination";
            case PerformanceEventType::SOCKET_OPERATION: return "Socket Operation";
            case PerformanceEventType::FILE_OPERATION: return "File Operation";
            case PerformanceEventType::CUSTOM_EVENT: return "Custom Event";
            default: return "Unknown Event";
        }
    }
};

// Global profiler instance
extern std::unique_ptr<PerformanceProfiler> g_performance_profiler;

// Performance profiling macros for easy usage
#define PROFILER_ENABLED() (g_performance_profiler && g_performance_profiler->is_profiling_enabled())
#define PROFILE_INSTRUCTION(thread_id, process_id, addr, size) \
    if (PROFILER_ENABLED()) g_performance_profiler->record_instruction_execution(thread_id, process_id, addr, size)
#define PROFILE_MEMORY_ACCESS(thread_id, process_id, addr, size, is_read) \
    if (PROFILER_ENABLED()) g_performance_profiler->record_memory_access(thread_id, process_id, addr, size, is_read)
#define PROFILE_SYSCALL(thread_id, process_id, syscall_num, ret_val) \
    if (PROFILER_ENABLED()) g_performance_profiler->record_syscall_execution(thread_id, process_id, syscall_num, ret_val)
#define PROFILE_CONTEXT_SWITCH(from_thread, to_thread, process_id) \
    if (PROFILER_ENABLED()) g_performance_profiler->record_context_switch(from_thread, to_thread, process_id)
#define PROFILE_CACHE_HIT(thread_id, process_id) \
    if (PROFILER_ENABLED()) g_performance_profiler->record_cache_hit(thread_id, process_id)
#define PROFILE_CACHE_MISS(thread_id, process_id) \
    if (PROFILER_ENABLED()) g_performance_profiler->record_cache_miss(thread_id, process_id)
#define PROFILE_FUNCTION_CALL(thread_id, process_id, name, addr) \
    if (PROFILER_ENABLED()) g_performance_profiler->record_function_call(thread_id, process_id, name, addr)
#define PROFILE_THREAD_CREATION(thread_id, process_id) \
    if (PROFILER_ENABLED()) g_performance_profiler->record_thread_creation(thread_id, process_id)
#define PROFILE_SOCKET_OPERATION(thread_id, process_id, operation, result) \
    if (PROFILER_ENABLED()) g_performance_profiler->record_socket_operation(thread_id, process_id, operation, result)
#define PROFILE_CUSTOM_COUNTER(name, value) \
    if (PROFILER_ENABLED()) g_performance_profiler->increment_custom_counter(name, value)

#endif // PERFORMANCE_PROFILER_H