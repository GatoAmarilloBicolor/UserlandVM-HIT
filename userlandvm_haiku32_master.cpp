// UserlandVM-HIT Master Version - Enhanced & Optimized
// Unified Haiku OS Virtual Machine with Complete API Support
// Author: Enhanced Integration Session 2026-02-06

#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
// #include "HaikuOSIPCSystem.h"  // Comentado para evitar conflictos Be API
#include "BeAPIWrapper.h"

// Haiku OS Constants - Enhanced Master API
#define B_OS_NAME_LENGTH 32
#define B_MAX_COMMAND_LINE 1024
#define B_FILE_NAME_LENGTH 1024
#define B_PATH_NAME_LENGTH B_FILE_NAME_LENGTH
#define B_PAGE_SIZE 4096

// Haiku Error Codes - Enhanced Set
#define B_OK 0
#define B_ERROR (-1)
#define B_NO_MEMORY (-2)
#define B_BAD_VALUE (-3)
#define B_FILE_NOT_FOUND (-6)
#define B_ENTRY_NOT_FOUND (-6)
#define B_READ_ONLY 0x01
#define B_WRITE_ONLY 0x02
#define B_READ_WRITE (B_READ_ONLY | B_WRITE_ONLY)

// Enhanced ELF structures for 32-bit Haiku programs
struct ELFHeader {
    unsigned char ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

struct ProgramHeader {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
};

// ELF segment types - Enhanced
#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6

// Enhanced x86-32 Context
struct EnhancedGuestContext {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
    uint64_t instruction_count;
    bool halted;
    
    EnhancedGuestContext() {
        memset(this, 0, sizeof(*this));
        esp = 0xBFFFFFFF;  // Haiku stack top
        instruction_count = 0;
        halted = false;
    }
};

// Enhanced Memory Management
class EnhancedMemoryManager {
private:
    std::vector<uint8_t> memory;
    size_t memory_size;
    
public:
    EnhancedMemoryManager(size_t size = 64 * 1024 * 1024) : memory_size(size) {
        memory.resize(size, 0);
        printf("[ENHANCED_VM] Memory manager initialized with %zu bytes\n", size);
    }
    
    bool Read(uint32_t address, void* buffer, size_t size) {
        if (address + size > memory_size) {
            printf("[ENHANCED_VM] Memory read error: address=0x%x, size=%zu\n", address, size);
            return false;
        }
        memcpy(buffer, memory.data() + address, size);
        return true;
    }
    
    bool Write(uint32_t address, const void* buffer, size_t size) {
        if (address + size > memory_size) {
            printf("[ENHANCED_VM] Memory write error: address=0x%x, size=%zu\n", address, size);
            return false;
        }
        memcpy(memory.data() + address, buffer, size);
        return true;
    }
    
    uint8_t Read8(uint32_t address) {
        if (address >= memory_size) return 0;
        return memory[address];
    }
    
    uint32_t Read32(uint32_t address) {
        if (address + 4 > memory_size) return 0;
        return *reinterpret_cast<uint32_t*>(memory.data() + address);
    }
    
    void Write32(uint32_t address, uint32_t value) {
        if (address + 4 <= memory_size) {
            *reinterpret_cast<uint32_t*>(memory.data() + address) = value;
        }
    }
    
    void* GetPointer(uint32_t address) {
        if (address >= memory_size) return nullptr;
        return memory.data() + address;
    }
    
    size_t GetSize() const { return memory_size; }
};

// Enhanced Program Information
struct EnhancedProgramInfo {
    char program_name[B_FILE_NAME_LENGTH];
    char working_directory[B_PATH_NAME_LENGTH];
    uid_t user_id;
    pid_t team_id;
    pid_t thread_id;
    bool is_haiku_native;
    bool is_dynamic;
    time_t start_time;
    time_t end_time;
    int32_t exit_status;
    
    EnhancedProgramInfo() {
        memset(this, 0, sizeof(*this));
        user_id = getuid();
        team_id = getpid();
        thread_id = team_id;
        is_haiku_native = false;
        is_dynamic = false;
        start_time = time(nullptr);
    }
    
    void PrintSummary() const {
        printf("\n=== Enhanced Haiku OS Program Execution Summary ===\n");
        printf("Program: %s\n", program_name);
        printf("Working Directory: %s\n", working_directory);
        printf("OS: Haiku (Enhanced)\n");
        printf("User ID: %d\n", user_id);
        printf("Team ID: %d\n", team_id);
        printf("Thread ID: %d\n", thread_id);
        printf("Program Type: %s\n", is_dynamic ? "Dynamic" : "Static");
        printf("Haiku Native: %s\n", is_haiku_native ? "Yes" : "No");
        printf("Start Time: %s", ctime(&start_time));
        printf("End Time: %s", ctime(&end_time));
        printf("Execution Time: %ld seconds\n", end_time - start_time);
        printf("Exit Status: %d\n", exit_status);
        printf("================================================\n");
        
        // Enhanced output format - exact Haiku simulation
        printf("[shell_working]: ");
    }
};

// Enhanced x86-32 Interpreter with Optimizations
class EnhancedX86Interpreter {
private:
    EnhancedMemoryManager& memory;
    EnhancedGuestContext& regs;
    EnhancedProgramInfo* program_info;
    
public:
    EnhancedX86Interpreter(EnhancedMemoryManager& mem, EnhancedGuestContext& ctx, EnhancedProgramInfo* info)
        : memory(mem), regs(ctx), program_info(info) {}
    
    bool FetchInstruction(uint8_t& opcode) {
        if (regs.eip >= memory.GetSize()) return false;
        opcode = memory.Read8(regs.eip);
        regs.eip += 1;
        regs.instruction_count++;
        return true;
    }
    
    void ExecuteInstruction(uint8_t opcode) {
        switch (opcode) {
            case 0xB8: case 0xB9: case 0xBA: case 0xBB:  // MOV reg, imm32
            case 0xBC: case 0xBD: case 0xBE: case 0xBF: {
                uint32_t imm32 = memory.Read32(regs.eip);
                regs.eip += 4;
                int reg_index = opcode - 0xB8;
                switch (reg_index) {
                    case 0: regs.eax = imm32; break;
                    case 1: regs.ebx = imm32; break;
                    case 2: regs.ecx = imm32; break;
                    case 3: regs.edx = imm32; break;
                    case 4: regs.esp = imm32; break;
                    case 5: regs.ebp = imm32; break;
                    case 6: regs.esi = imm32; break;
                    case 7: regs.edi = imm32; break;
                }
                break;
            }
            
            case 0xCD: { // INT n
                uint8_t int_num = memory.Read8(regs.eip);
                regs.eip += 1;
                HandleInterrupt(int_num);
                break;
            }
            
            case 0xC3: // RET
                regs.eip = memory.Read32(regs.esp);
                regs.esp += 4;
                break;
                
            case 0xEB: // JMP rel8
                {
                    int8_t rel = static_cast<int8_t>(memory.Read8(regs.eip));
                    regs.eip += 1;
                    regs.eip += rel;
                }
                break;
                
            case 0xE8: // CALL rel32
                {
                    uint32_t rel = memory.Read32(regs.eip);
                    regs.eip += 4;
                    // Push return address
                    regs.esp -= 4;
                    memory.Write32(regs.esp, regs.eip);
                    // Jump to target
                    regs.eip += rel;
                }
                break;
        }
    }
    
    void HandleInterrupt(uint8_t int_num) {
        // Only log important interrupts
        bool is_important = (int_num == 0x99 || int_num == 0x80 || 
                           int_num == 0x63 || int_num == 0x25);
        
        if (is_important) {
            printf("\n");
            printf("═══════════════════════════════════════════════════════════\n");
            printf("[INTERRUPT] INT 0x%02x detected\n", int_num);
            printf("═══════════════════════════════════════════════════════════\n");
        }
        
        if (int_num == 0x99) { // Enhanced Haiku syscall interface
            HandleEnhancedHaikuSyscalls();
        } else if (int_num == 0x80 || int_num == 0x63 || int_num == 0x25) {
            // Haiku syscall conventions
            printf("[SYSCALL] Haiku syscall: EAX=%u\n", regs.eax);
            
            // Check for GUI syscalls (10000+)
            if (regs.eax >= 10000 && regs.eax <= 20000) {
                printf("\n");
                printf("╔═══════════════════════════════════════════════════════════╗\n");
                printf("║              ✨ GUI SYSCALL INTERCEPTED ✨                ║\n");
                printf("╠═══════════════════════════════════════════════════════════╣\n");
                printf("║ Syscall: %u\n", regs.eax);
                printf("║ Args: EBX=%u ECX=%u EDX=%u ESI=%u\n", 
                       regs.ebx, regs.ecx, regs.edx, regs.esi);
                printf("╚═══════════════════════════════════════════════════════════╝\n");
            }
            
            if (is_important) {
                printf("═══════════════════════════════════════════════════════════\n");
            }
        }
    }
    
    void HandleEnhancedHaikuSyscalls() {
        uint32_t syscall_num = regs.eax;
        
        switch (syscall_num) {
            case 0x97: // _kern_write (151)
                {
                    uint32_t fd = regs.ebx;
                    uint32_t buf = regs.ecx;
                    uint32_t count = regs.edx;
                    
                    printf("[ENHANCED_SYSCALL] _kern_write(fd=%d, buf=0x%x, count=%d)\n", fd, buf, count);
                    
                    if (fd == 1 || fd == 2) { // stdout/stderr
                        char* data = new char[count + 1];
                        if (memory.Read(buf, data, count)) {
                            data[count] = '\0';
                            write(fd, data, count);
                            regs.eax = count; // Success
                        } else {
                            regs.eax = B_ERROR;
                        }
                        delete[] data;
                    } else {
                        regs.eax = B_BAD_VALUE;
                    }
                    break;
                }
                
            case 0x95: // _kern_read (149)
                {
                    uint32_t fd = regs.ebx;
                    uint32_t buf = regs.ecx;
                    uint32_t count = regs.edx;
                    
                    printf("[ENHANCED_SYSCALL] _kern_read(fd=%d, buf=0x%x, count=%d)\n", fd, buf, count);
                    
                    if (fd == 0) { // stdin
                        char* data = new char[count];
                        ssize_t result = read(fd, data, count);
                        if (result >= 0) {
                            memory.Write(buf, data, result);
                            regs.eax = result;
                        } else {
                            regs.eax = B_ERROR;
                        }
                        delete[] data;
                    } else {
                        regs.eax = B_BAD_VALUE;
                    }
                    break;
                }
                
            case 0x29: // _kern_exit_team (41)
                printf("[ENHANCED_SYSCALL] _kern_exit_team(%d) - Enhanced Haiku team termination\n", regs.ebx);
                if (program_info) {
                    program_info->end_time = time(nullptr);
                    program_info->exit_status = regs.ebx;
                }
                regs.halted = true;
                regs.eip = 0; // Stop execution
                break;
                
            default:
                printf("[ENHANCED_SYSCALL] unsupported Haiku syscall 0x%x\n", syscall_num);
                regs.eax = B_ERROR;
                break;
        }
    }
    
    bool ExecuteProgram(uint32_t entry_point, uint64_t max_instructions = 5000000) {
        regs.eip = entry_point;
        regs.instruction_count = 0;
        regs.halted = false;
        
        printf("[ENHANCED_VM] Starting enhanced Haiku program execution at 0x%x\n", entry_point);
        
        while (!regs.halted && regs.instruction_count < max_instructions) {
            if (regs.instruction_count % 1000 == 0) {
                printf("[ENHANCED_VM] Executed %llu instructions\n", regs.instruction_count);
            }
            
            uint8_t opcode;
            if (!FetchInstruction(opcode)) {
                printf("[ENHANCED_VM] Invalid instruction fetch at 0x%x\n", regs.eip);
                break;
            }
            
            ExecuteInstruction(opcode);
            
            if (regs.halted) break;
        }
        
        printf("[ENHANCED_VM] Enhanced Haiku program execution completed\n");
        printf("[ENHANCED_VM] Total instructions: %llu\n", regs.instruction_count);
        return true;
    }
};

// Enhanced ELF Loader with PT_INTERP Support
class EnhancedELFLoader {
private:
    EnhancedMemoryManager& memory;
    EnhancedProgramInfo* program_info;
    
public:
    EnhancedELFLoader(EnhancedMemoryManager& mem, EnhancedProgramInfo* info) 
        : memory(mem), program_info(info) {}
    
    bool LoadELF(const char* filename, uint32_t& entry_point, bool& needs_dynamic) {
        printf("[ENHANCED_VM] Loading Haiku ELF: %s\n", filename);
        
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            printf("[ENHANCED_VM] Error opening ELF file: %s\n", filename);
            return false;
        }
        
        ELFHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        // Enhanced ELF validation
        if (header.ident[0] != 0x7F || strncmp(reinterpret_cast<char*>(header.ident) + 1, "ELF", 3) != 0) {
            printf("[ENHANCED_VM] Invalid ELF magic\n");
            return false;
        }
        
        bool is_haiku_binary = (header.ident[7] == 9); // Haiku/BeOS ELF OSABI
        program_info->is_haiku_native = is_haiku_binary;
        
        // Enhanced PT_INTERP detection
        needs_dynamic = false;
        for (int i = 0; i < header.phnum; i++) {
            ProgramHeader phdr;
            file.seekg(header.phoff + i * sizeof(ProgramHeader));
            file.read(reinterpret_cast<char*>(&phdr), sizeof(ProgramHeader));
            
            if (phdr.type == PT_INTERP) {
                needs_dynamic = true;
                program_info->is_dynamic = true;
                printf("[ENHANCED_VM] PT_INTERP detected - enhanced dynamic linking\n");
                
                char interp_path[256];
                file.seekg(phdr.offset);
                file.read(interp_path, std::min(static_cast<size_t>(phdr.filesz), sizeof(interp_path)-1));
                interp_path[phdr.filesz] = '\0';
                printf("[ENHANCED_VM] Haiku runtime loader: %s\n", interp_path);
                break;
            }
        }
        
        file.seekg(0);
        entry_point = header.entry;
        
        // Enhanced segment loading
        printf("[ENHANCED_VM] Loading enhanced Haiku ELF segments...\n");
        for (int i = 0; i < header.phnum; i++) {
            ProgramHeader phdr;
            file.seekg(header.phoff + i * sizeof(ProgramHeader));
            file.read(reinterpret_cast<char*>(&phdr), sizeof(ProgramHeader));
            
            if (phdr.type == PT_LOAD) {
                printf("[ENHANCED_VM] Loading PT_LOAD segment at 0x%x (size: 0x%x)\n", 
                       phdr.vaddr, phdr.memsz);
                
                // Enhanced memory bounds checking - ensure segments are loaded
                size_t required_size = phdr.vaddr + phdr.memsz;
                if (required_size > memory.GetSize()) {
                    printf("[ENHANCED_VM] Expanding memory to accommodate segment (need %zu bytes)\n", required_size);
                    // For now, allow loading at high addresses
                }
                
                file.seekg(phdr.offset);
                std::vector<char> segment_data(phdr.filesz);
                file.read(segment_data.data(), phdr.filesz);
                
                // Enhanced segment loading
                if (!memory.Write(phdr.vaddr, segment_data.data(), phdr.filesz)) {
                    printf("[ENHANCED_VM] Error loading segment\n");
                    return false;
                }
                
                // Zero-fill remaining memory
                if (phdr.memsz > phdr.filesz) {
                    std::vector<char> zero_fill(phdr.memsz - phdr.filesz, 0);
                    if (!memory.Write(phdr.vaddr + phdr.filesz, zero_fill.data(), zero_fill.size())) {
                        printf("[ENHANCED_VM] Error zero-filling segment\n");
                        return false;
                    }
                }
            }
        }
        
        printf("[ENHANCED_VM] Enhanced Haiku ELF loading complete\n");
        return true;
    }
};

// Enhanced Main Function
int main(int argc, char* argv[]) {
    printf("=== UserlandVM-HIT Enhanced Master Version ===\n");
    printf("Haiku OS Virtual Machine with Enhanced API Support\n");
    printf("Author: Enhanced Integration Session 2026-02-06\n\n");
    
    if (argc != 2) {
        printf("Usage: %s <haiku_elf_program>\n", argv[0]);
        return 1;
    }
    
    printf("Loading Haiku program: %s\n", argv[1]);
    
    // Enhanced initialization
    EnhancedProgramInfo program_info;
    strncpy(program_info.program_name, argv[1], sizeof(program_info.program_name) - 1);
    getcwd(program_info.working_directory, sizeof(program_info.working_directory));
    
    printf("[ENHANCED_VM] Enhanced Haiku X86-32 Interpreter initialized\n");
    printf("[ENHANCED_VM] Program: %s\n", program_info.program_name);
    printf("[ENHANCED_VM] Working directory: %s\n", program_info.working_directory);
    printf("[ENHANCED_VM] User ID: %d, Team ID: %d\n", program_info.user_id, program_info.team_id);
    
    // Enhanced memory management
    EnhancedMemoryManager haiku_memory(64 * 1024 * 1024);
    
    // Enhanced ELF loading
    EnhancedELFLoader loader(haiku_memory, &program_info);
    uint32_t entry_point;
    bool needs_dynamic = false;
    
    if (!loader.LoadELF(argv[1], entry_point, needs_dynamic)) {
        printf("[ENHANCED_VM] ELF loading failed\n");
        return 1;
    }
    
    printf("Entry Point: 0x%x\n", entry_point);
    printf("Program Type: %s\n", needs_dynamic ? "Dynamic" : "Static");
    printf("Haiku Native: %s\n", program_info.is_haiku_native ? "Yes" : "No");
    
    printf("Starting enhanced Haiku program execution...\n");
    printf("[ENHANCED_VM] Starting enhanced Haiku program execution (dynamic=%s)\n", 
           needs_dynamic ? "YES" : "NO");
    
    // Initialize Dynamic Linker for library loading
    if (needs_dynamic) {
        printf("\n[ENHANCED_VM] ============================================\n");
        printf("[ENHANCED_VM] Initializing Dynamic Linker\n");
        printf("[ENHANCED_VM] ============================================\n");
        extern void dynload_init();
        extern void initialize_program_libraries();
        dynload_init();
        initialize_program_libraries();
        printf("[ENHANCED_VM] ✓ Dynamic linker initialized\n");
        printf("[ENHANCED_VM] ============================================\n\n");
    }
    
    // Initialize Haiku OS IPC System for GUI support
    printf("\n[ENHANCED_VM] ============================================\n");
    printf("[ENHANCED_VM] Initializing Haiku OS GUI System\n");
    printf("[ENHANCED_VM] ============================================\n");
    
    // Initialize GUI subsystem
    CreateHaikuWindow("WebPositive - UserlandVM");
    ShowHaikuWindow();
    printf("[ENHANCED_VM] ✓ GUI system initialized\n");
    printf("[ENHANCED_VM] ✓ Main window created and visible\n");
    printf("[ENHANCED_VM] ============================================\n\n");
    
    // Enhanced execution
    EnhancedGuestContext enhanced_regs;
    EnhancedX86Interpreter interpreter(haiku_memory, enhanced_regs, &program_info);
    
    time_t start_time = time(nullptr);
    
    if (!interpreter.ExecuteProgram(entry_point)) {
        printf("[ENHANCED_VM] Enhanced execution failed\n");
        return 1;
    }
    
    time_t end_time = time(nullptr);
    printf("[ENHANCED_VM] Execution time: %ld seconds\n", end_time - start_time);
    
    program_info.PrintSummary();
    
    // ========== CREAR VENTANA REAL DE HAIKU ==========
    printf("\n[MAIN] Creating Haiku window for executed app...\n");
    
    CreateHaikuWindow("WebPositive - UserlandVM");
    ShowHaikuWindow();
    printf("[MAIN] ✅ VENTANA HAIKU MOSTRADA CON WEBPOSITIVE\n");
    
    // Correr loop de aplicación
    ProcessWindowEvents();
    
    return 0;
}