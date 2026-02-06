#ifndef IMAGE_DEFS_H
#define IMAGE_DEFS_H

//////////////////////////////////////////////////////////////////////////////
// image_defs.h - Stub for missing Haiku image definitions
//
// This file provides minimal image definitions needed
// when image_defs.h is not available in the system headers.
//////////////////////////////////////////////////////////////////////////////

// Image types
#define B_SYSTEM_IMAGE                   0x01
#define B_APP_IMAGE                      0x02
#define B_LIBRARY_IMAGE                  0x03
#define B_ADD_ON_IMAGE                  0x04
#define B_DRIVER_IMAGE                   0x05
#define B_TRANSLATOR_IMAGE               0x06

// Image flags
#define B_USE_LOADED_IMAGES             0x0001
#define B_USE_ENVIRONMENT_VARIABLES      0x0002
#define B_ALLOW_LIBRARY_EXPORTS         0x0004
#define B_APP_PRIVATE_LIBRARY_SHLIB    0x0008
#define B_SYMBOL_LOCALIZED               0x0010
#define B_SYMBOL_EXPORTED               0x0020
#define B_LIBRARY_GLOBAL_RESOURCES       0x0040
#define B_DELAY_INIT                    0x0080
#define B_ALREADY_LOADED                  0x8000

// Add-on flags
#define B_ADD_ON_WITHOUT_RESOURCE      0x0001

// Symbol resolution flags
#define B_SYMBOL_TYPE_TEXT               0x00000001
#define B_SYMBOL_TYPE_DATA               0x00000002
#define B_SYMBOL_TYPE_CODE               B_SYMBOL_TYPE_TEXT
#define B_SYMBOL_TYPE_ANY                0xFFFFFFFF

// Dependency info types
#define B_ADD_ON_DEPENDENCY            0
#define B_LIBRARY_DEPENDENCY           1

#endif // IMAGE_DEFS_H