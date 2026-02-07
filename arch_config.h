#ifndef _ARCH_CONFIG_H
#define _ARCH_CONFIG_H

#include <stdint.h>
#include <stddef.h>

// Haiku architecture configuration for UserlandVM-HIT
// Provides essential architecture-specific definitions

// Architecture detection
#ifdef __i386__
#define HAIKU_ARCH_X86 1
#define HAIKU_ARCH_X86_32 1
#elif defined(__x86_64__)
#define HAIKU_ARCH_X86 1
#define HAIKU_ARCH_X86_64 1
#elif defined(__arm__)
#define HAIKU_ARCH_ARM 1
#elif defined(__aarch64__)
#define HAIKU_ARCH_ARM 1
#define HAIKU_ARCH_ARM64 1
#else
#define HAIKU_ARCH_UNKNOWN 1
#endif

// Common architecture definitions
#define HAIKU_ADDRESS_WIDTH (sizeof(void*) * 8)

// Page size (standard 4KB)
#define B_PAGE_SIZE 4096

// Address types
typedef uintptr_t addr_t;
typedef uintptr_t phys_addr_t;
typedef size_t vm_size_t;
typedef addr_t vm_addr_t;

// Architecture-specific constants
#if HAIKU_ARCH_X86_32
#define HAIKU_USER_BASE 0x01000000
#define HAIKU_KERNEL_BASE 0x80000000
#define HAIKU_USER_STACK_TOP (HAIKU_KERNEL_BASE - B_PAGE_SIZE)
#elif HAIKU_ARCH_X86_64
#define HAIKU_USER_BASE 0x0000000001000000ULL
#define HAIKU_KERNEL_BASE 0xFFFF800000000000ULL
#define HAIKU_USER_STACK_TOP (HAIKU_KERNEL_BASE - B_PAGE_SIZE)
#endif

// System call conventions
#if HAIKU_ARCH_X86_32
#define HAIKU_SYSCALL_INT 0x99
#define HAIKU_SYSCALL_REG_EAX 0
#define HAIKU_SYSCALL_REG_EBX 1
#define HAIKU_SYSCALL_REG_ECX 2
#define HAIKU_SYSCALL_REG_EDX 3
#elif HAIKU_ARCH_X86_64
#define HAIKU_SYSCALL_INT 0x99
#define HAIKU_SYSCALL_REG_RAX 0
#define HAIKU_SYSCALL_REG_RDI 1
#define HAIKU_SYSCALL_REG_RSI 2
#define HAIKU_SYSCALL_REG_RDX 3
#endif

#endif /* _ARCH_CONFIG_H */