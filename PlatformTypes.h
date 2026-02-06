#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

///////////////////////////////////////////////////////////////////////////////
// PlatformTypes.h - Core Type Definitions
//
// THIS FILE MUST BE INCLUDED FIRST in all compilation units.
// It defines all basic types BEFORE any system headers.
// This prevents circular dependencies and type conflicts.
///////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

///////////////////////////////////////////////////////////////////////////////
// Basic Integer Types (from Haiku, but defined independently)
///////////////////////////////////////////////////////////////////////////////

typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int8_t int8;
typedef uint8_t uint8;

///////////////////////////////////////////////////////////////////////////////
// Haiku-specific Types (defined WITHOUT relying on Haiku headers)
///////////////////////////////////////////////////////////////////////////////

// Return/status type
typedef int32_t status_t;

// Boolean type
typedef uint32_t bool32;

// Memory area identifiers
typedef int32_t area_id;
typedef int32_t team_id;
typedef int32_t thread_id;
typedef int32_t port_id;
typedef int32_t sem_id;

// Address types
typedef uintptr_t addr_t;
typedef uintptr_t phys_addr_t;
typedef uintptr_t vm_addr_t;
typedef size_t vm_size_t;

// Time type
typedef int64_t bigtime_t;

// CPU types
typedef uint32_t cpu_type;
typedef uint32_t cpu_subtype;

///////////////////////////////////////////////////////////////////////////////
// Status Codes (from Haiku, but defined independently)
///////////////////////////////////////////////////////////////////////////////

#define B_OK                       0
#define B_ERROR                    -1
#define B_NO_MEMORY                -2
#define B_BAD_VALUE                -3
#define B_BAD_TYPE                 -4
#define B_NAME_NOT_FOUND           -5
#define B_ENTRY_NOT_FOUND          -6
#define B_PERMISSION_DENIED        -7
#define B_FILE_EXISTS              -8
#define B_FILE_NOT_FOUND           -9
#define B_NOT_ALLOWED              -10
#define B_INTERRUPTED              -11
#define B_NO_INIT                  -12
#define B_BUSY                     -13
#define B_TIMED_OUT                -14
#define B_CANCELED                 -15
#define B_WOULD_BLOCK              -16
#define B_FILE_ERROR               -17
#define B_IO_ERROR                 -18
#define B_NOT_SUPPORTED            -13
#define B_BUFFER_OVERFLOW          -22

///////////////////////////////////////////////////////////////////////////////
// Memory Area Flags (from Haiku)
///////////////////////////////////////////////////////////////////////////////

#define B_READ_AREA                0x01
#define B_WRITE_AREA               0x02
#define B_EXECUTE_AREA             0x04
#define B_READ_WRITE               (B_READ_AREA | B_WRITE_AREA)
#define B_NO_LOCK                  0x00
#define B_ANY_ADDRESS              0x01
#define B_EXACT_ADDRESS            0x02
#define B_LAZY_LOCK                0x04
#define B_FULL_LOCK                0x08

///////////////////////////////////////////////////////////////////////////////
// File Flags (from Haiku, mapping to POSIX)
///////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>

#define B_READ_ONLY                O_RDONLY
#define B_WRITE_ONLY               O_WRONLY
#define B_READ_WRITE_MODE          O_RDWR
#define B_CREATE_FILE              O_CREAT
#define B_ERASE_FILE               O_TRUNC
#define B_OPEN_AT_END              O_APPEND

///////////////////////////////////////////////////////////////////////////////
// Thread/Team Priorities (from Haiku)
///////////////////////////////////////////////////////////////////////////////

#define B_IDLE_PRIORITY            0
#define B_LOWEST_ACTIVE_PRIORITY   1
#define B_LOW_PRIORITY             5
#define B_NORMAL_PRIORITY          10
#define B_DISPLAY_PRIORITY         15
#define B_URGENT_DISPLAY_PRIORITY  20
#define B_REAL_TIME_DISPLAY_PRIORITY 100
#define B_URGENT_PRIORITY          110
#define B_REAL_TIME_PRIORITY       120

///////////////////////////////////////////////////////////////////////////////
// Endian Definitions (from Haiku)
///////////////////////////////////////////////////////////////////////////////

#define B_HOST_IS_LENDIAN          1
#define B_HOST_IS_BENDIAN          0

///////////////////////////////////////////////////////////////////////////////
// ELF-related constants
///////////////////////////////////////////////////////////////////////////////

#define PT_NULL                    0
#define PT_LOAD                    1
#define PT_DYNAMIC                 2
#define PT_INTERP                  3
#define PT_NOTE                    4
#define PT_SHLIB                   5
#define PT_PHDR                    6
#define PT_LOPROC                  0x70000000
#define PT_HIPROC                  0x7fffffff

#define DT_NULL                    0
#define DT_NEEDED                  1
#define DT_PLTRELSZ                2
#define DT_PLTGOT                  3
#define DT_HASH                    4
#define DT_STRTAB                  5
#define DT_SYMTAB                  6
#define DT_RELA                    7
#define DT_RELASZ                  8
#define DT_RELAENT                 9
#define DT_STRSZ                   10
#define DT_SYMENT                  11
#define DT_INIT                    12
#define DT_FINI                    13
#define DT_SONAME                  14
#define DT_RPATH                   15
#define DT_SYMBOLIC                16
#define DT_REL                     17
#define DT_RELSZ                   18
#define DT_RELENT                  19
#define DT_PLTREL                  20
#define DT_DEBUG                   21
#define DT_TEXTREL                 22
#define DT_JMPREL                  23

#define EM_386                     3
#define EM_486                     3
#define EM_68K                     4
#define EM_PPC                     20
#define EM_ARM                     40
#define EM_X86_64                  62
#define EM_ARM64                   183
#define EM_RISCV                   243

#define ELFCLASS32                 1
#define ELFCLASS64                 2

#define ELFMAG0                    0x7f
#define ELFMAG1                    'E'
#define ELFMAG2                    'L'
#define ELFMAG3                    'F'

#define EI_MAG0                    0
#define EI_MAG1                    1
#define EI_MAG2                    2
#define EI_MAG3                    3
#define EI_CLASS                   4
#define EI_DATA                    5
#define EI_VERSION                 6
#define EI_OSABI                   7
#define EI_ABIVERSION              8
#define EI_NIDENT                  16

#define SHN_UNDEF                  0
#define SHN_ABS                    0xfff1

///////////////////////////////////////////////////////////////////////////////
// Architecture-specific constants
///////////////////////////////////////////////////////////////////////////////

#define B_PAGE_SIZE                4096

///////////////////////////////////////////////////////////////////////////////
// Ensure Haiku headers don't redefine our types
///////////////////////////////////////////////////////////////////////////////

#ifdef __HAIKU__
// On Haiku systems, we can now safely include OS.h
// because we've already defined all the types it needs
#include <OS.h>
#else
// On non-Haiku systems, provide stubs for Haiku APIs
extern area_id create_area(const char *name, void **address, uint32 addressSpec,
                          size_t size, uint32 lock, uint32 protection);
extern status_t delete_area(area_id area);
extern area_id clone_area(const char *name, void **address, 
                         uint32 addressSpec, area_id source);
#endif

#endif // PLATFORM_TYPES_H
