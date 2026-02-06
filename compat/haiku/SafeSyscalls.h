/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * SafeSyscalls.h - Secure syscall implementation with comprehensive validation
 */

#ifndef SAFE_SYSCALLS_H
#define SAFE_SYSCALLS_H

#include <SupportDefs.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <errno.h>
#include <unistd.h>

// Security policy for syscall execution
class SyscallSecurityPolicy {
public:
    static constexpr uint32_t MAX_STRING_LENGTH = 4096;
    static constexpr uint32_t MAX_PATH_LENGTH = 1024;
    static constexpr uint32_t MAX_SYSCALL_DEPTH = 8;
    
    // Operation safety limits
    static constexpr uint32_t MAX_SAFE_OPERATIONS = 100;
    static constexpr uint32_t MAX_MEMORY_ALLOCATIONS = 50;
    
    // Safety states
    enum SecurityState {
        SECURE,
        WARNING,
        ERROR,
        TERMINATED
    };
    
    static bool IsOperationSafe(const char* operation, SecurityState& state, uint32_t operation_count);
    static void RecordUnsafeOperation(const char* operation, const char* details);
    static void TerminateOnError(const char* error);
    static void ResetOperationCount(const char* operation);
};

// Input validation with bounds checking
template<typename T>
class SecureInput {
public:
    static status_t ValidateRange(const T& value, const T& min_val, const T& max_val, const char* name);
    static status_t ValidatePointer(const T* ptr, const char* context, bool allow_null = false);
    static status_t ValidateString(const char* str, size_t max_len, const char* context);
    static status_t ValidateBuffer(const void* buffer, size_t size, size_t max_len, const char* context);
    static status_t ValidateArraySize(uint32_t count, uint32_t max_count, const char* context);
    static void SanitizeString(char* str, size_t len);
};

// Memory allocation with bounds checking
template<typename T>
class SecureAllocator {
public:
    static T* Allocate(size_t count);
    static T* Reallocate(T* ptr, size_t new_size);
    static void Free(T* ptr);
    static bool IsValidPointer(const T* ptr, const void* base, size_t size);
    static void MemoryStats();
    
    static void ResetStats();
    
private:
    static uint32_t total_allocations;
    static uint32_t current_allocations;
    static uint32_t max_concurrent_allocations;
    static size_t total_bytes_allocated;
};

// Syscall execution tracking
class SyscallTracker {
public:
    static void Initialize();
    static void RecordSyscall(uint32_t syscall_num);
    static void RecordSecurityViolation(uint32_t syscall_num, const char* details);
    static void PrintStatistics();
    
private:
    static uint32_t syscall_counts[256];
    static uint32_t security_violations;
    static uint64_t start_time;
    static bool initialized;
};

// Safe syscall dispatcher with comprehensive security
class SafeSyscallDispatcher {
public:
    SafeSyscallDispatcher();
    ~SafeSyscallDispatcher();
    
    // Main secure syscall interface
    static status_t ExecuteSyscall(uint32_t syscall_num, uint32_t* args, uint32_t arg_count);
    
private:
    static status_t ValidateSyscall(uint32_t syscall_num, uint32_t arg_count);
    static const char* GetSyscallName(uint32_t num);
    static SecurityState GetSyscallSecurityLevel(uint32_t syscall_num);
    static void LogSyscall(uint32_t syscall_num, uint64_t execution_time_us);
    static void UpdateExecutionTime();
    
private:
    static const char* GetSecurityMessage(uint32_t error_code);
};

// Defensive programming utilities
class DefensiveUtils {
public:
    static bool SafeMemcpy(void* dest, const void* src, size_t size);
    static bool SafeStringCopy(char* dest, size_t dest_size, const char* src);
    static bool SafePointerCast(const void* ptr, const char* operation);
    static void SecureZeroMemory(void* ptr, size_t size);
    static bool IsValidMemoryRange(const void* ptr, size_t size, const void* base, size_t region_size);
};

#endif // SAFE_SYSCALLS_H