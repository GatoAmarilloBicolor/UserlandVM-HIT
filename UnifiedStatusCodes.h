// Unified status code definitions for UserlandVM
// Avoids conflicts between guest and host Haiku definitions
#ifndef UNIFIED_STATUS_CODES_H
#define UNIFIED_STATUS_CODES_H

#include <stdint.h>

// Use host Haiku OS status codes exclusively
#include <OS.h>

// Type aliases for compatibility
typedef status_t guest_status_t;
typedef uint32_t guest_uint32_t;
typedef int32_t guest_int32_t;
typedef uint8_t guest_uint8_t;
typedef int8_t guest_int8_t;
typedef uint16_t guest_uint16_t;
typedef int16_t guest_int16_t;
typedef uint64_t guest_uint64_t;
typedef int64_t guest_int64_t;

// Standard guest/host return value
inline status_t make_status(int value) {
    return (status_t)value;
}

#endif // UNIFIED_STATUS_CODES_H
