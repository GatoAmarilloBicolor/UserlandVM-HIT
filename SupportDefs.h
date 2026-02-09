#pragma once

// Must include OS.h first to get Haiku's status codes
#include <OS.h>

// Must include PlatformTypes second for type definitions
#include "PlatformTypes.h"

// Use Haiku's built-in definitions exclusively
// This file is now primarily a compatibility shim

#ifndef _SUPPORTDEFS_IMPL_H
#define _SUPPORTDEFS_IMPL_H

// All status codes are now provided by <OS.h>
// No redefinitions needed

// Memory area flags (for Haiku compatibility)
// These should also come from OS.h but define as aliases if needed
#ifndef B_READ_AREA
#define B_READ_AREA 0x01
#define B_WRITE_AREA 0x02
#define B_EXECUTE_AREA 0x04
#endif

#ifndef B_ANY_ADDRESS
#define B_ANY_ADDRESS 0x00
#endif

#ifndef B_NO_LOCK
#define B_NO_LOCK 0x00
#endif

// Area management
#ifdef __HAIKU__
// On Haiku, delete_area is provided by OS.h
#else
#define delete_area(area) do { (void)(area); } while(0)
#endif

// Endian definitions
#define B_HOST_IS_LENDIAN 1
#define B_HOST_IS_BENDIAN 0

#endif // _SUPPORTDEFS_IMPL_H