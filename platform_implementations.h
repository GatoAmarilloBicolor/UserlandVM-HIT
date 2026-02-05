/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 * 
 * Platform-Independent Function Implementations
 * Provides stub implementations for platform-specific functions
 */

#ifndef _PLATFORM_IMPLEMENTATIONS_H
#define _PLATFORM_IMPLEMENTATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Function implementations that don't conflict with Haiku headers
void printf(const char* format, ...) {
    printf(format, ##__VA_ARGS__);
}

void* malloc(size_t size) {
    return malloc(size);
}

void free(void* ptr) {
    free(ptr);
}

void* calloc(size_t count, size_t size) {
    return calloc(count, size);
}

// Platform function wrappers
void platform_log(const char* level, const char* format, ...) {
    // Use colorized logging if available, otherwise basic logging
    #ifdef HAVE_COLORED_OUTPUT
    #include "colored_output.h"
        // Forward to logging system
        if (strcmp(level, "debug") == 0) {
            haiku_debug(format, ##__VA_ARGS__);
        } else if (strcmp(level, "info") == 0) {
            haiku_info(format, ##__VA_ARGS__);
        } else if (strcmp(level, "success") == 0) {
            haiku_success(format, ##__VA_ARGS__);
        } else if (strcmp(level, "warning") == 0) {
            haiku_warning(format, ##__VA_ARGS__);
        } else if (strcmp(level, "error") == 0) {
            haiku_error(format, ##__VA_ARGS__);
        } else {
            printf("[%s] %s", level, format);
        }
    #else
        printf("[%s] %s", level, format);
    #endif
}

void* platform_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

// Logging macros for Haiku-style output
#define PLATFORM_DEBUG(...)   platform_log("debug", __VA_ARGS__)
#define PLATFORM_INFO(...)    platform_log("info", __VA_ARGS__)
#define PLATFORM_SUCCESS(...)  platform_log("success", __VA_ARGS__)
#define PLATFORM_WARNING(...) platform_log("warning", __VA_ARGS__)
#define PLATFORM_ERROR(...)   platform_log("error", __VA_ARGS__)

#endif // _PLATFORM_IMPLEMENTATIONS_H