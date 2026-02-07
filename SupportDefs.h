#pragma once

// Must include PlatformTypes first for type definitions
#include "PlatformTypes.h"

// Basic Haiku type definitions for HaikuOS compatibility
// Note: Avoid conflicts with system types
#ifndef _SUPPORTDEFS_IMPL_H
#define _SUPPORTDEFS_IMPL_H

// Already defined in PlatformTypes.h - don't redefine

// Common status codes
#ifndef B_OK
#define B_OK 0
#define B_ERROR (-1)
#define B_NO_MEMORY (-2)
#define B_BAD_VALUE (-3)
#define B_ENTRY_NOT_FOUND (-6)
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

#endif

// Common status codes
#define B_OK 0
#define B_ERROR -1
#define B_NO_MEMORY -2
#define B_BAD_VALUE -3
#define B_BAD_TYPE -4
#define B_NAME_NOT_FOUND -5
#define B_ENTRY_NOT_FOUND -6
#define B_PERMISSION_DENIED -7
#define B_FILE_EXISTS -8
#define B_FILE_NOT_FOUND -9
#define B_NOT_ALLOWED -10
#define B_INTERRUPTED -11
#define B_NO_INIT -12
#define B_BUSY -13
#define B_TIMED_OUT -14
#define B_CANCELED -15
#define B_WOULD_BLOCK -16
#define B_FILE_ERROR -17
#define B_IO_ERROR -18

// Flags
#define B_READ_ONLY 0x01
#define B_WRITE_ONLY 0x02
#define B_READ_WRITE 0x03

// Memory area flags (for Haiku compatibility)
#define B_ANY_ADDRESS 0
#define B_NO_LOCK 0
#define B_READ_AREA B_READ_ONLY
#define B_WRITE_AREA B_WRITE_ONLY

// Area management
#ifdef __HAIKU__
// On Haiku, delete_area should be provided by OS.h
// If not available, provide a stub
extern "C" status_t _kern_delete_area(area_id area) __attribute__((weak));
inline status_t delete_area(area_id area) {
  if (_kern_delete_area) return _kern_delete_area(area);
  return B_OK;
}
#else
#define delete_area(area) do { (void)(area); } while(0)
#endif

// Endian definitions
#define B_HOST_IS_LENDIAN 1
#define B_HOST_IS_BENDIAN 0

// For compilation compatibility on non-Haiku systems
#ifndef __HAIKU__
#define __HAIKU__ 0
#endif