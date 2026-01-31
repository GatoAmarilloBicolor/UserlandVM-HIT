/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DebugOutput.h - Global debug output management
 * Allows separation of debug trace from guest program output
 */

#pragma once

#include <stdio.h>
#include <stdarg.h>

// Global debug output destination
extern FILE* g_debug_output;
extern bool g_debug_enabled;

/**
 * Initialize debug output system
 * @param enable_debug - If true, debug output is enabled
 * @param debug_file - Path to debug log file (NULL for stderr)
 */
void DebugOutput_Init(bool enable_debug, const char* debug_file);

/**
 * Write debug message with formatting
 * Same as fprintf but uses g_debug_output instead of stdout
 */
void DebugPrintf(const char* format, ...);

/**
 * Write raw debug message (vprintf style)
 */
void DebugVPrintf(const char* format, va_list args);

/**
 * Flush debug output
 */
void DebugFlush(void);

/**
 * Close debug output file
 */
void DebugOutput_Cleanup(void);

// Convenience macro that only prints when debug is enabled
#define DEBUG_PRINT(fmt, ...) \
    do { if (g_debug_enabled) DebugPrintf(fmt, ##__VA_ARGS__); } while(0)

#define DEBUG_VPRINT(fmt, args) \
    do { if (g_debug_enabled) DebugVPrintf(fmt, args); } while(0)
