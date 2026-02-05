/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * UserlandVM-HIT - Platform-Independent Headers
 * Avoids Haiku header conflicts while providing necessary types
 */

#ifndef _PLATFORM_INDEPENDENT_H
#define _PLATFORM_INDEPENDENT_H

// Standard types (avoids Haiku header conflicts)
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Status codes (compatible with Linux but portable)
typedef int32_t platform_status_t;
#define PLATFORM_OK        0
#define PLATFORM_ERROR    (-1)
#define PLATFORM_NO_INIT (-2)
#define PLATFORM_NO_MEMORY (-3)
#define PLATFORM_BAD_VALUE (-4)
#define PLATFORM_BAD_ADDRESS (-5)

// Platform types
typedef enum {
    PLATFORM_X86_64,
    PLATFORM_X86_32,
    PLATFORM_ARM64,
    PLATFORM_ARM32,
    PLATFORM_RISCV64,
    PLATFORM_RISCV32,
    PLATFORM_UNKNOWN
} platform_type_t;

// Platform information structure
typedef struct {
    platform_type_t type;
    char name[32];
    char arch[16];
    bool is_64bit;
    char vendor[64];
    char model[64];
    uint32_t flags;
    uint32_t features;
} platform_info_t;

// Platform detection functions
platform_info_t get_platform_info();
const char* get_platform_name(platform_type_t type);
bool is_64bit_platform();
bool supports_native_execution(platform_type_t host, platform_type_t target);

// Compatibility checking
platform_status_t check_compatibility(platform_type_t host, platform_type_t target);
const char* get_compatibility_message(platform_type_t host, platform_type_t target);

// Error handling
void set_platform_error(const char* function, const char* error);
void clear_platform_error();

// Logging (can be overriden)
extern void platform_log(const char* level, const char* format, ...);
extern void platform_log_platform(platform_info_t* info);
extern void platform_log_compatibility(platform_type_t host, platform_type_t target);

// Memory management
void* platform_malloc(size_t size);
void platform_free(void* ptr);
void* platform_calloc(size_t count, size_t size);
void* platform_realloc(void* ptr, size_t size);

// Threading
typedef void* platform_thread_t;
typedef void* (*thread_func)(void*);

platform_status_t platform_create_thread(thread_func func, void* data, platform_thread_t* thread);
platform_status_t platform_join_thread(platform_thread_t thread);
platform_status_t platform_detach_thread(thread_func func, void* data);
void platform_exit_thread(platform_status_t exit_code);

// Performance
uint64_t platform_get_time_microseconds();
void platform_sleep_microseconds(uint64_t useconds);

#endif // _PLATFORM_INDEPENDENT_H