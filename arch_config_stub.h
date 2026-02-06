#ifndef ARCH_CONFIG_STUB_H
#define ARCH_CONFIG_STUB_H

///////////////////////////////////////////////////////////////////////////////
// arch_config_stub.h - Stub for missing Haiku arch_config.h
//
// This file provides minimal architecture configuration needed
// when arch_config.h is not available in the system headers.
///////////////////////////////////////////////////////////////////////////////

// Architecture detection
#if defined(__x86_64__) || defined(__amd64__)
#define B_HOST_IS_X86_64 1
#define B_HOST_IS_X86 0
#elif defined(__i386__) || defined(__x86__)
#define B_HOST_IS_X86_64 0
#define B_HOST_IS_X86 1
#elif defined(__aarch64__)
#define B_HOST_IS_ARM64 1
#define B_HOST_IS_ARM 0
#elif defined(__arm__)
#define B_HOST_IS_ARM64 0
#define B_HOST_IS_ARM 1
#elif defined(__riscv) && (__SIZEOF_POINTER__ == 8)
#define B_HOST_IS_RISCV64 1
#define B_HOST_IS_RISCV32 0
#elif defined(__riscv) && (__SIZEOF_POINTER__ == 4)
#define B_HOST_IS_RISCV64 0
#define B_HOST_IS_RISCV32 1
#else
#error "Unsupported architecture"
#endif

// Pointer size
#define B_HOST_IS_LENDIAN 1
#define B_HOST_IS_BENDIAN 0

// Cache line size (typical)
#define B_CACHE_LINE_SIZE 64

#endif // ARCH_CONFIG_STUB_H
