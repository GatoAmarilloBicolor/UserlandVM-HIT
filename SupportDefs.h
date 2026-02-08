#pragma once

#include <stdint.h>
#include <sys/types.h>

// Haiku-style type definitions - use system types to avoid conflicts
#ifndef _SUPPORTDEFS_H
#define _SUPPORTDEFS_H

#ifdef __HAIKU__
#include <SupportDefs.h>
#else
// For non-Haiku systems
#ifndef status_t
typedef int32_t status_t;
#endif
#ifndef addr_t
typedef uintptr_t addr_t;
#endif
#ifndef phys_addr_t
typedef uintptr_t phys_addr_t;  
#endif
#ifndef vm_addr_t
typedef uintptr_t vm_addr_t;
#endif
#ifndef vm_size_t
typedef size_t vm_size_t;
#endif
#ifndef area_id
typedef int32_t area_id;
#endif
#ifndef team_id
typedef int32_t team_id;
#endif
#endif

// Common status codes
#ifndef B_OK
#define B_OK 0
#define B_ERROR (-1)
#define B_NO_MEMORY (-2)
#define B_BAD_VALUE (-3)
#define B_BAD_TYPE (-4)
#define B_NAME_NOT_FOUND (-5)
#define B_ENTRY_NOT_FOUND (-6)
#define B_PERMISSION_DENIED (-7)
#define B_FILE_EXISTS (-8)
#define B_FILE_NOT_FOUND (-9)
#define B_NOT_ALLOWED (-10)
#define B_INTERRUPTED (-11)
#define B_NO_INIT (-12)
#define B_BUSY (-13)
#define B_TIMED_OUT (-14)
#define B_CANCELED (-15)
#define B_WOULD_BLOCK (-16)
#define B_FILE_ERROR (-17)
#define B_IO_ERROR (-18)
#define B_NAME_IN_USE (-15)
#endif

// Common area flags
#ifndef B_READ_AREA
#define B_READ_AREA 0x01
#define B_WRITE_AREA 0x02
#define B_READ_WRITE (B_READ_AREA | B_WRITE_AREA)
#define B_NO_LOCK 0x00
#define B_ANY_ADDRESS 0x01
#endif

// Cleanup macro
#ifndef delete_area
#ifdef __HAIKU__
#define delete_area(area) delete_area(area)
#else  
#define delete_area(area) do { (void)(area); } while(0)
#endif
#endif

// Additional flags
#ifndef B_READ_ONLY
#define B_READ_ONLY 0x01
#define B_WRITE_ONLY 0x02
#define B_READ_WRITE 0x03
#endif

// Memory area flags (for Haiku compatibility)
#ifndef B_ANY_ADDRESS
#define B_ANY_ADDRESS 0
#endif
#ifndef B_NO_LOCK
#define B_NO_LOCK 0
#endif
#ifndef B_READ_AREA
#define B_READ_AREA B_READ_ONLY
#endif
#ifndef B_WRITE_AREA
#define B_WRITE_AREA B_WRITE_ONLY
#endif

// Endian definitions
#ifndef B_HOST_IS_LENDIAN
#define B_HOST_IS_LENDIAN 1
#endif
#ifndef B_HOST_IS_BENDIAN
#define B_HOST_IS_BENDIAN 0
#endif

// For compilation compatibility on non-Haiku systems
#ifndef __HAIKU__
#define __HAIKU__ 0
#endif

#endif // _SUPPORTDEFS_H