/**
 * @file UnifiedDefinitionsCorrected.h
 * @brief Centralized definitions for UserlandVM - corrected version
 */

#ifndef UNIFIED_DEFINITIONS_CORRECTED_H
#define UNIFIED_DEFINITIONS_CORRECTED_H

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

// Standard status codes
#ifndef B_OK
#define B_OK                     0
#define B_ERROR                 ((int32_t)-1)
#define B_NO_MEMORY             ((int32_t)-2)
#define B_BAD_VALUE             ((int32_t)-3)
#define B_ENTRY_NOT_FOUND       ((int32_t)-4)
#define B_NAME_TOO_LONG         ((int32_t)-5)
#define B_NOT_ALLOWED           ((int32_t)-6)
#define B_TIMED_OUT             ((int32_t)-7)
#define B_FILE_EXISTS           ((int32_t)-8)
#define B_BUSY                  ((int32_t)-9)
#define B_NO_INIT               ((int32_t)-10)
#define B_DEV_NOT_READY         ((int32_t)-11)
#define B_DEVICE_FULL           ((int32_t)-12)
#define B_BAD_ADDRESS          ((int32_t)-13)
#define B_BAD_DATA             ((int32_t)-14)
#endif

// Platform compatibility
#ifdef PLATFORM_LINUX
    #define HAIKU_OK B_OK
#else
    #define HAIKU_OK B_OK
#endif

// =============================================================================
// ELF32 CONSTANTS - Complete and consistent
// =============================================================================

// ELF magic numbers
#ifndef ELFMAG0
#define ELFMAG0                0x7f
#define ELFMAG1                'E'
#define ELFMAG2                'L'
#define ELFMAG3                'F'
#endif

#ifndef EI_CLASS
#define EI_CLASS               4
#define EI_DATA                5
#define EI_VERSION             6
#define EI_OSABI               7
#define EI_ABIVERSION          8
#endif

#ifndef ELFCLASS32
#define ELFCLASS32             1
#define ELFCLASS64             2
#endif

#ifndef ELFDATA2LSB
#define ELFDATA2LSB            1
#define ELFDATA2MSB            2
#endif

#ifndef EV_CURRENT
#define EV_CURRENT             1
#endif

// ELF types
#ifndef ET_NONE
#define ET_NONE                0
#define ET_REL                 1
#define ET_EXEC                2
#define ET_DYN                 3
#define ET_CORE                4
#endif

// Machine types
#ifndef EM_386
#define EM_386                 3
#define EM_486                 6
#define EM_X86_64              62
#endif

// Program header types
#ifndef PT_NULL
#define PT_NULL                0
#define PT_LOAD                1
#define PT_DYNAMIC             2
#define PT_INTERP              3
#define PT_NOTE                4
#define PT_SHLIB               5
#define PHDR                   6
#endif

// Section header types
#ifndef SHT_NULL
#define SHT_NULL               0
#define SHT_PROGBITS           1
#define SHT_SYMTAB             2
#define SHT_STRTAB             3
#define SHT_RELA               4
#define SHT_HASH               5
#define SHT_DYNAMIC            6
#define SHT_NOTE               7
#define SHT_NOBITS             8
#define SHT_REL                9
#define SHT_DYNSYM            11
#endif

// Symbol bindings
#ifndef STB_LOCAL
#define STB_LOCAL              0
#define STB_GLOBAL             1
#define STB_WEAK               2
#endif

// Symbol types
#ifndef STT_NOTYPE
#define STT_NOTYPE             0
#define STT_OBJECT             1
#define STT_FUNC               2
#define STT_SECTION            3
#define STT_FILE               4
#endif

// Section header special indices
#ifndef SHN_UNDEF
#define SHN_UNDEF              0
#define SHN_LORESERVE          0xff00
#define SHN_LOPROC             0xff00
#define SHN_HIPROC             0xff1f
#define SHN_LOOS               0xff20
#define SHN_HIOS               0xff3f
#define SHN_ABS                0xfff1
#define SHN_COMMON             0xfff2
#define SHN_XINDEX             0xffff
#define SHN_HIRESERVE          0xffff
#endif

// ELF32 relocation macros
#ifndef ELF32_R_SYM
#define ELF32_R_SYM(info)      ((info) >> 8)
#define ELF32_R_TYPE(info)     ((uint8_t)(info))
#define ELF32_R_INFO(sym, type) (((sym) << 8) + ((type) & 0xff))
#endif

// ELF32 symbol info macros
#ifndef ELF32_ST_BIND
#define ELF32_ST_BIND(info)    ((info) >> 4)
#define ELF32_ST_TYPE(info)    ((info) & 0xf)
#define ELF32_ST_INFO(bind, type) (((bind) << 4) + ((type) & 0xf))
#endif

// ELF32 relocation types
#ifndef R_386_NONE
#define R_386_NONE             0
#define R_386_32                1
#define R_386_PC32              2
#define R_386_GOT32             3
#define R_386_PLT32             4
#define R_386_COPY              5
#define R_386_GLOB_DAT          6
#define R_386_JMP_SLOT          7
#define R_386_RELATIVE          8
#define R_386_GOTOFF             9
#define R_386_GOTPC              10
#define R_386_32PLT              11
#define R_386_16                 20
#define R_386_PC16               21
#define R_386_8                  22
#define R_386_PC8                23
#endif

// =============================================================================
// MEMORY PROTECTION CONSTANTS
// =============================================================================

#ifndef PROT_READ
#define PROT_READ              0x1
#define PROT_WRITE             0x2
#define PROT_EXEC              0x4
#define PROT_NONE              0x0
#endif

#ifndef MAP_FAILED
#define MAP_FAILED             ((void *)-1)
#endif

// =============================================================================
// LINUX SYSCALL NUMBERS - Consolidated and deduplicated
// =============================================================================

#ifdef __linux__

// Primary syscall definitions
#define SYS_exit               1
#define SYS_fork               2
#define SYS_read               3
#define SYS_write              4
#define SYS_open               5
#define SYS_close              6
#define SYS_waitpid            7
#define SYS_creat              8
#define SYS_link               9
#define SYS_unlink             10
#define SYS_execve             11
#define SYS_chdir              12
#define SYS_time               13
#define SYS_mknod              14
#define SYS_chmod              15
#define SYS_lchown             16
#define SYS_break              17
#define SYS_oldstat            18
#define SYS_lseek              19
#define SYS_getpid             20
#define SYS_mount              21
#define SYS_umount             22
#define SYS_setuid             23
#define SYS_getuid             24
#define SYS_stime              25
#define SYS_ptrace             26
#define SYS_alarm              27
#define SYS_oldfstat           28
#define SYS_pause              29
#define SYS_utime              30
#define SYS_stty               31
#define SYS_gtty               32
#define SYS_access             33
#define SYS_nice               34
#define SYS_ftime              35
#define SYS_sync               36
#define SYS_kill               37
#define SYS_rename             38
#define SYS_mkdir              39
#define SYS_rmdir              40
#define SYS_dup                41
#define SYS_pipe               42
#define SYS_times              43
#define SYS_prof               44
#define SYS_brk                45
#define SYS_setgid             46
#define SYS_getgid             47
#define SYS_signal             48
#define SYS_geteuid            49
#define SYS_getegid            50
#define SYS_acct               51
#define SYS_umount2            52
#define SYS_lock               53
#define SYS_ioctl              54
#define SYS_fcntl              55
#define SYS_mpx                56
#define SYS_setpgid            57
#define SYS_ulimit             58
#define SYS_oldoldumount       59
#define SYS_statfs             60
#define SYS_pivot_root         61
#define SYS_print              62
#define SYS_oldumount          64

// Extended syscall definitions
#define SYS_mmap               90
#define SYS_munmap             91
#define SYS_truncate           92
#define SYS_ftruncate          93
#define SYS_fchmod             94
#define SYS_fchown             95
#define SYS_getpriority        96
#define SYS_setpriority        97
#define SYS_profil             98
#define SYS_fstatfs2           99
#define SYS_fstatfs            100
#define SYS_ioperm             101
#define SYS_socketcall         102
#define SYS_syslog             103
#define SYS_setitimer          104
#define SYS_getitimer          105
#define SYS_stat               106
#define SYS_lstat              107
#define SYS_fstat              108
#define SYS_olduname           109
#define SYS_iopl               110
#define SYS_vhangup            111
#define SYS_idle               112
#define SYS_vm86old            113
#define SYS_wait4              114
#define SYS_swapoff            115
#define SYS_sysinfo            116
#define SYS_ipc                117
#define SYS_fsync              118
#define SYS_sigreturn          119
#define SYS_clone              120
#define SYS_setdomainname      121
#define SYS_uname              122
#define SYS_modify_ldt         123
#define SYS_adjtimex           124
#define SYS_mprotect           125
#define SYS_sigprocmask        126
#define SYS_create_module      127
#define SYS_init_module        128
#define SYS_delete_module      129
#define SYS_get_kernel_syms    130
#define SYS_quotactl           131
#define SYS_getpgid            132
#define SYS_fchdir             133
#define SYS_bdflush            134
#define SYS_sysfs              135
#define SYS_personality        136
#define SYS_afs_syscall        137
#define SYS_setfsuid           138
#define SYS_setfsgid           139
#define SYS__llseek            140
#define SYS_getdents           141
#define SYS__newselect         142
#define SYS_flock              143
#define SYS_msync              144
#define SYS_readv              145
#define SYS_writev             146
#define SYS_getsid             147
#define SYS_fdatasync          148
#define SYS__sysctl            149
#define SYS_mlock              150
#define SYS_munlock            151
#define SYS_mlockall           152
#define SYS_munlockall         153
#define SYS_vhangup2           154
#define SYS_modify_ldt         155
#define SYS_pivot_root         156

#else

// Haiku/Other platform syscall numbers
#define SYS_exit               1
#define SYS_fork               2
#define SYS_read               3
#define SYS_write              4
#define SYS_open               5
#define SYS_close              6
#define SYS_waitpid            7
#define SYS_creat              8
#define SYS_link               9
#define SYS_unlink             10
#define SYS_execve             11
#define SYS_chdir              12
#define SYS_time               13
#define SYS_mknod              14
#define SYS_chmod              15
#define SYS_lchown             16
#define SYS_break              17
#define SYS_oldstat            18
#define SYS_lseek              19
#define SYS_getpid             20
#define SYS_mount              21
#define SYS_umount             22
#define SYS_setuid             23
#define SYS_getuid             24
#define SYS_stime              25
#define SYS_ptrace             26
#define SYS_alarm              27
#define SYS_oldfstat           28
#define SYS_pause              29
#define SYS_utime              30
#define SYS_stty               31
#define SYS_gtty               32
#define SYS_access             33
#define SYS_nice               34
#define SYS_ftime              35
#define SYS_sync               36
#define SYS_kill               37
#define SYS_rename             38
#define SYS_mkdir              39
#define SYS_rmdir              40
#define SYS_dup                41
#define SYS_pipe               42
#define SYS_times              43
#define SYS_prof               44
#define SYS_brk                45
#define SYS_setgid             46
#define SYS_getgid             47
#define SYS_signal             48
#define SYS_geteuid            49
#define SYS_getegid            50
#define SYS_acct               51
#define SYS_umount2            52
#define SYS_lock               53
#define SYS_ioctl              54
#define SYS_fcntl              55
#define SYS_mpx                56
#define SYS_setpgid            57
#define SYS_ulimit             58
#define SYS_oldoldumount       59
#define SYS_statfs             60
#define SYS_pivot_root         61
#define SYS_print              62
#define SYS_oldumount          64
#define SYS_mmap               90
#define SYS_munmap             91
#define SYS_truncate           92
#define SYS_ftruncate          93
#define SYS_fchmod             94
#define SYS_fchown             95
#define SYS_getpriority        96
#define SYS_setpriority        97
#define SYS_profil             98
#define SYS_fstatfs2           99
#define SYS_fstatfs            100
#define SYS_ioperm             101
#define SYS_socketcall         102
#define SYS_syslog             103
#define SYS_setitimer          104
#define SYS_getitimer          105
#define SYS_stat               106
#define SYS_lstat              107
#define SYS_fstat              108
#define SYS_olduname           109
#define SYS_iopl               110
#define SYS_vhangup            111
#define SYS_idle               112
#define SYS_vm86old            113
#define SYS_wait4              114
#define SYS_swapoff            115
#define SYS_sysinfo            116
#define SYS_ipc                117
#define SYS_fsync              118
#define SYS_sigreturn          119
#define SYS_clone              120
#define SYS_setdomainname      121
#define SYS_uname              122
#define SYS_modify_ldt         123
#define SYS_adjtimex           124
#define SYS_mprotect           125
#define SYS_sigprocmask        126
#define SYS_create_module      127
#define SYS_init_module        128
#define SYS_delete_module      129
#define SYS_get_kernel_syms    130
#define SYS_quotactl           131
#define SYS_getpgid            132
#define SYS_fchdir             133
#define SYS_bdflush            134
#define SYS_sysfs              135
#define SYS_personality        136
#define SYS_afs_syscall        137
#define SYS_setfsuid           138
#define SYS_setfsgid           139
#define SYS__llseek            140
#define SYS_getdents           141
#define SYS__newselect         142
#define SYS_flock              143
#define SYS_msync              144
#define SYS_readv              145
#define SYS_writev             146
#define SYS_getsid             147
#define SYS_fdatasync          148
#define SYS__sysctl            149
#define SYS_mlock              150
#define SYS_munlock            151
#define SYS_mlockall           152
#define SYS_munlockall         153
#define SYS_vhangup            154
#define SYS_modify_ldt         155
#define SYS_pivot_root         156

#endif // __linux__

// =============================================================================
// CPU FLAGS AND REGISTERS
// =============================================================================

// EFLAGS bits
#define CF_FLAG                0x00000001
#define PF_FLAG                0x00000004
#define AF_FLAG                0x00000010
#define ZF_FLAG                0x00000040
#define SF_FLAG                0x00000080
#define TF_FLAG                0x00000100
#define IF_FLAG                0x00000200
#define DF_FLAG                0x00000400
#define OF_FLAG                0x00000800
#define IOPL_FLAG              0x00003000
#define NT_FLAG                0x00004000
#define RF_FLAG                0x00010000
#define VM_FLAG                0x00020000
#define AC_FLAG                0x00040000
#define VIF_FLAG               0x00080000
#define VIP_FLAG               0x00100000
#define ID_FLAG                0x00200000

// =============================================================================
// PERFORMANCE AND DEBUGGING
// =============================================================================

// Performance metrics
struct PerformanceMetrics {
    uint64_t instructions_executed;
    uint64_t cycles_used;
    uint64_t memory_accesses;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t syscalls_made;
    uint64_t exceptions_handled;
    double instructions_per_second;
    double cycles_per_instruction;
};

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

#endif // UNIFIED_DEFINITIONS_CORRECTED_H