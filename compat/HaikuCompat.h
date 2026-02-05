/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * HaikuCompat.h - Compatibility layer for building UserlandVM on Linux
 * This file provides the necessary types and definitions that are normally
 * provided by the Haiku OS headers.
 */
#ifndef _HAIKU_COMPAT_H
#define _HAIKU_COMPAT_H

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

/*
 * When building on Linux, we need to provide Haiku-compatible types.
 * We define the __haiku_* types that SupportDefs.h depends on.
 */

#ifndef __HAIKU__

/* Fixed-width integer types for Haiku compatibility */
typedef int8_t __haiku_std_int8;
typedef uint8_t __haiku_std_uint8;
typedef int16_t __haiku_std_int16;
typedef uint16_t __haiku_std_uint16;
typedef int32_t __haiku_std_int32;
typedef uint32_t __haiku_std_uint32;
typedef int64_t __haiku_std_int64;
typedef uint64_t __haiku_std_uint64;

typedef __haiku_std_int8 __haiku_int8;
typedef __haiku_std_uint8 __haiku_uint8;
typedef __haiku_std_int16 __haiku_int16;
typedef __haiku_std_uint16 __haiku_uint16;
typedef __haiku_std_int32 __haiku_int32;
typedef __haiku_std_uint32 __haiku_uint32;
typedef __haiku_std_int64 __haiku_int64;
typedef __haiku_std_uint64 __haiku_uint64;

/* Haiku-style short type aliases */
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

/* Address types */
typedef intptr_t __haiku_saddr_t;
typedef uintptr_t __haiku_addr_t;
typedef int64_t __haiku_phys_saddr_t;
typedef uint64_t __haiku_phys_addr_t;
typedef uintptr_t __haiku_generic_addr_t;
typedef uintptr_t phys_addr_t;
typedef size_t phys_size_t;
typedef uintptr_t generic_addr_t;
typedef size_t generic_size_t;
typedef uintptr_t addr_t;

/* Status and time types */
typedef int32_t status_t;
typedef int64_t bigtime_t;
typedef int64_t nanotime_t;
typedef uint32_t type_code;
typedef uint32_t perform_code;
typedef int32_t area_id;
typedef int32_t team_id;
typedef int32_t thread_id;
typedef int32_t port_id;
typedef int32_t sem_id;
typedef int32_t image_id;

/* printf format macros compatible with Haiku */
#define __HAIKU_PRI_PREFIX_32 ""
#define __HAIKU_PRI_PREFIX_64 "ll"
#define __HAIKU_PRI_PREFIX_ADDR "l"
#define __HAIKU_PRI_PREFIX_PHYS_ADDR __HAIKU_PRI_PREFIX_64
#define __HAIKU_PRI_PREFIX_GENERIC_ADDR __HAIKU_PRI_PREFIX_ADDR

#define B_PRId8 "d"
#define B_PRIi8 "i"
#define B_PRId16 "d"
#define B_PRIi16 "i"
#define B_PRId32 "d"
#define B_PRIi32 "i"
#define B_PRId64 "lld"
#define B_PRIi64 "lli"
#define B_PRIu8 "u"
#define B_PRIo8 "o"
#define B_PRIx8 "x"
#define B_PRIX8 "X"
#define B_PRIu16 "u"
#define B_PRIo16 "o"
#define B_PRIx16 "x"
#define B_PRIX16 "X"
#define B_PRIu32 "u"
#define B_PRIo32 "o"
#define B_PRIx32 "x"
#define B_PRIX32 "X"
#define B_PRIu64 "llu"
#define B_PRIo64 "llo"
#define B_PRIx64 "llx"
#define B_PRIX64 "llX"

/* Architecture detection */
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    defined(__riscv) && __riscv_xlen == 64
#define __HAIKU_ARCH_BITS 64
#define __HAIKU_ARCH_64_BIT 1
#define B_HAIKU_64_BIT 1
#else
#define __HAIKU_ARCH_BITS 32
#define __HAIKU_ARCH_32_BIT 1
#define B_HAIKU_32_BIT 1
#endif

#define __HAIKU_ARCH_PHYSICAL_BITS 64
#define __HAIKU_ARCH_PHYSICAL_64_BIT 1

/* Haiku error codes (map to POSIX equivalents where possible) */
#define B_OK 0
#define B_ERROR (-1)
#define B_NO_ERROR 0
#define B_NO_MEMORY ENOMEM
#define B_BAD_VALUE EINVAL
#define B_BAD_ADDRESS EFAULT
#define B_NOT_ALLOWED EPERM
#define B_PERMISSION_DENIED EACCES
#define B_NAME_NOT_FOUND ENOENT
#define B_NO_MORE_AREAS ENOMEM
#define B_ENTRY_NOT_FOUND ENOENT
#define B_BAD_INDEX ERANGE
#define B_TIMED_OUT ETIMEDOUT
#define B_INTERRUPTED EINTR
#define B_BAD_TEAM_ID ESRCH

/* Area creation constants */
#define B_ANY_ADDRESS 0
#define B_EXACT_ADDRESS 1
#define B_BASE_ADDRESS 2
#define B_CLONE_ADDRESS 3
#define B_ANY_KERNEL_ADDRESS 4

/* Memory locking */
#define B_NO_LOCK 0
#define B_LAZY_LOCK 1
#define B_FULL_LOCK 2
#define B_CONTIGUOUS 3

/* Area protection flags */
#define B_READ_AREA 0x01
#define B_WRITE_AREA 0x02
#define B_EXECUTE_AREA 0x04
#define B_STACK_AREA 0x08

/* Image types */
#define B_APP_IMAGE 1
#define B_LIBRARY_IMAGE 2
#define B_ADD_ON_IMAGE 3
#define B_SYSTEM_IMAGE 4

/* Haiku-style function attributes */
#define _EXPORT __attribute__((visibility("default")))
#define _PACKED __attribute__((packed))

#endif /* !__HAIKU__ */

#endif /* _HAIKU_COMPAT_H */
