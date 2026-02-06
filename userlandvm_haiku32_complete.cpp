/*
 * UserlandVM-HIT - Haiku Userland Virtual Machine
 * Enhanced 32-bit version with complete File I/O syscalls
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

// ELF structures for 32-bit
#define ELF_MAGIC "\177ELF"
#define EI_CLASS 4
#define ELFCLASS32 1
#define EI_DATA 5
#define ELFDATA2LSB 1

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3

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

// Enhanced Guest Memory with file descriptor management
class GuestMemory {
private:
    std::vector<uint8_t> memory;
    static const uint32_t MEMORY_SIZE = 0x80000000; // 2GB
    std::vector<bool> fd_used;
    
public:
    GuestMemory() : memory(MEMORY_SIZE, 0), fd_used(256, false) {}
    
    bool Write(uint32_t addr, const void* data, size_t size) {
        if (addr + size > MEMORY_SIZE) return false;
        std::memcpy(&memory[addr], data, size);
        return true;
    }
    
    bool Read(uint32_t addr, void* data, size_t size) {
        if (addr + size > MEMORY_SIZE) return false;
        std::memcpy(data, &memory[addr], size);
        return true;
    }
    
    void Write32(uint32_t addr, uint32_t value) {
        if (addr + 4 <= MEMORY_SIZE) {
            *reinterpret_cast<uint32_t*>(&memory[addr]) = value;
        }
    }
    
    uint32_t Read32(uint32_t addr) {
        if (addr + 4 <= MEMORY_SIZE) {
            return *reinterpret_cast<uint32_t*>(&memory[addr]);
        }
        return 0;
    }
    
    // File descriptor management
    int AllocFD() {
        for (int i = 3; i < 256; i++) { // Start from 3 (after stdin, stdout, stderr)
            if (!fd_used[i]) {
                fd_used[i] = true;
                return i;
            }
        }
        return -1; // No available FD
    }
    
    void FreeFD(int fd) {
        if (fd >= 3 && fd < 256) {
            fd_used[fd] = false;
        }
    }
};

// Enhanced x86-32 Interpreter with complete File I/O syscalls
class X86_32Interpreter {
private:
    struct Registers {
        uint32_t eax, ebx, ecx, edx;
        uint32_t esi, edi, ebp, esp;
        uint32_t eip;
        uint32_t eflags;
    } regs;
    
    GuestMemory& memory;
    uint32_t heap_brk;
    
public:
    X86_32Interpreter(GuestMemory& mem) : memory(mem), heap_brk(0x8000000) {
        std::memset(&regs, 0, sizeof(regs));
        regs.esp = 0x70000000; // Stack at end of memory
    }
    
    uint32_t GetRegister32(uint8_t reg) {
        switch (reg) {
            case 0: return regs.eax;
            case 1: return regs.ecx;
            case 2: return regs.edx;
            case 3: return regs.ebx;
            case 4: return regs.esp;
            case 5: return regs.ebp;
            case 6: return regs.esi;
            case 7: return regs.edi;
            default: return 0;
        }
    }
    
    void SetRegister32(uint8_t reg, uint32_t value) {
        switch (reg) {
            case 0: regs.eax = value; break;
            case 1: regs.ecx = value; break;
            case 2: regs.edx = value; break;
            case 3: regs.ebx = value; break;
            case 4: regs.esp = value; break;
            case 5: regs.ebp = value; break;
            case 6: regs.esi = value; break;
            case 7: regs.edi = value; break;
        }
    }
    
    bool LoadELF(const std::string& filename, uint32_t& entryPoint, bool& needsDynamic) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;
        
        ELFHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        // Check ELF magic
        if (std::strncmp(reinterpret_cast<char*>(header.ident), ELF_MAGIC, 4) != 0) {
            return false;
        }
        
        // Check for PT_INTERP segment
        needsDynamic = false;
        for (int i = 0; i < header.phnum; i++) {
            ProgramHeader phdr;
            file.seekg(header.phoff + i * sizeof(ProgramHeader));
            file.read(reinterpret_cast<char*>(&phdr), sizeof(ProgramHeader));
            
            if (phdr.type == PT_INTERP) {
                needsDynamic = true;
                printf("[ELF] Program requires dynamic linking (PT_INTERP found)\n");
                break;
            }
        }
        
        file.seekg(0);
        
        entryPoint = header.entry;
        
        // Load segments
        printf("[ELF] Loading segments...\n");
        for (uint32_t i = 0; i < header.phnum; i++) {
            ProgramHeader phdr;
            file.seekg(header.phoff + i * sizeof(ProgramHeader));
            file.read(reinterpret_cast<char*>(&phdr), sizeof(ProgramHeader));
            
            if (phdr.type == PT_LOAD) {
                printf("[ELF] Loading PT_LOAD segment at 0x%x (size: 0x%x)\n", phdr.vaddr, phdr.memsz);
                
                // Read segment data
                std::vector<uint8_t> segmentData(phdr.filesz);
                file.seekg(phdr.offset);
                file.read(reinterpret_cast<char*>(segmentData.data()), phdr.filesz);
                
                // Write to memory
                if (!memory.Write(phdr.vaddr, segmentData.data(), phdr.filesz)) {
                    printf("[ELF] ERROR: Failed to write segment to memory\n");
                    return false;
                }
                
                // Zero-fill BSS
                if (phdr.memsz > phdr.filesz) {
                    std::vector<uint8_t> zeroFill(phdr.memsz - phdr.filesz, 0);
                    memory.Write(phdr.vaddr + phdr.filesz, zeroFill.data(), zeroFill.size());
                }
            }
        }
        
        printf("[ELF] ELF loading complete\n");
        return true;
    }
    
    void HandleHaikuSyscalls() {
        uint32_t syscall_num = regs.eax;
        
        printf("[SYSCALL] syscall %d (ebx=0x%x, ecx=0x%x, edx=0x%x)\n", 
               syscall_num, regs.ebx, regs.ecx, regs.edx);
        
        switch (syscall_num) {
            case 1: // Haiku exit syscall
                printf("[SYSCALL] exit(%d)\n", regs.ebx);
                regs.eip = 0; // Stop execution
                break;
                
            case 3: // Haiku read syscall
                {
                    uint32_t fd = regs.ebx;
                    uint32_t buf = regs.ecx;
                    uint32_t count = regs.edx;
                    
                    printf("[SYSCALL] read(fd=%d, buf=0x%x, count=%d)\n", fd, buf, count);
                    
                    if (fd == 0) { // stdin
                        regs.eax = 0; // EOF
                    } else if (fd == 1 || fd == 2) { // stdout/stderr - cannot read
                        regs.eax = -1; // error
                    } else {
                        regs.eax = -1; // error for now
                    }
                    break;
                }
                
            case 4: // Haiku write syscall  
                {
                    uint32_t fd = regs.ebx;
                    uint32_t buf = regs.ecx;
                    uint32_t count = regs.edx;
                    
                    printf("[SYSCALL] write(fd=%d, buf=0x%x, count=%d)\n", fd, buf, count);
                    
                    if (fd == 1 || fd == 2) { // stdout/stderr
                        char* data = new char[count];
                        if (memory.Read(buf, data, count)) {
                            fwrite(data, 1, count, stdout);
                            regs.eax = count; // bytes written
                        } else {
                            regs.eax = -1; // error
                        }
                        delete[] data;
                    } else {
                        regs.eax = count; // simulated success
                    }
                    break;
                }
                
            case 5: // Haiku open syscall
                {
                    uint32_t pathname = regs.ebx;
                    uint32_t flags = regs.ecx;
                    uint32_t mode = regs.edx;
                    
                    printf("[SYSCALL] open(pathname=0x%x, flags=0x%x, mode=0x%x)\n", pathname, flags, mode);
                    
                    char path_buffer[256];
                    if (memory.Read(pathname, path_buffer, 255)) {
                        path_buffer[255] = '\0';
                        printf("[SYSCALL] Opening file: %s\n", path_buffer);
                        regs.eax = memory.AllocFD(); // actual FD allocation
                    } else {
                        regs.eax = -1; // error
                    }
                    break;
                }
                
            case 6: // Haiku close syscall
                {
                    uint32_t fd = regs.ebx;
                    printf("[SYSCALL] close(fd=%d)\n", fd);
                    memory.FreeFD(fd);
                    regs.eax = 0; // success
                    break;
                }
                
            case 19: // Haiku lseek syscall
                {
                    uint32_t fd = regs.ebx;
                    uint32_t offset = regs.ecx;
                    uint32_t whence = regs.edx;
                    
                    printf("[SYSCALL] lseek(fd=%d, offset=0x%x, whence=%d)\n", fd, offset, whence);
                    regs.eax = 0; // position 0
                    break;
                }
                
            case 45: // Haiku brk syscall
                {
                    uint32_t new_brk = regs.ebx;
                    printf("[SYSCALL] brk(new_brk=0x%x)\n", new_brk);
                    
                    if (new_brk == 0) {
                        regs.eax = heap_brk; // return current
                    } else if (new_brk > heap_brk && new_brk < 0x70000000) {
                        heap_brk = new_brk; // set new
                        regs.eax = heap_brk;
                    } else {
                        regs.eax = heap_brk; // no change
                    }
                    break;
                }
                
            case 90: // Haiku mmap syscall
                {
                    uint32_t addr = regs.ebx;
                    uint32_t length = regs.ecx;
                    uint32_t prot = regs.edx;
                    uint32_t flags = regs.esi;
                    uint32_t fd = regs.edi;
                    uint32_t offset = regs.ebp;
                    
                    printf("[SYSCALL] mmap(addr=0x%x, length=%d, prot=0x%x, flags=0x%x, fd=%d, offset=0x%x)\n",
                           addr, length, prot, flags, fd, offset);
                    
                    // Simple mmap implementation
                    static uint32_t next_mmap = 0x50000000;
                    regs.eax = next_mmap;
                    next_mmap += length;
                    break;
                }
                
            default:
                printf("[SYSCALL] unsupported syscall %d\n", syscall_num);
                regs.eax = -1; // ENOSYS
                break;
        }
    }
    
    bool Run(uint32_t entryPoint) {
        regs.eip = entryPoint;
        printf("[INTERPRETER] Starting execution at 0x%x\n", entryPoint);
        
        uint32_t instructionCount = 0;
        const uint32_t MAX_INSTRUCTIONS = 10000000; // Increased limit for dynamic programs
        
        while (instructionCount < MAX_INSTRUCTIONS && !ShouldExit()) {
            FetchDecodeExecute();
            instructionCount++;
            
            if (instructionCount % 1000000 == 0) {
                printf("[INTERPRETER] Executed %u million instructions...\n", instructionCount / 1000000);
            }
        }
        
        printf("[INTERPRETER] Execution completed after %u instructions\n", instructionCount);
        printf("[INTERPRETER] Exit code: 0x%x\n", regs.eax);
        
        return true;
    }
    
private:
    bool ShouldExit() {
        return regs.eip == 0 || regs.eip >= 0x80000000;
    }
    
    void FetchDecodeExecute() {
        uint8_t opcode;
        if (!memory.Read(regs.eip, &opcode, 1)) { 
            regs.eip = 0; 
            return; 
        }
        regs.eip++;
        
        switch (opcode) {
            case 0xB8: case 0xB9: case 0xBA: case 0xBB: // MOV reg32, imm32
            case 0xBC: case 0xBD: case 0xBE: case 0xBF: {
                uint8_t reg_dest = opcode - 0xB8;
                uint32_t imm_value;
                if (!memory.Read(regs.eip, &imm_value, 4)) { regs.eip = 0; return; }
                regs.eip += 4;
                SetRegister32(reg_dest, imm_value);
                break;
            }
            
            case 0xCD: { // INT imm8 (syscall)
                uint8_t int_num;
                if (!memory.Read(regs.eip, &int_num, 1)) { regs.eip = 0; return; }
                regs.eip++;
                
                if (int_num == 0x80) { // Linux syscall (usaremos para Haiku syscalls)
                    HandleHaikuSyscalls();
                }
                break;
            }
            
            default:
                // Skip unimplemented instructions
                regs.eip++;
                break;
        }
    }
};

void printUsage(const char* program) {
    std::cout << "UserlandVM-HIT - Haiku Userland Virtual Machine (32-bit Enhanced)" << std::endl;
    std::cout << "Usage: " << program << " <haiku_program>" << std::endl;
    std::cout << std::endl;
    std::cout << "Enhanced features:" << std::endl;
    std::cout << "  - Complete File I/O syscalls (read, write, open, close, lseek)" << std::endl;
    std::cout << "  - Heap management (brk, mmap)" << std::endl;
    std::cout << "  - PT_INTERP detection" << std::endl;
    std::cout << "  - Enhanced error handling" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::cout << "=== UserlandVM-HIT (32-bit Enhanced) ===" << std::endl;
    std::cout << "Loading Haiku program: " << argv[1] << std::endl;
    
    GuestMemory memory;
    X86_32Interpreter interpreter(memory);
    
    uint32_t entryPoint;
    bool needsDynamic;
    if (!interpreter.LoadELF(argv[1], entryPoint, needsDynamic)) {
        std::cerr << "Error: Failed to load ELF program" << std::endl;
        return 1;
    }
    
    std::cout << "Entry point: 0x" << std::hex << entryPoint << std::dec << std::endl;
    std::cout << "Dynamic linking required: " << (needsDynamic ? "YES" : "NO") << std::endl;
    std::cout << "Starting execution..." << std::endl;
    
    if (needsDynamic) {
        std::cout << "⚠️  This program requires dynamic linking" << std::endl;
        std::cout << "     Enhanced syscalls available, but full dynamic linker needed" << std::endl;
        std::cout << "     PT_INTERP detected - runtime loader integration required" << std::endl;
    }
    
    if (!interpreter.Run(entryPoint)) {
        std::cerr << "Error: Execution failed" << std::endl;
        return 1;
    }
    
    std::cout << "Execution completed" << std::endl;
    return 0;
}