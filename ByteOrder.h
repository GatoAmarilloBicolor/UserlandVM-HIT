#ifndef _BYTE_ORDER_H
#define _BYTE_ORDER_H

#include <stdint.h>
#include <endian.h>
#include <stddef.h>

// Basic Haiku byte order compatibility stub
// Provides essential macros for UserlandVM-HIT

/* swap directions */
typedef enum {
	B_SWAP_HOST_TO_LENDIAN,
	B_SWAP_HOST_TO_BENDIAN,
	B_SWAP_LENDIAN_TO_HOST,
	B_SWAP_BENDIAN_TO_HOST,
	B_SWAP_ALWAYS
} swap_action;

/* Always swap macros */
#define B_SWAP_DOUBLE(arg) __builtin_bswap64(arg)
#define B_SWAP_FLOAT(arg)  __builtin_bswap32(arg)
#define B_SWAP_INT64(arg)  __builtin_bswap64(arg)
#define B_SWAP_INT32(arg)  __builtin_bswap32(arg)
#define B_SWAP_INT16(arg)  __builtin_bswap16(arg)

#if BYTE_ORDER == LITTLE_ENDIAN
/* Host is little endian */
#define B_HOST_IS_LENDIAN 1
#define B_HOST_IS_BENDIAN 0

/* Host native to little endian (no swap needed) */
#define B_HOST_TO_LENDIAN_DOUBLE(arg) (double)(arg)
#define B_HOST_TO_LENDIAN_FLOAT(arg)  (float)(arg)
#define B_HOST_TO_LENDIAN_INT64(arg)  (uint64_t)(arg)
#define B_HOST_TO_LENDIAN_INT32(arg)  (uint32_t)(arg)
#define B_HOST_TO_LENDIAN_INT16(arg)  (uint16_t)(arg)

/* Little endian to host native (no swap needed) */
#define B_LENDIAN_TO_HOST_DOUBLE(arg) (double)(arg)
#define B_LENDIAN_TO_HOST_FLOAT(arg)  (float)(arg)
#define B_LENDIAN_TO_HOST_INT64(arg)  (uint64_t)(arg)
#define B_LENDIAN_TO_HOST_INT32(arg)  (uint32_t)(arg)
#define B_LENDIAN_TO_HOST_INT16(arg)  (uint16_t)(arg)

/* Host native to big endian (swap needed) */
#define B_HOST_TO_BENDIAN_DOUBLE(arg) __builtin_bswap64(arg)
#define B_HOST_TO_BENDIAN_FLOAT(arg)  __builtin_bswap32(arg)
#define B_HOST_TO_BENDIAN_INT64(arg)  __builtin_bswap64(arg)
#define B_HOST_TO_BENDIAN_INT32(arg)  __builtin_bswap32(arg)
#define B_HOST_TO_BENDIAN_INT16(arg)  __builtin_bswap16(arg)

/* Big endian to host native (swap needed) */
#define B_BENDIAN_TO_HOST_DOUBLE(arg) __builtin_bswap64(arg)
#define B_BENDIAN_TO_HOST_FLOAT(arg)  __builtin_bswap32(arg)
#define B_BENDIAN_TO_HOST_INT64(arg)  __builtin_bswap64(arg)
#define B_BENDIAN_TO_HOST_INT32(arg)  __builtin_bswap32(arg)
#define B_BENDIAN_TO_HOST_INT16(arg)  __builtin_bswap16(arg)

#else
/* Host is big endian */
#define B_HOST_IS_LENDIAN 0
#define B_HOST_IS_BENDIAN 1

/* Similar definitions for big endian hosts... */
#define B_HOST_TO_LENDIAN_DOUBLE(arg) __builtin_bswap64(arg)
#define B_HOST_TO_LENDIAN_FLOAT(arg)  __builtin_bswap32(arg)
#define B_HOST_TO_LENDIAN_INT64(arg)  __builtin_bswap64(arg)
#define B_HOST_TO_LENDIAN_INT32(arg)  __builtin_bswap32(arg)
#define B_HOST_TO_LENDIAN_INT16(arg)  __builtin_bswap16(arg)

#define B_LENDIAN_TO_HOST_DOUBLE(arg) __builtin_bswap64(arg)
#define B_LENDIAN_TO_HOST_FLOAT(arg)  __builtin_bswap32(arg)
#define B_LENDIAN_TO_HOST_INT64(arg)  __builtin_bswap64(arg)
#define B_LENDIAN_TO_HOST_INT32(arg)  __builtin_bswap32(arg)
#define B_LENDIAN_TO_HOST_INT16(arg)  __builtin_bswap16(arg)

#define B_HOST_TO_BENDIAN_DOUBLE(arg) (double)(arg)
#define B_HOST_TO_BENDIAN_FLOAT(arg)  (float)(arg)
#define B_HOST_TO_BENDIAN_INT64(arg)  (uint64_t)(arg)
#define B_HOST_TO_BENDIAN_INT32(arg)  (uint32_t)(arg)
#define B_HOST_TO_BENDIAN_INT16(arg)  (uint16_t)(arg)

#define B_BENDIAN_TO_HOST_DOUBLE(arg) (double)(arg)
#define B_BENDIAN_TO_HOST_FLOAT(arg)  (float)(arg)
#define B_BENDIAN_TO_HOST_INT64(arg)  (uint64_t)(arg)
#define B_BENDIAN_TO_HOST_INT32(arg)  (uint32_t)(arg)
#define B_BENDIAN_TO_HOST_INT16(arg)  (uint16_t)(arg)
#endif

// Stub functions for compatibility
typedef int32_t status_t;
typedef uint32_t type_code;

static inline status_t swap_data(type_code type, void *data, size_t length, swap_action action) {
    (void)type; (void)data; (void)length; (void)action;
    return 0; // B_OK
}

static inline bool is_type_swapped(type_code type) {
    (void)type;
    return false;
}

#endif /* _BYTE_ORDER_H */