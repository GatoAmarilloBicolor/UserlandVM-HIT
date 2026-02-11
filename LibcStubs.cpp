/*
 * LibcStubs.cpp - Basic C Library Stubs Implementation
 * 
 * Provides essential libc function implementations for programs running in the VM
 * Uses standard library implementations with VM-specific logging
 */

#include "LibcStubs.h"
#include "GuestHeap.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cmath>

// Global guest heap instance
extern GuestHeap* g_guest_heap;

bool LibcStubs::initialized = false;

bool LibcStubs::Initialize() {
    if (initialized) {
        return true;
    }
    
    printf("[LibcStubs] Initializing libc stubs for guest programs\n");
    initialized = true;
    
    printf("[LibcStubs] ✅ Libc stubs initialized\n");
    return true;
}

void LibcStubs::Shutdown() {
    if (!initialized) {
        return;
    }
    
    printf("[LibcStubs] Shutting down libc stubs\n");
    initialized = false;
}

// Memory functions - use guest heap if available
void* LibcStubs::malloc(size_t size) {
    if (g_guest_heap) {
        return g_guest_heap->malloc(size);
    }
    return std::malloc(size);
}

void* LibcStubs::calloc(size_t count, size_t size) {
    if (g_guest_heap) {
        return g_guest_heap->calloc(count, size);
    }
    return std::calloc(count, size);
}

void* LibcStubs::realloc(void* ptr, size_t size) {
    if (g_guest_heap) {
        return g_guest_heap->realloc(ptr, size);
    }
    return std::realloc(ptr, size);
}

void LibcStubs::free(void* ptr) {
    if (g_guest_heap) {
        g_guest_heap->free(ptr);
        return;
    }
    std::free(ptr);
}

// String functions
size_t LibcStubs::strlen(const char* str) {
    if (!str) return 0;
    return std::strlen(str);
}

char* LibcStubs::strcpy(char* dest, const char* src) {
    return std::strcpy(dest, src);
}

char* LibcStubs::strncpy(char* dest, const char* src, size_t n) {
    return std::strncpy(dest, src, n);
}

int LibcStubs::strcmp(const char* str1, const char* str2) {
    return std::strcmp(str1, str2);
}

int LibcStubs::strncmp(const char* str1, const char* str2, size_t n) {
    return std::strncmp(str1, str2, n);
}

char* LibcStubs::strcat(char* dest, const char* src) {
    return std::strcat(dest, src);
}

char* LibcStubs::strchr(const char* str, int c) {
    return std::strchr(str, c);
}

char* LibcStubs::strstr(const char* haystack, const char* needle) {
    return std::strstr(haystack, needle);
}

// Memory operations
void* LibcStubs::memcpy(void* dest, const void* src, size_t n) {
    return std::memcpy(dest, src, n);
}

void* LibcStubs::memmove(void* dest, const void* src, size_t n) {
    return std::memmove(dest, src, n);
}

void* LibcStubs::memset(void* s, int c, size_t n) {
    return std::memset(s, c, n);
}

int LibcStubs::memcmp(const void* s1, const void* s2, size_t n) {
    return std::memcmp(s1, s2, n);
}

// I/O functions
int LibcStubs::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int result = vprintf(format, args);
    
    va_end(args);
    return result;
}

int LibcStubs::fprintf(FILE* stream, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int result = vfprintf(stream, format, args);
    
    va_end(args);
    return result;
}

// sprintf removed - use snprintf instead

int LibcStubs::snprintf(char* str, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int result = vsnprintf(str, size, format, args);
    
    va_end(args);
    return result;
}

int LibcStubs::puts(const char* str) {
    return printf("%s\n", str);
}

int LibcStubs::putchar(int c) {
    return std::putchar(c);
}

int LibcStubs::getchar(void) {
    return std::getchar();
}

// Math functions
int LibcStubs::abs(int x) {
    return std::abs(x);
}

long LibcStubs::labs(long x) {
    return std::labs(x);
}

double LibcStubs::fabs(double x) {
    return std::fabs(x);
}