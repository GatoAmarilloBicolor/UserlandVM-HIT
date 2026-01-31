/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * DebugOutput.cpp - Implementation of debug output system
 */

#include "DebugOutput.h"
#include <stdlib.h>
#include <string.h>

// Global debug output configuration
FILE* g_debug_output = NULL;
bool g_debug_enabled = false;
static FILE* g_debug_file = NULL;

void DebugOutput_Init(bool enable_debug, const char* debug_file)
{
    g_debug_enabled = enable_debug;
    
    if (!enable_debug) {
        g_debug_output = NULL;
        return;
    }
    
    // If debug_file is provided, open it
    if (debug_file != NULL) {
        g_debug_file = fopen(debug_file, "w");
        if (g_debug_file != NULL) {
            g_debug_output = g_debug_file;
            fprintf(g_debug_output, "=== UserlandVM Debug Trace ===\n");
            fprintf(g_debug_output, "Debug output started\n\n");
            fflush(g_debug_output);
        } else {
            // Fall back to stderr if file open fails
            g_debug_output = stderr;
            fprintf(stderr, "Warning: Could not open debug file '%s', using stderr\n", debug_file);
        }
    } else {
        // Default to stderr
        g_debug_output = stderr;
    }
}

void DebugPrintf(const char* format, ...)
{
    if (!g_debug_enabled || g_debug_output == NULL) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(g_debug_output, format, args);
    va_end(args);
}

void DebugVPrintf(const char* format, va_list args)
{
    if (!g_debug_enabled || g_debug_output == NULL) {
        return;
    }
    
    vfprintf(g_debug_output, format, args);
}

void DebugFlush(void)
{
    if (g_debug_output != NULL) {
        fflush(g_debug_output);
    }
}

void DebugOutput_Cleanup(void)
{
    if (g_debug_file != NULL && g_debug_file != stdout && g_debug_file != stderr) {
        fprintf(g_debug_file, "\n=== Debug trace ended ===\n");
        fflush(g_debug_file);
        fclose(g_debug_file);
        g_debug_file = NULL;
    }
    g_debug_output = NULL;
    g_debug_enabled = false;
}
