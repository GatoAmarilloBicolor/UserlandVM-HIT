#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

#include <cstdint>
#include <cstddef>

// Unified type definitions for Haiku UserlandVM
// This header resolves type inconsistencies across the codebase

// Basic integer types - use unsigned for IDs and parameters
typedef uint32_t haiku_id_t;
typedef uint32_t haiku_param_t;
typedef uint32_t haiku_error_t;
typedef uint32_t haiku_status_t;

// Signed types for values that can be negative
typedef int32_t haiku_value_t;
typedef int32_t haiku_offset_t;

// Pointer types - be explicit about const-ness
typedef void* haiku_pointer_t;
typedef const void* haiku_const_pointer_t;
typedef char* haiku_string_t;
typedef const char* haiku_const_string_t;

// System-specific types
typedef uint32_t haiku_area_id;
typedef uint32_t haiku_port_id;
typedef uint32_t haiku_sem_id;
typedef uint32_t haiku_team_id;
typedef uint32_t haiku_thread_id;
typedef uint32_t haiku_image_id;

// BeAPI types
typedef uint32_t be_app_id;
typedef uint32_t be_window_id;
typedef uint32_t be_view_id;
typedef uint32_t be_message_id;

// Error codes
#define HAIKU_OK                    0
#define HAIKU_ERROR                 ((haiku_error_t)-1)
#define HAIKU_BAD_VALUE             ((haiku_error_t)-2147483647)  // B_BAD_VALUE
#define HAIKU_NO_MEMORY             ((haiku_error_t)-2147483646)  // B_NO_MEMORY
#define HAIKU_NOT_FOUND             ((haiku_error_t)-2147483645)  // B_NOT_FOUND
#define HAIKU_PERMISSION_DENIED     ((haiku_error_t)-2147483644)  // B_PERMISSION_DENIED

// Common constants
#define HAIKU_MAX_PATH_LENGTH       1024
#define HAIKU_MAX_NAME_LENGTH       256
#define HAIKU_MAX_MESSAGE_SIZE      32768

// Type safety macros
#define HAIKU_ID_VALID(id)          ((id) != 0 && (id) != ((haiku_id_t)-1))
#define HAIKU_ERROR_CHECK(status)   ((status) < HAIKU_OK)

// Template parameter types - use non-const for consistency
template<typename T>
using haiku_template_param = T*;

// Common template instantiations
typedef haiku_template_param<char> haiku_char_template_t;
typedef haiku_template_param<void> haiku_void_template_t;

#endif // PLATFORM_TYPES_H