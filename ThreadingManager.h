#ifndef THREADING_MANAGER_H
#define THREADING_MANAGER_H

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <cstdio>

// Forward declarations to avoid circular dependencies
typedef uint32_t addr_t;
typedef uint32_t vm_pid_t;
typedef uint32_t tid_t;

enum class ThreadState {
    CREATED,
    READY,
    RUNNING,
    BLOCKED,
    SLEEPING,
    TERMINATED,
    ZOMBIE
};

enum class ThreadPriority {
    IDLE = 0,
    LOW = 1,
    NORMAL = 2,
    HIGH = 3,
    CRITICAL = 4
};

// Thread context for 32-bit execution
struct ThreadContext {
    // General purpose registers (32-bit)
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp;
    
    // Segment registers
    uint16_t cs, ds, es, fs, gs, ss;
    
    // Instruction pointer and flags
    uint32_t eip;
    uint32_t eflags;
    
    // Thread metadata
    tid_t thread_id;
    vm_pid_t parent_process_id;
    ThreadState state;
    ThreadPriority priority;
    
    // Stack information
    addr_t stack_base;
    addr_t stack_limit;
    uint32_t stack_size;
    
    // Scheduling information
    uint64_t creation_time;
    uint64_t last_execution_time;
    uint64_t quantum_remaining;
    uint32_t execution_time_slice;
    
    // Synchronization
    std::atomic<bool> can_execute{false};
    std::condition_variable* wait_condition = nullptr;
    
    // Memory management
    std::unique_ptr<uint8_t[]> stack_memory;
    
    ThreadContext() {
        // Initialize all registers to 0
        eax = ebx = ecx = edx = esi = edi = ebp = esp = 0;
        cs = ds = es = fs = gs = ss = 0;
        eip = eflags = 0;
        thread_id = 0;
        parent_process_id = 0;
        state = ThreadState::CREATED;
        priority = ThreadPriority::NORMAL;
        stack_base = stack_limit = 0;
        stack_size = 0;
        creation_time = last_execution_time = 0;
        quantum_remaining = 1000; // Default quantum
        execution_time_slice = 1000;
    }
};

// Synchronization primitives
class Mutex {
private:
    std::atomic<bool> locked_{false};
    tid_t owner_thread_{0};
    uint32_t lock_count_{0};
    std::condition_variable cv_;
    std::mutex mtx_;
    
public:
    bool lock(tid_t thread_id) {
        std::unique_lock<std::mutex> lock(mtx_);
        
        if (owner_thread_ == thread_id) {
            lock_count_++;
            return true;
        }
        
        cv_.wait(lock, [this] { return !locked_; });
        
        locked_ = true;
        owner_thread_ = thread_id;
        lock_count_ = 1;
        
        return true;
    }
    
    bool unlock(tid_t thread_id) {
        std::unique_lock<std::mutex> lock(mtx_);
        
        if (owner_thread_ != thread_id) {
            return false;
        }
        
        lock_count_--;
        if (lock_count_ == 0) {
            locked_ = false;
            owner_thread_ = 0;
            cv_.notify_one();
        }
        
        return true;
    }
    
    bool try_lock(tid_t thread_id) {
        std::lock_guard<std::mutex> lock(mtx_);
        
        if (locked_ && owner_thread_ != thread_id) {
            return false;
        }
        
        if (owner_thread_ == thread_id) {
            lock_count_++;
        } else {
            locked_ = true;
            owner_thread_ = thread_id;
            lock_count_ = 1;
        }
        
        return true;
    }
};

class Semaphore {
private:
    std::atomic<int32_t> count_;
    std::condition_variable cv_;
    std::mutex mtx_;
    
public:
    explicit Semaphore(int32_t initial_count = 0) : count_(initial_count) {}
    
    bool wait(tid_t thread_id) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return count_ > 0; });
        count_--;
        return true;
    }
    
    bool try_wait() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (count_ > 0) {
            count_--;
            return true;
        }
        return false;
    }
    
    void post() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            count_++;
        }
        cv_.notify_one();
    }
    
    int32_t get_count() const { return count_.load(); }
};

// Thread scheduler
class ThreadScheduler {
private:
    struct ReadyQueue {
        std::vector<tid_t> threads;
        size_t current_index = 0;
        
        void add_thread(tid_t thread_id) {
            threads.push_back(thread_id);
        }
        
        tid_t get_next_thread() {
            if (threads.empty()) return 0;
            
            if (current_index >= threads.size()) {
                current_index = 0;
            }
            
            return threads[current_index++];
        }
        
        void remove_thread(tid_t thread_id) {
            for (size_t i = 0; i < threads.size(); i++) {
                if (threads[i] == thread_id) {
                    threads.erase(threads.begin() + i);
                    if (current_index >= threads.size() && current_index > 0) {
                        current_index--;
                    }
                    break;
                }
            }
        }
        
        size_t size() const { return threads.size(); }
        bool empty() const { return threads.empty(); }
    };
    
    std::unordered_map<ThreadPriority, ReadyQueue> ready_queues_;
    std::unordered_map<tid_t, std::shared_ptr<ThreadContext>> threads_;
    std::unordered_map<tid_t, std::shared_ptr<std::thread>> native_threads_;
    
    std::mutex scheduler_mutex_;
    std::condition_variable scheduler_cv_;
    std::atomic<bool> scheduler_running_{true};
    std::atomic<tid_t> current_thread_{0};
    tid_t next_thread_id_ = 1;
    
    // Statistics
    struct {
        std::atomic<uint64_t> total_threads_created{0};
        std::atomic<uint64_t> total_threads_terminated{0};
        std::atomic<uint64_t> context_switches{0};
        std::atomic<uint64_t> scheduler_iterations{0};
    } stats_;
    
public:
    ThreadScheduler() {
        // Initialize ready queues for each priority level
        ready_queues_[ThreadPriority::IDLE] = ReadyQueue{};
        ready_queues_[ThreadPriority::LOW] = ReadyQueue{};
        ready_queues_[ThreadPriority::NORMAL] = ReadyQueue{};
        ready_queues_[ThreadPriority::HIGH] = ReadyQueue{};
        ready_queues_[ThreadPriority::CRITICAL] = ReadyQueue{};
    }
    
    tid_t create_thread(vm_pid_t process_id, addr_t entry_point, uint32_t stack_size = 64 * 1024) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        tid_t thread_id = next_thread_id_++;
        
        auto context = std::make_shared<ThreadContext>();
        context->thread_id = thread_id;
        context->parent_process_id = process_id;
        context->state = ThreadState::CREATED;
        context->priority = ThreadPriority::NORMAL;
        context->stack_size = stack_size;
        context->stack_memory = std::make_unique<uint8_t[]>(stack_size);
        context->stack_base = static_cast<addr_t>(reinterpret_cast<uintptr_t>(context->stack_memory.get()) + stack_size);
        context->stack_limit = static_cast<addr_t>(reinterpret_cast<uintptr_t>(context->stack_memory.get()));
        context->esp = context->stack_base;
        context->eip = entry_point;
        context->creation_time = get_current_time();
        
        threads_[thread_id] = context;
        ready_queues_[ThreadPriority::NORMAL].add_thread(thread_id);
        
        stats_.total_threads_created++;
        
        return thread_id;
    }
    
    bool set_thread_priority(tid_t thread_id, ThreadPriority priority) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        auto it = threads_.find(thread_id);
        if (it == threads_.end()) return false;
        
        ThreadPriority old_priority = it->second->priority;
        if (old_priority != priority) {
            // Remove from old queue
            ready_queues_[old_priority].remove_thread(thread_id);
            
            // Add to new queue
            it->second->priority = priority;
            ready_queues_[priority].add_thread(thread_id);
        }
        
        return true;
    }
    
    tid_t schedule_next_thread() {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        stats_.scheduler_iterations++;
        
        // Check queues in priority order: CRITICAL -> HIGH -> NORMAL -> LOW -> IDLE
        std::vector<ThreadPriority> priority_order = {
            ThreadPriority::CRITICAL, ThreadPriority::HIGH, ThreadPriority::NORMAL,
            ThreadPriority::LOW, ThreadPriority::IDLE
        };
        
        for (ThreadPriority priority : priority_order) {
            auto& queue = ready_queues_[priority];
            if (!queue.empty()) {
                tid_t next_thread = queue.get_next_thread();
                
                auto it = threads_.find(next_thread);
                if (it != threads_.end() && it->second->state == ThreadState::READY) {
                    current_thread_ = next_thread;
                    stats_.context_switches++;
                    return next_thread;
                }
            }
        }
        
        return 0; // No threads ready
    }
    
    bool block_thread(tid_t thread_id, ThreadState new_state = ThreadState::BLOCKED) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        auto it = threads_.find(thread_id);
        if (it == threads_.end()) return false;
        
        it->second->state = new_state;
        return true;
    }
    
    bool unblock_thread(tid_t thread_id) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        auto it = threads_.find(thread_id);
        if (it == threads_.end()) return false;
        
        it->second->state = ThreadState::READY;
        ready_queues_[it->second->priority].add_thread(thread_id);
        return true;
    }
    
    bool terminate_thread(tid_t thread_id) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        auto it = threads_.find(thread_id);
        if (it == threads_.end()) return false;
        
        it->second->state = ThreadState::TERMINATED;
        
        // Remove from ready queue
        ready_queues_[it->second->priority].remove_thread(thread_id);
        
        stats_.total_threads_terminated++;
        
        // Clean up native thread if exists
        auto native_it = native_threads_.find(thread_id);
        if (native_it != native_threads_.end()) {
            if (native_it->second && native_it->second->joinable()) {
                native_it->second->join();
            }
            native_threads_.erase(native_it);
        }
        
        return true;
    }
    
    std::shared_ptr<ThreadContext> get_thread_context(tid_t thread_id) {
        std::lock_guard<std::mutex> lock(scheduler_mutex_);
        
        auto it = threads_.find(thread_id);
        if (it != threads_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    tid_t get_current_thread() const { return current_thread_.load(); }
    
    // Statistics
    struct SchedulerStats {
        uint64_t total_threads_created;
        uint64_t total_threads_terminated;
        uint64_t context_switches;
        uint64_t scheduler_iterations;
        uint64_t active_threads;
        uint64_t ready_threads;
    };
    
    SchedulerStats get_statistics() const {
        SchedulerStats result;
        result.total_threads_created = stats_.total_threads_created.load();
        result.total_threads_terminated = stats_.total_threads_terminated.load();
        result.context_switches = stats_.context_switches.load();
        result.scheduler_iterations = stats_.scheduler_iterations.load();
        
        // Count active and ready threads
        uint64_t active = 0, ready = 0;
        for (const auto& pair : threads_) {
            if (pair.second->state != ThreadState::TERMINATED && 
                pair.second->state != ThreadState::ZOMBIE) {
                active++;
            }
            if (pair.second->state == ThreadState::READY) {
                ready++;
            }
        }
        result.active_threads = active;
        result.ready_threads = ready;
        
        return result;
    }
    
    void print_statistics() const {
        auto stats = get_statistics();
        
        printf("\n=== THREAD SCHEDULER STATISTICS ===\n");
        printf("Total Threads Created: %lu\n", stats.total_threads_created);
        printf("Total Threads Terminated: %lu\n", stats.total_threads_terminated);
        printf("Active Threads: %lu\n", stats.active_threads);
        printf("Ready Threads: %lu\n", stats.ready_threads);
        printf("Context Switches: %lu\n", stats.context_switches);
        printf("Scheduler Iterations: %lu\n", stats.scheduler_iterations);
        printf("Current Thread: TID %u\n", current_thread_.load());
        printf("====================================\n\n");
    }
    
private:
    uint64_t get_current_time() const {
        // Simple time implementation (would use system clock in real system)
        static std::atomic<uint64_t> time_counter{0};
        return time_counter.fetch_add(1);
    }
};

// Main threading manager
class ThreadingManager {
private:
    std::unique_ptr<ThreadScheduler> scheduler_;
    std::unordered_map<tid_t, std::unique_ptr<Mutex>> mutexes_;
    std::unordered_map<tid_t, std::unique_ptr<Semaphore>> semaphores_;
    std::mutex manager_mutex_;
    
    tid_t next_mutex_id_ = 1000;
    tid_t next_semaphore_id_ = 2000;
    
public:
    ThreadingManager() : scheduler_(std::make_unique<ThreadScheduler>()) {}
    
    // Thread management
    tid_t create_thread(vm_pid_t process_id, addr_t entry_point, uint32_t stack_size = 64 * 1024) {
        return scheduler_->create_thread(process_id, entry_point, stack_size);
    }
    
    bool set_thread_priority(tid_t thread_id, ThreadPriority priority) {
        return scheduler_->set_thread_priority(thread_id, priority);
    }
    
    tid_t schedule_next_thread() {
        return scheduler_->schedule_next_thread();
    }
    
    bool block_thread(tid_t thread_id, ThreadState state = ThreadState::BLOCKED) {
        return scheduler_->block_thread(thread_id, state);
    }
    
    bool unblock_thread(tid_t thread_id) {
        return scheduler_->unblock_thread(thread_id);
    }
    
    bool terminate_thread(tid_t thread_id) {
        return scheduler_->terminate_thread(thread_id);
    }
    
    std::shared_ptr<ThreadContext> get_thread_context(tid_t thread_id) {
        return scheduler_->get_thread_context(thread_id);
    }
    
    // Synchronization primitive management
    tid_t create_mutex() {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        tid_t mutex_id = next_mutex_id_++;
        mutexes_[mutex_id] = std::make_unique<Mutex>();
        return mutex_id;
    }
    
    bool destroy_mutex(tid_t mutex_id) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        return mutexes_.erase(mutex_id) > 0;
    }
    
    bool mutex_lock(tid_t mutex_id, tid_t thread_id) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        auto it = mutexes_.find(mutex_id);
        if (it != mutexes_.end()) {
            return it->second->lock(thread_id);
        }
        return false;
    }
    
    bool mutex_unlock(tid_t mutex_id, tid_t thread_id) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        auto it = mutexes_.find(mutex_id);
        if (it != mutexes_.end()) {
            return it->second->unlock(thread_id);
        }
        return false;
    }
    
    bool mutex_try_lock(tid_t mutex_id, tid_t thread_id) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        auto it = mutexes_.find(mutex_id);
        if (it != mutexes_.end()) {
            return it->second->try_lock(thread_id);
        }
        return false;
    }
    
    tid_t create_semaphore(int32_t initial_count = 0) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        tid_t semaphore_id = next_semaphore_id_++;
        semaphores_[semaphore_id] = std::make_unique<Semaphore>(initial_count);
        return semaphore_id;
    }
    
    bool destroy_semaphore(tid_t semaphore_id) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        return semaphores_.erase(semaphore_id) > 0;
    }
    
    bool semaphore_wait(tid_t semaphore_id, tid_t thread_id) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        auto it = semaphores_.find(semaphore_id);
        if (it != semaphores_.end()) {
            return it->second->wait(thread_id);
        }
        return false;
    }
    
    bool semaphore_try_wait() {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        // Find any semaphore and try to wait on it
        if (!semaphores_.empty()) {
            auto it = semaphores_.begin();
            return it->second->try_wait();
        }
        return false;
    }
    
    bool semaphore_post(tid_t semaphore_id) {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        auto it = semaphores_.find(semaphore_id);
        if (it != semaphores_.end()) {
            it->second->post();
            return true;
        }
        return false;
    }
    
    // Statistics and monitoring
    void print_statistics() const {
        scheduler_->print_statistics();
        
        printf("=== SYNCHRONIZATION PRIMITIVES ===\n");
        printf("Active Mutexes: %zu\n", mutexes_.size());
        printf("Active Semaphores: %zu\n", semaphores_.size());
        printf("===================================\n\n");
    }
    
    // Cleanup
    void cleanup() {
        std::lock_guard<std::mutex> lock(manager_mutex_);
        
        // Terminate all threads
        auto stats = scheduler_->get_statistics();
        
        // Clear synchronization primitives
        mutexes_.clear();
        semaphores_.clear();
    }
};

#endif // THREADING_MANAGER_H