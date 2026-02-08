/**
 * @file UnifiedDefinitionsFixed.h
 * @brief Centralized definitions for UserlandVM - fixed version
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

// Always include elf.h - we assume it's available on Linux
#include <elf.h>
#define HAS_ELF_H 1

// =============================================================================
// STATUS CODES - Centralized and consistent
// =============================================================================

typedef int32_t status_t;

// Standard status codes (based on Haiku/BeOS conventions)
#define B_OK                       0
#define B_NO_MEMORY               (-1)
#define B_BAD_VALUE               (-2)
#define B_BAD_ADDRESS              (-3)
#define B_BAD_DATA                (-4)
#define B_BUFFER_OVERFLOW          (-5)
#define B_PERMISSION_DENIED        (-6)
#define B_ERROR                   (-7)
#define B_FILE_ERROR              (-8)
#define B_ENTRY_NOT_FOUND         (-9)
#define B_NAME_TOO_LONG           (-10)
#define B_DEVICE_FULL             (-11)
#define B_NO_INIT                 (-12)
#define B_BUSY                    (-13)
#define B_TIMED_OUT               (-14)
#define B_WOULD_BLOCK             (-15)
#define B_CANCELED                (-16)
#define B_LINK_LIMIT              (-17)
#define B_NOT_SUPPORTED           (-18)
#define B_MISMATCHED_VALUES       (-19)

// =============================================================================
// ELF CONSTANTS - Complete definitions
// =============================================================================

// ELF magic numbers (if elf.h not available)
#ifndef ELFMAG0
#define ELFMAG0    0x7f
#define ELFMAG1    'E'
#define ELFMAG2    'L'
#define ELFMAG3    'F'
#endif

#ifndef EI_CLASS
#define EI_CLASS   4
#define EI_DATA    5
#define EI_VERSION 6
#define EI_OSABI   7
#define EI_ABIVERSION 8
#endif

#ifndef ELFCLASS32
#define ELFCLASS32 1
#define ELFCLASS64 2
#endif

#ifndef ELFDATA2LSB
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2
#endif

#ifndef EV_CURRENT
#define EV_CURRENT 1
#endif

// ELF types
#ifndef ET_NONE
#define ET_NONE     0
#define ET_REL      1
#define ET_EXEC     2
#define ET_DYN      3
#define ET_CORE     4
#endif

// ELF machines
#ifndef EM_386
#define EM_386      3
#define EM_486       6
#define EM_586       7
#endif

// ELF program header types
#ifndef PT_NULL
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4
#define PT_SHLIB    5
#define PT_PHDR     6
#define PT_TLS      7
#endif

// ELF section header types
#ifndef SHT_NULL
#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_RELA     4
#define SHT_HASH     5
#define SHT_DYNAMIC  6
#define SHT_NOTE     7
#define SHT_NOBITS   8
#define SHT_REL      9
#define SHT_SHLIB    10
#define SHT_DYNSYM   11
#endif

// ELF symbol binding
#ifndef STB_LOCAL
#define STB_LOCAL   0
#define STB_GLOBAL  1
#define STB_WEAK    2
#endif

// ELF symbol types
#ifndef STT_NOTYPE
#define STT_NOTYPE  0
#define STT_OBJECT   1
#define STT_FUNC     2
#define STT_SECTION  3
#define STT_FILE     4
#endif

// ELF special section indices
#ifndef SHN_UNDEF
#define SHN_UNDEF     0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC    0xff00
#define SHN_HIPROC    0xff1f
#define SHN_LOOS      0xff20
#define SHN_HIOS      0xff3f
#define SHN_ABS       0xfff1
#define SHN_COMMON    0xfff2
#define SHN_XINDEX    0xffff
#define SHN_HIRESERVE 0xffff
#endif

// ELF macro helpers
#ifndef ELF32_R_SYM
#define ELF32_R_SYM(i)  ((i)>>8)
#endif

#ifndef ELF32_R_TYPE
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#endif

#ifndef ELF32_ST_BIND
#define ELF32_ST_BIND(i) ((i)>>4)
#endif

#ifndef ELF32_ST_TYPE
#define ELF32_ST_TYPE(i) ((i)&0xf)
#endif

// =============================================================================
// RELOCATION TYPES - Complete x86-32 relocations
// =============================================================================

#define R_386_NONE      0   // No relocation
#define R_386_32        1   // Direct absolute relocation
#define R_386_PC32      2   // PC-relative relocation
#define R_386_GOT32     3   // 32-bit GOT entry
#define R_386_PLT32     4   // 32-bit PLT address
#define R_386_COPY       5   // Copy symbol at runtime
#define R_386_GLOB_DAT  6   // Set GOT entry to data address
#define R_386_JMP_SLOT  7   // Set GOT entry to function address
#define R_386_RELATIVE  8   // Base-relative relocation
#define R_386_GOTOFF    9   // Offset to GOT
#define R_386_GOTPC     10  // PC-relative GOT offset
#define R_386_32PLT     11  // 32-bit PLT address
#define R_386_16        20  // 16-bit absolute relocation
#define R_386_PC16      21  // 16-bit PC-relative
#define R_386_8         22  // 8-bit absolute relocation
#define R_386_PC8       23  // 8-bit PC-relative

// =============================================================================
// MEMORY CONSTANTS - Standardized
// =============================================================================

// Memory protection flags
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define PROT_EXEC     0x4

// Memory mapping flags
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20

#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#endif

// Page sizes
#define PAGE_SIZE_4K    4096
#define PAGE_SIZE_64K   65536

// Memory sizes (4GB address space)
#define GB_4_SIZE       0x100000000ULL    // 4GB
#define GB_4_SIZE_32    0x10000000       // 4GB as uint32_t (clipped)
#define MB_128_SIZE     0x08000000       // 128MB
#define MB_16_SIZE      0x01000000       // 16MB
#define MB_64_SIZE      0x04000000       // 64MB
#define MB_32_SIZE      0x02000000       // 32MB

// Standard memory layout for ET_DYN
#define ET_DYN_LOAD_BASE      0x08000000     // 128MB
#define ET_DYN_CODE_BASE     (ET_DYN_LOAD_BASE)
#define ET_DYN_CODE_SIZE     MB_16_SIZE
#define ET_DYN_DATA_BASE     (ET_DYN_CODE_BASE + ET_DYN_CODE_SIZE)
#define ET_DYN_DATA_SIZE     MB_16_SIZE
#define ET_DYN_HEAP_BASE     (ET_DYN_DATA_BASE + ET_DYN_DATA_SIZE)
#define ET_DYN_HEAP_SIZE     MB_64_SIZE
#define ET_DYN_STACK_BASE    0xC0000000     // 3GB mark
#define ET_DYN_STACK_SIZE    MB_32_SIZE

// =============================================================================
// INSTRUCTION CONSTANTS - Complete opcode definitions
// =============================================================================

// 0x0F prefix opcodes
#define OP_0F_JO      0x80   // Jump if overflow
#define OP_0F_JNO     0x81   // Jump if not overflow
#define OP_0F_JB      0x82   // Jump if below/carry
#define OP_0F_JNB     0x83   // Jump if not below/carry
#define OP_0F_JZ      0x84   // Jump if zero/equal
#define OP_0F_JNZ     0x85   // Jump if not zero/equal
#define OP_0F_JBE     0x86   // Jump if below/equal
#define OP_0F_JNBE    0x87   // Jump if not below/equal
#define OP_0F_JS      0x88   // Jump if sign
#define OP_0F_JNS     0x89   // Jump if not sign
#define OP_0F_JP      0x8A   // Jump if parity
#define OP_0F_JNP     0x8B   // Jump if not parity
#define OP_0F_JL      0x8C   // Jump if less
#define OP_0F_JNL     0x8D   // Jump if not less
#define OP_0F_JLE     0x8E   // Jump if less/equal
#define OP_0F_JNLE    0x8F   // Jump if not less/equal

// GROUP opcodes
#define OP_GROUP_80   0x80   // Arithmetic immediate 8-bit
#define OP_GROUP_81   0x81   // Arithmetic immediate 32-bit
#define OP_GROUP_83   0x83   // Arithmetic immediate sign-extended 8-bit
#define OP_GROUP_C0   0xC0   // Shift/rotate immediate 8-bit
#define OP_GROUP_C1   0xC1   // Shift/rotate immediate 8-bit
#define OP_GROUP_D0   0xD0   // Shift/rotate implicit 1-bit
#define OP_GROUP_D1   0xD1   // Shift/rotate implicit cl
#define OP_GROUP_D2   0xD2   // Shift/rotate immediate cl
#define OP_GROUP_D3   0xD3   // Shift/rotate implicit cl
#define OP_GROUP_F6   0xF6   // Group F6
#define OP_GROUP_F7   0xF7   // Group F7
#define OP_GROUP_FE   0xFE   // Group FE
#define OP_GROUP_FF   0xFF   // Group FF

// I/O opcodes
#define OP_IN_AL_DX   0xEC   // IN AL, DX
#define OP_IN_EAX_DX  0xED   // IN EAX, DX
#define OP_OUT_DX_AL  0xEE   // OUT DX, AL
#define OP_OUT_DX_EAX 0xEF   // OUT DX, EAX
#define OP_INS_B      0x6C   // INS BYTE
#define OP_INS_W      0x6D   // INS WORD
#define OP_OUTS_B     0x6E   // OUTS BYTE
#define OP_OUTS_W     0x6F   // OUTS WORD

// =============================================================================
// CPU FLAGS - Complete flag definitions
// =============================================================================

// EFLAGS bits
#define EFLAGS_CF    0x000001    // Carry flag
#define EFLAGS_PF    0x000004    // Parity flag
#define EFLAGS_AF    0x000010    // Auxiliary carry
#define EFLAGS_ZF    0x000040    // Zero flag
#define EFLAGS_SF    0x000080    // Sign flag
#define EFLAGS_TF    0x000100    // Trap flag
#define EFLAGS_IF    0x000200    // Interrupt enable
#define EFLAGS_DF    0x000400    // Direction flag
#define EFLAGS_OF    0x000800    // Overflow flag
#define EFLAGS_NT    0x004000    // Nested task
#define EFLAGS_RF    0x010000    // Resume flag
#define EFLAGS_VM    0x020000    // Virtual mode
#define EFLAGS_AC    0x040000    // Alignment check
#define EFLAGS_VIF   0x080000    // Virtual interrupt flag
#define EFLAGS_VIP   0x100000    // Virtual interrupt pending
#define EFLAGS_ID    0x200000    // CPUID

// =============================================================================
// SYSTEM CALLS - Common Linux x86-32 syscalls
// =============================================================================

#define SYS_EXIT        1
#define SYS_FORK        2
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_OPEN        5
#define SYS_CLOSE       6
#define SYS_WAITPID     7
#define SYS_CREAT       8
#define SYS_LINK        9
#define SYS_UNLINK      10
#define SYS_EXECVE      11
#define SYS_CHDIR       12
#define SYS_TIME        13
#define SYS_MKNOD       14
#define SYS_CHMOD       15
#define SYS_LCHOWN      16
#define SYS_BREAK       17
#define SYS_OLDSTAT     18
#define SYS_LSEEK       19
#define SYS_GETPID      20
#define SYS_MOUNT       21
#define SYS_UMOUNT      22
#define SYS_SETUID      23
#define SYS_GETUID      24
#define SYS_STIME       25
#define SYS_PTRACE      26
#define SYS_ALARM       27
#define SYS_OLDFSTAT    28
#define SYS_PAUSE       29
#define SYS_UTIME       30
#define SYS_STTY        31
#define SYS_GTTY        32
#define SYS_ACCESS      33
#define SYS_NICE        34
#define SYS_FTIME       35
#define SYS_SYNC        36
#define SYS_KILL        37
#define SYS_RENAME      38
#define SYS_MKDIR       39
#define SYS_RMDIR       40
#define SYS_DUP         41
#define SYS_PIPE        42
#define SYS_TIMES       43
#define SYS_PROF        44
#define SYS_BRK         45
#define SYS_SETGID      46
#define SYS_GETGID      47
#define SYS_SIGNAL      48
#define SYS_GETEUID     49
#define SYS_GETEGID     50
#define SYS_ACCT        51
#define SYS_UMOUNT2     52
#define SYS_LOCK        53
#define SYS_IOCTL       54
#define SYS_FCNTL       55
#define SYS_MPX         56
#define SYS_SETPGRP     57
#define SYS_ULIMIT      58
#define SYS_OLDOLDUMOUNT 59
#define SYS_STATFS      60
#define SYS_UMOUNT      61
#define SYS_PIVOT_ROOT  62
#define SYS_PRINT       63
#define SYS_OLDUMOUNT   64
#define SYS_UMOUNT      65
#define SYS_SIGRETURN   119
#define SYS_MMAP        90
#define SYS_MUNMAP      91
#define SYS_MPROTECT    125

// =============================================================================
// ERROR HANDLING - Centralized error management
// =============================================================================

// Error categories
enum ErrorCategory {
    ERROR_NONE,
    ERROR_SYSTEM,
    ERROR_MEMORY,
    ERROR_ELF,
    ERROR_RELOCATION,
    ERROR_OPCODE,
    ERROR_IO,
    ERROR_PERMISSION,
    ERROR_TIMEOUT,
    ERROR_VALIDATION
};

// Error reporting function types
typedef void (*ErrorCallback)(ErrorCategory category, int error_code, const char* message, void* user_data);

// =============================================================================
// DEBUG AND LOGGING - Standardized debugging
// =============================================================================

// Log levels
enum LogLevel {
    LOG_NONE = 0,
    LOG_ERROR = 1,
    LOG_WARN = 2,
    LOG_INFO = 3,
    LOG_DEBUG = 4,
    LOG_TRACE = 5
};

// =============================================================================
// PERFORMANCE - Standardized performance metrics
// =============================================================================

// Performance counters
struct PerformanceCounters {
    uint64_t instructions_executed;
    uint64_t cycles_elapsed;
    uint64_t memory_reads;
    uint64_t memory_writes;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t syscalls_made;
    uint64_t interrupts_handled;
    uint64_t branches_taken;
    uint64_t branches_not_taken;
    uint64_t page_faults;
    
    // Timing
    uint64_t execution_start_time_ns;
    uint64_t execution_end_time_ns;
    
    void Reset() {
        memset(this, 0, sizeof(*this));
    }
    
    double GetInstructionsPerSecond() const {
        if (execution_end_time_ns <= execution_start_time_ns) return 0.0;
        uint64_t time_diff_ns = execution_end_time_ns - execution_start_time_ns;
        return (double)instructions_executed * 1000000000.0 / (double)time_diff_ns;
    }
    
    double GetCyclesPerInstruction() const {
        if (instructions_executed == 0) return 0.0;
        return (double)cycles_elapsed / (double)instructions_executed;
    }
};

// =============================================================================
// COMPATIBILITY - Platform compatibility layer
// =============================================================================

// Platform detection
#ifdef __linux__
    #define PLATFORM_LINUX 1
    #define PLATFORM_HAIKU 0
#elif defined(__HAIKU__)
    #define PLATFORM_LINUX 0
    #define PLATFORM_HAIKU 1
#else
    #define PLATFORM_LINUX 0
    #define PLATFORM_HAIKU 0
    #define PLATFORM_UNKNOWN 1
#endif

// Platform-specific includes
#ifdef PLATFORM_LINUX
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <unistd.h>
#elif defined(PLATFORM_HAIKU)
    // Haiku-specific includes would go here
#endif

// =============================================================================
// VALIDATION MACROS
// =============================================================================

// Memory validation
#define IS_VALID_32BIT_ADDRESS(addr) ((addr) < 0x100000000ULL)
#define IS_VALID_64BIT_ADDRESS(addr) ((addr) < 0x100000000ULL) // Clip to 4GB for now

// Pointer validation
#define IS_ALIGNED(ptr, alignment) (((uintptr_t)(ptr) & ((alignment) - 1)) == 0)
#define IS_PAGE_ALIGNED(ptr) IS_ALIGNED(ptr, PAGE_SIZE_4K)

// Range validation
#define IS_IN_RANGE(addr, base, size) ((addr) >= (base) && (addr) < ((base) + (size)))

// =============================================================================
// UTILITY MACROS
// =============================================================================

// Bit manipulation
#define BIT_SET(val, bit)       ((val) |= (bit))
#define BIT_CLEAR(val, bit)     ((val) &= ~(bit))
#define BIT_TEST(val, bit)      (((val) & (bit)) != 0)
#define BIT_TOGGLE(val, bit)    ((val) ^= (bit))

// Min/Max
#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max)    ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

// Array size
#define ARRAY_SIZE(arr)          (sizeof(arr) / sizeof(arr[0]))

// String handling
#define STRINGIFY(x)            #x
#define TOSTRING(x)             STRINGIFY(x)

// Alignment
#define ALIGN_UP(size, align)   (((size) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(size, align)  ((size) & ~((align) - 1))

// =============================================================================
// COMPATIBILITY WITH OLD CODE
// =============================================================================

// Legacy support for old type names
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;
typedef int32_t  int32;
typedef int16_t  int16;
typedef int8_t   int8;

// Legacy status codes compatibility
#ifndef B_OK
#define B_OK 0
#endif

// Legacy ELF constants
#ifndef ET_DYN
#define ET_DYN 3
#endif

#ifndef PT_LOAD
#define PT_LOAD 1
#endif

// =============================================================================
// DEBUG BUILD CONFIGURATION
// =============================================================================

#ifdef DEBUG
    #define DEBUG_ASSERT(cond) \
        do { \
            if (!(cond)) { \
                fprintf(stderr, "ASSERTION FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
                abort(); \
            } \
        } while(0)
    #define DEBUG_LOG(fmt, ...) \
        fprintf(stderr, "[DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define DEBUG_ASSERT(cond) do {} while(0)
    #define DEBUG_LOG(fmt, ...) do {} while(0)
#endif

#endif // UNIFIED_DEFINITIONS_FIXED_H