/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under terms of MIT License.
 * 
 * SafeSyscallArguments.h - Secure argument handling for syscalls
 */

#ifndef SAFE_SYSCALL_ARGUMENTS_H
#define SAFE_SYSCALL_ARGUMENTS_H

#include <SupportDefs.h>
#include <stdint.h>

// Maximum safe argument sizes for different syscall types
#define MAX_READ_ARGS 3      // read(fd, buf, count)
#define MAX_WRITE_ARGS 3     // write(fd, buf, count)  
#define MAX_IOCTL_ARGS 4      // ioctl(fd, request, argp)
#define MAX_EXECVE_ARGS 8     // execve(filename, argv, envp)

// Safe argument buffer with overflow protection
struct SafeArgBuffer {
    uint64_t args[MAX_IOCTL_ARGS];
    uint32_t arg_count;
    bool initialized;
    
    SafeArgBuffer() : arg_count(0), initialized(false) {}
    
    status_t Initialize() {
        if (initialized) return B_OK;
        
        arg_count = 0;
        for (uint32_t i = 0; i < MAX_IOCTL_ARGS; i++) {
            args[i] = 0;
        }
        initialized = true;
        return B_OK;
    }
    
    status_t AddArg(uint64_t arg) {
        if (arg_count >= MAX_IOCTL_ARGS) {
            printf("[SYSCALL] ERROR: Too many arguments (max: %d)\n", MAX_IOCTL_ARGS);
            return B_ERROR;
        }
        
        args[arg_count++] = arg;
        return B_OK;
    }
    
    template<typename T>
    status_t GetArg(uint32_t index, T& result) const {
        if (index >= arg_count || !initialized) {
            result = T{};
            return B_ERROR;
        }
        
        result = static_cast<T>(args[index]);
        return B_OK;
    }
    
    uint32_t GetArgCount() const { return arg_count; }
    
    void Reset() {
        arg_count = 0;
        initialized = false;
        // Clear any sensitive data
        memset(args, 0, sizeof(args));
    }
};

// Safe syscall argument reader with bounds checking
template<typename T>
class SafeArgReader {
public:
    SafeArgReader(uint64_t* args, uint32_t arg_count) 
        : fArgs(args), fArgCount(arg_count), fCurrentIndex(0) {}
    
    status_t ReadArg(uint32_t index, T& result) {
        if (index >= fArgCount) {
            printf("[SYSCALL] ERROR: Invalid argument index %d (max: %d)\n", index, fArgCount);
            result = T{};
            return B_ERROR;
        }
        
        result = static_cast<T>(fArgs[index]);
        fCurrentIndex = index + 1;
        return B_OK;
    }
    
    void Reset() { fCurrentIndex = 0; }
private:
    uint64_t* fArgs;
    uint32_t fArgCount;
    uint32_t fCurrentIndex;
};

// Syscall argument sanitization and validation
template<typename T>
static status_t SafeValidateArg(T value, const char* arg_name, bool allow_zero = false) {
    // Check for null or invalid pointers (when applicable)
    if constexpr (std::is_pointer_v<T>::value) && !allow_zero) {
        if (value == nullptr) {
            printf("[SYSCALL] ERROR: NULL pointer in argument %s\n", arg_name);
            return B_ERROR;
        }
    }
    
    return B_OK;
}

// Safe string reading with bounds checking
class SafeStringReader {
public:
    SafeStringReader(const char* buffer, size_t buffer_size, size_t* consumed = nullptr)
        : fBuffer(buffer), fBufferSize(buffer_size), fConsumed(consumed ? *consumed : 0) {}
    
    status_t ReadString(char* destination, size_t max_length) {
        if (!destination || max_length == 0) {
            return B_BAD_VALUE;
        }
        
        size_t bytes_to_read = std::min(max_length - 1, fBufferSize - fConsumed);
        
        size_t i;
        char ch;
        for (i = 0; i < bytes_to_read; i++) {
            ch = fBuffer[fConsumed + i];
            if (ch == '\0') break; // Allow null terminator
            
            destination[i] = ch;
            fConsumed++;
        }
        
        destination[i] = '\0'; // Null terminate
        if (bytes_to_read < max_length) {
            fConsumed++;
        }
        
        return B_OK;
    }
};

// Security policy for syscall execution
class SyscallSecurityPolicy {
public:
    static constexpr uint32_t MAX_SYSCALL_STACK_SIZE = 1024;
    static constexpr uint32_t DANGEROUS_OP_LIMIT = 1000;
    static constexpr uint32_t CRITICAL_OP_LIMIT = 100;
    
    static bool IsOperationSafe(const char* operation, uint32_t instruction_count);
    static bool IsSyscallSafe(uint32_t syscall_num);
    
    static void RecordUnsafeOperation(const char* details);
    static void TerminateOnCriticalError(const char* error);
};

#endif // SAFE_SYSCALL_ARGUMENTS_H