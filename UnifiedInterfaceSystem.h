// UserlandVM-HIT Unified Interface System
// Fixes inconsistencies and provides standardized interfaces
// Author: Unified Interface System 2026-02-07

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <SupportDefs.h>

// Define missing constants if not available
#ifndef B_BAD_ADDRESS
#define B_BAD_ADDRESS B_ERROR
#endif

#ifndef B_NOT_SUPPORTED
#define B_NOT_SUPPORTED B_ERROR
#endif

// Unified error handling system
namespace UnifiedErrors {
    enum ErrorCode {
        SUCCESS = B_OK,
        INVALID_ARGUMENT = -1,
        OUT_OF_MEMORY = -2,
        IO_ERROR = -4,
        PERMISSION_DENIED = -5,
        DOES_NOT_EXIST = -6,
        ALREADY_EXISTS = -7,
        OPERATION_FAILED = -8,
        NOT_SUPPORTED = -9,
        INVALID_ADDRESS = -10,
        HALTED = 1,
        TIMEOUT = B_TIMED_OUT
    };
    
    // Standardized error handling functions
    inline const char* get_error_string(ErrorCode error) {
        switch (error) {
            case SUCCESS: return "Success";
            case INVALID_ARGUMENT: return "Invalid argument";
            case OUT_OF_MEMORY: return "Out of memory";
            case IO_ERROR: return "I/O error";
            case PERMISSION_DENIED: return "Permission denied";
            case DOES_NOT_EXIST: return "Does not exist";
            case ALREADY_EXISTS: return "Already exists";
            case OPERATION_FAILED: return "Operation failed";
            case NOT_SUPPORTED: return "Not supported";
            case INVALID_ADDRESS: return "Invalid address";
            case HALTED: return "Execution halted";
            case TIMEOUT: return "Operation timeout";
            default: return "Unknown error";
        }
    }
    
    inline void log_error(ErrorCode error, const char* operation, const char* details = nullptr) {
        printf("[ERROR] %s failed: %s", operation, get_error_string(error));
        if (details) {
            printf(" (%s)", details);
        }
        printf("\n");
    }
    
    inline void log_success(const char* operation, const char* details = nullptr) {
        printf("[SUCCESS] %s", operation);
        if (details) {
            printf(": %s", details);
        }
        printf("\n");
    }
}

// Unified memory management interface
namespace UnifiedMemory {
    // Standardized memory types
    enum MemoryType {
        GUEST_MEMORY,
        HOST_MEMORY,
        SHARED_MEMORY,
        DEVICE_MEMORY,
        CACHE_MEMORY
    };
    
    // Memory protection flags
    enum ProtectionFlags {
        READ = 0x1,
        WRITE = 0x2,
        EXECUTE = 0x4,
        READ_WRITE = READ | WRITE,
        READ_EXECUTE = READ | EXECUTE,
        READ_WRITE_EXECUTE = READ | WRITE | EXECUTE
    };
    
    // Unified memory interface
    class IMemoryManager {
    public:
        virtual ~IMemoryManager() = default;
        
        virtual UnifiedErrors::ErrorCode allocate(
            size_t size, MemoryType type, ProtectionFlags prot, uint64_t& address) = 0;
        
        virtual UnifiedErrors::ErrorCode deallocate(uint64_t address) = 0;
        
        virtual UnifiedErrors::ErrorCode protect(
            uint64_t address, size_t size, ProtectionFlags prot) = 0;
        
        virtual void* map_to_host(uint64_t guest_address, size_t size) = 0;
        
        virtual UnifiedErrors::ErrorCode unmap_from_host(
            uint64_t guest_address, void* host_ptr, size_t size) = 0;
        
        virtual bool is_valid_address(uint64_t address) const = 0;
        
        virtual ProtectionFlags get_protection(uint64_t address) const = 0;
        
        virtual void print_memory_map() const = 0;
    };
}

// Unified register interface
namespace UnifiedRegisters {
    // Standardized register types
    enum RegisterType {
        GENERAL_PURPOSE,
        SPECIAL_PURPOSE,
        FLOATING_POINT,
        VECTOR,
        CONTROL_STATUS
    };
    
    // Register access interface
    class IRegisterAccess {
    public:
        virtual ~IRegisterAccess() = default;
        
        virtual UnifiedErrors::ErrorCode get_register(
            const char* name, uint64_t& value) const = 0;
        
        virtual UnifiedErrors::ErrorCode set_register(
            const char* name, uint64_t value) = 0;
        
        virtual UnifiedErrors::ErrorCode get_register_by_id(
            int id, RegisterType type, uint64_t& value) const = 0;
        
        virtual UnifiedErrors::ErrorCode set_register_by_id(
            int id, RegisterType type, uint64_t value) = 0;
        
        virtual const char* get_register_name(int id, RegisterType type) const = 0;
        
        virtual int get_register_count(RegisterType type) const = 0;
        
        virtual void print_registers(RegisterType type = GENERAL_PURPOSE) const = 0;
    };
}

// Unified instruction interface
namespace UnifiedInstructions {
    // Instruction formats
    enum InstructionFormat {
        UNKNOWN_FORMAT = 0,
        R_TYPE,      // Register-register
        I_TYPE,      // Register-immediate
        S_TYPE,      // Store
        B_TYPE,      // Branch
        U_TYPE,      // Upper immediate
        J_TYPE       // Jump
    };
    
    // Instruction categories
    enum InstructionCategory {
        UNKNOWN_CATEGORY = 0,
        ALU,
        LOAD_STORE,
        BRANCH_JUMP,
        SYSTEM,
        FLOATING_POINT,
        VECTOR,
        CRYPTO
    };
    
    // Base instruction interface
    class IInstruction {
    public:
        virtual ~IInstruction() = default;
        
        virtual InstructionFormat get_format() const = 0;
        virtual InstructionCategory get_category() const = 0;
        virtual uint32_t get_opcode() const = 0;
        virtual uint32_t get_size() const = 0;
        virtual uint64_t get_address() const = 0;
        
        virtual bool is_branch() const = 0;
        virtual bool is_jump() const = 0;
        virtual bool is_load() const = 0;
        virtual bool is_store() const = 0;
        virtual bool is_system() const = 0;
        virtual bool changes_pc() const = 0;
        
        virtual uint64_t get_target_address() const = 0;
        virtual bool is_target_cached() const = 0;
        
        virtual void print() const = 0;
    };
    
    // Instruction decoder interface
    class IInstructionDecoder {
    public:
        virtual ~IInstructionDecoder() = default;
        
        virtual const IInstruction* decode(uint64_t address, const uint8_t* code) = 0;
        
        virtual void invalidate_cache(uint64_t address) = 0;
        
        virtual void flush_cache() = 0;
        
        virtual void print_cache_stats() const = 0;
    };
}

// Unified execution engine interface
namespace UnifiedExecution {
    // Execution states
    enum ExecutionState {
        STOPPED,
        RUNNING,
        PAUSED,
        HALTED,
        ERROR
    };
    
    // Execution statistics
    struct ExecutionStats {
        uint64_t instructions_executed;
        uint64_t execution_time_ns;
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t memory_reads;
        uint64_t memory_writes;
        uint64_t system_calls;
        uint64_t exceptions;
        
        double get_instructions_per_second() const {
            return execution_time_ns > 0 ? 
                (double)instructions_executed * 1000000000.0 / execution_time_ns : 0.0;
        }
        
        double get_cache_hit_rate() const {
            uint64_t total = cache_hits + cache_misses;
            return total > 0 ? (double)cache_hits / total : 0.0;
        }
    };
    
    // Execution engine interface
    class IExecutionEngine : public UnifiedRegisters::IRegisterAccess {
    public:
        virtual ~IExecutionEngine() = default;
        
        virtual UnifiedErrors::ErrorCode initialize(
            UnifiedMemory::IMemoryManager* memory_manager) = 0;
        
        virtual UnifiedErrors::ErrorCode execute(
            uint64_t entry_point, uint64_t stack_pointer) = 0;
        
        virtual UnifiedErrors::ErrorCode step() = 0;
        
        virtual UnifiedErrors::ErrorCode continue_execution() = 0;
        
        virtual UnifiedErrors::ErrorCode pause() = 0;
        
        virtual UnifiedErrors::ErrorCode stop() = 0;
        
        virtual UnifiedErrors::ErrorCode halt() = 0;
        
        virtual ExecutionState get_state() const = 0;
        
        virtual uint64_t get_program_counter() const = 0;
        
        virtual UnifiedErrors::ErrorCode set_program_counter(uint64_t pc) = 0;
        
        virtual const ExecutionStats& get_statistics() const = 0;
        
        virtual void reset_statistics() = 0;
        
        virtual void print_statistics() const = 0;
    };
}

// Unified symbol resolution interface
namespace UnifiedSymbols {
    // Symbol types
    enum SymbolType {
        UNKNOWN_SYMBOL = 0,
        FUNCTION,
        VARIABLE,
        OBJECT,
        SECTION,
        THREAD_LOCAL
    };
    
    // Symbol binding
    enum SymbolBinding {
        UNKNOWN_BINDING = 0,
        LOCAL,
        GLOBAL,
        WEAK
    };
    
    // Symbol information
    struct SymbolInfo {
        const char* name;
        uint64_t address;
        uint64_t size;
        SymbolType type;
        SymbolBinding binding;
        bool is_resolved;
        
        SymbolInfo() : name(nullptr), address(0), size(0), 
                     type(UNKNOWN_SYMBOL), binding(UNKNOWN_BINDING), 
                     is_resolved(false) {}
    };
    
    // Symbol resolver interface
    class ISymbolResolver {
    public:
        virtual ~ISymbolResolver() = default;
        
        virtual UnifiedErrors::ErrorCode add_symbol(
            const char* name, uint64_t address, uint64_t size,
            SymbolType type, SymbolBinding binding) = 0;
        
        virtual UnifiedErrors::ErrorCode resolve_symbol(
            const char* name, SymbolInfo& info) = 0;
        
        virtual UnifiedErrors::ErrorCode remove_symbol(const char* name) = 0;
        
        virtual UnifiedErrors::ErrorCode get_symbol_by_address(
            uint64_t address, SymbolInfo& info) = 0;
        
        virtual bool has_symbol(const char* name) const = 0;
        
        virtual size_t get_symbol_count() const = 0;
        
        virtual void print_symbols() const = 0;
        
        virtual void print_statistics() const = 0;
    };
}

// Unified system call interface
namespace UnifiedSyscalls {
    // System call interface
    class ISyscallHandler {
    public:
        virtual ~ISyscallHandler() = default;
        
        virtual UnifiedErrors::ErrorCode handle_syscall(
            uint64_t number, const uint64_t* args, size_t arg_count, uint64_t& result) = 0;
        
        virtual const char* get_syscall_name(uint64_t number) const = 0;
        
        virtual bool is_syscall_supported(uint64_t number) const = 0;
        
        virtual void print_syscall_stats() const = 0;
    };
}

// Unified context structure
struct UnifiedContext {
    uint64_t program_counter;
    uint64_t stack_pointer;
    uint64_t frame_pointer;
    uint64_t general_purpose_regs[32];  // Architecture-independent (x86-64: 16, RISC-V: 32)
    uint64_t special_regs[8];
    uint64_t flags;
    ExecutionState state;
    
    UnifiedContext() {
        memset(this, 0, sizeof(*this));
        state = UnifiedExecution::STOPPED;
    }
};

// Unified architecture factory
namespace UnifiedArchitecture {
    // Architectures
    enum Architecture {
        UNKNOWN_ARCH = 0,
        X86_32,
        X86_64,
        RISCV_32,
        RISCV_64,
        ARM_32,
        ARM_64
    };
    
    // Factory interface
    class IArchitectureFactory {
    public:
        virtual ~IArchitectureFactory() = default;
        
        virtual UnifiedErrors::ErrorCode create_execution_engine(
            Architecture arch, UnifiedMemory::IMemoryManager* memory,
            UnifiedExecution::IExecutionEngine*& engine) = 0;
        
        virtual UnifiedErrors::ErrorCode create_instruction_decoder(
            Architecture arch, UnifiedInstructions::IInstructionDecoder*& decoder) = 0;
        
        virtual UnifiedErrors::ErrorCode create_syscall_handler(
            Architecture arch, UnifiedSyscalls::ISyscallHandler*& handler) = 0;
        
        virtual Architecture detect_architecture(const uint8_t* code, size_t size) = 0;
        
        virtual const char* get_architecture_name(Architecture arch) const = 0;
    };
}

// Utility functions for standardization
namespace UnifiedUtils {
    // Standardized naming conventions
    inline const char* standardize_register_name(const char* name) {
        // Convert various naming conventions to standard format
        if (!name) return "unknown";
        
        // Handle common variations
        if (strcmp(name, "rip") == 0 || strcmp(name, "pc") == 0) return "pc";
        if (strcmp(name, "rsp") == 0 || strcmp(name, "sp") == 0) return "sp";
        if (strcmp(name, "rbp") == 0 || strcmp(name, "fp") == 0) return "fp";
        
        return name;
    }
    
    // Standardized error checking
    inline bool is_success(UnifiedErrors::ErrorCode error) {
        return error == UnifiedErrors::SUCCESS;
    }
    
    inline bool is_failure(UnifiedErrors::ErrorCode error) {
        return error != UnifiedErrors::SUCCESS;
    }
    
    // Performance timing utilities
    inline uint64_t get_timestamp_ns() {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
    }
    
    inline uint64_t get_timestamp_us() {
        return get_timestamp_ns() / 1000;
    }
    
    inline uint64_t get_timestamp_ms() {
        return get_timestamp_ns() / 1000000;
    }
    
    // Memory alignment utilities
    inline bool is_aligned(uint64_t address, size_t alignment) {
        return (address & (alignment - 1)) == 0;
    }
    
    inline uint64_t align_up(uint64_t address, size_t alignment) {
        return (address + alignment - 1) & ~(alignment - 1);
    }
    
    inline uint64_t align_down(uint64_t address, size_t alignment) {
        return address & ~(alignment - 1);
    }
}

// Global unified interface manager
class UnifiedInterfaceManager {
private:
    static UnifiedInterfaceManager* instance;
    
    UnifiedArchitecture::IArchitectureFactory* architecture_factory;
    UnifiedMemory::IMemoryManager* memory_manager;
    UnifiedSymbols::ISymbolResolver* symbol_resolver;
    UnifiedSyscalls::ISyscallHandler* syscall_handler;
    
public:
    static UnifiedInterfaceManager& get_instance() {
        if (!instance) {
            instance = new UnifiedInterfaceManager();
        }
        return *instance;
    }
    
    UnifiedInterfaceManager() : architecture_factory(nullptr), memory_manager(nullptr),
                              symbol_resolver(nullptr), syscall_handler(nullptr) {
        printf("[UNIFIED] Unified interface manager initialized\n");
    }
    
    ~UnifiedInterfaceManager() {
        delete architecture_factory;
        delete memory_manager;
        delete symbol_resolver;
        delete syscall_handler;
    }
    
    void set_architecture_factory(UnifiedArchitecture::IArchitectureFactory* factory) {
        architecture_factory = factory;
    }
    
    void set_memory_manager(UnifiedMemory::IMemoryManager* manager) {
        memory_manager = manager;
    }
    
    void set_symbol_resolver(UnifiedSymbols::ISymbolResolver* resolver) {
        symbol_resolver = resolver;
    }
    
    void set_syscall_handler(UnifiedSyscalls::ISyscallHandler* handler) {
        syscall_handler = handler;
    }
    
    UnifiedArchitecture::IArchitectureFactory* get_architecture_factory() const {
        return architecture_factory;
    }
    
    UnifiedMemory::IMemoryManager* get_memory_manager() const {
        return memory_manager;
    }
    
    UnifiedSymbols::ISymbolResolver* get_symbol_resolver() const {
        return symbol_resolver;
    }
    
    UnifiedSyscalls::ISyscallHandler* get_syscall_handler() const {
        return syscall_handler;
    }
    
    void print_status() const {
        printf("\n=== UNIFIED INTERFACE STATUS ===\n");
        printf("Architecture Factory: %s\n", architecture_factory ? "OK" : "NULL");
        printf("Memory Manager: %s\n", memory_manager ? "OK" : "NULL");
        printf("Symbol Resolver: %s\n", symbol_resolver ? "OK" : "NULL");
        printf("Syscall Handler: %s\n", syscall_handler ? "OK" : "NULL");
        printf("===============================\n\n");
    }
};

// Static instance
UnifiedInterfaceManager* UnifiedInterfaceManager::instance = nullptr;

// Convenience macros for unified interfaces
#define UNIFIED_ERROR(code, operation, details) \
    UnifiedErrors::log_error(code, operation, details)

#define UNIFIED_SUCCESS(operation, details) \
    UnifiedErrors::log_success(operation, details)

#define UNIFIED_CHECK(call, operation) do { \
    UnifiedErrors::ErrorCode result = call; \
    if (UnifiedUtils::is_failure(result)) { \
        UNIFIED_ERROR(result, operation); \
        return result; \
    } \
} while(0)

#define UNIFIED_GET_TIME() UnifiedUtils::get_timestamp_ns()
#define UNIFIED_ALIGN(addr, align) UnifiedUtils::align_up(addr, align)
#define UNIFIED_IS_ALIGNED(addr, align) UnifiedUtils::is_aligned(addr, align)