/*
 * LibcStubs.h - Basic C Library Stubs for Guest Programs
 * 
 * Provides essential libc function implementations for programs running in the VM
 * that need basic string, memory, and I/O functions
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Basic libc function wrappers and stubs
class LibcStubs {
public:
    // Initialize libc stubs system
    static bool Initialize();
    static void Shutdown();
    
    // Memory functions - these could be wrapped to use GuestHeap
    static void* malloc(size_t size);
    static void* calloc(size_t count, size_t size);
    static void* realloc(void* ptr, size_t size);
    static void free(void* ptr);
    
    // String functions
    static size_t strlen(const char* str);
    static char* strcpy(char* dest, const char* src);
    static char* strncpy(char* dest, const char* src, size_t n);
    static int strcmp(const char* str1, const char* str2);
    static int strncmp(const char* str1, const char* str2, size_t n);
    static char* strcat(char* dest, const char* src);
    static char* strchr(const char* str, int c);
    static char* strstr(const char* haystack, const char* needle);
    
    // Memory operations
    static void* memcpy(void* dest, const void* src, size_t n);
    static void* memmove(void* dest, const void* src, size_t n);
    static void* memset(void* s, int c, size_t n);
    static int memcmp(const void* s1, const void* s2, size_t n);
    
    // I/O functions
    static int printf(const char* format, ...);
    static int fprintf(FILE* stream, const char* format, ...);
    // sprintf removed due to const qualifier issues
    static int snprintf(char* str, size_t size, const char* format, ...);
    static int puts(const char* str);
    static int putchar(int c);
    static int getchar(void);
    
    // Math functions (basic)
    static int abs(int x);
    static long labs(long x);
    static double fabs(double x);
    
private:
    static bool initialized;
};