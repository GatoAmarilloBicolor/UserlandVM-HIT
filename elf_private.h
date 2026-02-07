#ifndef ELF_PRIVATE_H
#define ELF_PRIVATE_H

// Haiku ELF private definitions stub for UserlandVM-HIT
// Provides essential ELF constants for compilation

#define B_ELF_VERSION 1

// ELF version constants
#define EV_NONE		0
#define EV_CURRENT		1

// ELF section header flags
#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_LOOS	0xff20
#define SHN_HIOS	0xff3f
#define SHN_ABS	0xfff1
#define SHN_COMMON	0xfff2
#define SHN_XINDEX	0xffff
#define SHN_HIRESERVE	0xffff

// ELF symbol binding
#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_LOPROC	13
#define STB_HIPROC	15

// ELF symbol type
#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_COMMON	5
#define STT_TLS	6
#define STT_LOPROC	13
#define STT_HIPROC	15

// ELF symbol visibility
#define STV_DEFAULT	0
#define STV_INTERNAL	1
#define STV_HIDDEN	2
#define STV_PROTECTED	3

// ELF relocation types (x86)
#define R_386_NONE	0
#define R_386_32	1
#define R_386_PC32	2
#define R_386_GOT32	3
#define R_386_PLT32	4
#define R_386_COPY	5
#define R_386_GLOB_DAT	6
#define R_386_JMP_SLOT	7
#define R_386_RELATIVE	8
#define R_386_GOTOFF	9
#define R_386_GOTPC	10
#define R_386_GOT32	3
#define R_386_PLT32	4
#define R_386_32PLT	11

#endif /* ELF_PRIVATE_H */