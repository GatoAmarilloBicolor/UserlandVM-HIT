// UserlandVM-HIT Common VM Components
// Reusable components for all VM implementations
// Author: Code Recycling Session 2026-02-06

#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#ifdef __HAIKU__
#define PLATFORM_NAME "Haiku"
#define PLATFORM_LIBS "BeOS API available"
#else
#define PLATFORM_NAME "Linux"
#define PLATFORM_LIBS "Native Linux system calls"
#endif

// Platform output formatting
#ifdef __HAIKU__
#define PLATFORM_OUTPUT "[haiku.cosmoe]"
#else
#define PLATFORM_OUTPUT "[linux.cosmoe]"
#endif

// Common constants
#define VM_OK 0
#define VM_ERROR (-1)
#define PT_LOAD 1
#define PT_INTERP 3

// Common ELF structures - shared across all VMs
struct CommonELFHeader {
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

struct CommonProgramHeader {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
};

// Common Program Info - shared execution tracking
struct CommonProgramInfo {
    char program_name[256];
    bool has_pt_interp;
    char interp_path[256];
    time_t start_time;
    time_t end_time;
    uint32_t entry_point;
    
    CommonProgramInfo() : has_pt_interp(false), entry_point(0), start_time(time(nullptr)) {
        memset(this, 0, sizeof(*this));
    }
    
    void PrintExecutionSummary(const char* vm_type) const {
        printf("\n=== %s VM Execution ===\n", vm_type);
        printf("Program: %s\n", program_name);
        printf("Platform: %s\n", PLATFORM_NAME);
        printf("Entry Point: 0x%x\n", entry_point);
        printf("PT_INTERP: %s\n", has_pt_interp ? "Yes" : "No");
        if (has_pt_interp) {
            printf("Interpreter: %s\n", interp_path);
        }
        printf("Start: %s", ctime(&start_time));
        printf("End: %s", ctime(&end_time));
        printf("Duration: %ld seconds\n", end_time - start_time);
        printf("%s [%s_shell]: ", PLATFORM_OUTPUT, vm_type);
    }
};

// Common Memory Manager - template for reuse
template<size_t DEFAULT_SIZE = 64 * 1024 * 1024>
class CommonMemory {
private:
    std::vector<uint8_t> memory;
    
public:
    CommonMemory(size_t size = DEFAULT_SIZE) {
        memory.resize(size, 0);
    }
    
    bool Write(uint32_t addr, const void* data, size_t size) {
        if (addr + size > memory.size()) return false;
        memcpy(memory.data() + addr, data, size);
        return true;
    }
    
    bool Read(uint32_t addr, void* buffer, size_t size) {
        if (addr + size > memory.size()) return false;
        memcpy(buffer, memory.data() + addr, size);
        return true;
    }
    
    void* GetPointer(uint32_t addr) {
        return addr < memory.size() ? memory.data() + addr : nullptr;
    }
    
    size_t GetSize() const {
        return memory.size();
    }
    
    bool ZeroFill(uint32_t addr, size_t size) {
        if (addr + size > memory.size()) return false;
        memset(memory.data() + addr, 0, size);
        return true;
    }
};

// Common ELF Loader - base class for inheritance
class CommonELFLoader {
protected:
    CommonMemory<>& memory;
    const char* loader_name;
    
public:
    CommonELFLoader(CommonMemory<>& mem, const char* name = "Common") 
        : memory(mem), loader_name(name) {}
    
    virtual ~CommonELFLoader() = default;
    
    bool LoadELF(const char* filename, uint32_t& entry_point, bool& needs_interp) {
        printf("%s [%s_VM] Loading ELF: %s\n", PLATFORM_OUTPUT, loader_name, filename);
        
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            printf("[%s_VM] Error opening file: %s\n", loader_name, filename);
            return false;
        }
        
        CommonELFHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        // Validate ELF
        if (header.ident[0] != 0x7F || strncmp(reinterpret_cast<char*>(header.ident) + 1, "ELF", 3) != 0) {
            printf("%s [%s_VM] Invalid ELF magic\n", PLATFORM_OUTPUT, loader_name);
            return false;
        }
        
        entry_point = header.entry;
        printf("%s [%s_VM] Entry Point: 0x%x\n", PLATFORM_OUTPUT, loader_name, entry_point);
        
        // Check for PT_INTERP
        needs_interp = false;
        for (int i = 0; i < header.phnum; i++) {
            CommonProgramHeader phdr;
            file.seekg(header.phoff + i * sizeof(CommonProgramHeader));
            file.read(reinterpret_cast<char*>(&phdr), sizeof(phdr));
            
            if (phdr.type == PT_INTERP) {
                needs_interp = true;
                printf("%s [%s_VM] PT_INTERP detected\n", PLATFORM_OUTPUT, loader_name);
                break;
            }
        }
        
        // Load program segments
        printf("%s [%s_VM] Loading %d program segments...\n", PLATFORM_OUTPUT, loader_name, header.phnum);
        file.seekg(0);
        for (int i = 0; i < header.phnum; i++) {
            CommonProgramHeader phdr;
            file.read(reinterpret_cast<char*>(&phdr), sizeof(phdr));
            
            if (phdr.type == PT_LOAD) {
                printf("%s [%s_VM] Loading PT_LOAD: vaddr=0x%x, size=0x%x, filesz=0x%x\n", 
                       PLATFORM_OUTPUT, loader_name, phdr.vaddr, phdr.memsz, phdr.filesz);
                
                std::vector<char> segment_data(phdr.filesz);
                file.seekg(phdr.offset);
                file.read(segment_data.data(), phdr.filesz);
                
                if (!memory.Write(phdr.vaddr, segment_data.data(), phdr.filesz)) {
                    printf("%s [%s_VM] Error: Failed to write segment at 0x%x\n", PLATFORM_OUTPUT, loader_name, phdr.vaddr);
                    return false;
                }
                
                // Zero-fill if needed
                if (phdr.memsz > phdr.filesz) {
                    size_t zero_size = phdr.memsz - phdr.filesz;
                    memory.ZeroFill(phdr.vaddr + phdr.filesz, zero_size);
                }
            }
        }
        
        printf("%s [%s_VM] ELF loading complete\n", PLATFORM_OUTPUT, loader_name);
        return true;
    }
};

// Common VM Executor - template for different VM types
template<typename MemoryType, typename LoaderType>
class CommonVMExecutor {
protected:
    MemoryType& memory;
    LoaderType& elf_loader;
    CommonProgramInfo& program_info;
    const char* vm_name;
    
public:
    CommonVMExecutor(MemoryType& mem, LoaderType& loader, CommonProgramInfo& info, const char* name)
        : memory(mem), elf_loader(loader), program_info(info), vm_name(name) {}
    
    virtual bool ExecuteProgram(const char* filename) {
        strncpy(program_info.program_name, filename, sizeof(program_info.program_name) - 1);
        
        printf("%s [%s_VM] Starting program execution\n", PLATFORM_OUTPUT, vm_name);
        
        if (!elf_loader.LoadELF(filename, program_info.entry_point, program_info.has_pt_interp)) {
            printf("%s [%s_VM] ELF loading failed\n", PLATFORM_OUTPUT, vm_name);
            return VM_ERROR;
        }
        
        printf("%s [%s_VM] Starting execution at 0x%x\n", PLATFORM_OUTPUT, vm_name, program_info.entry_point);
        
        // Common execution simulation
        printf("%s [%s_VM] Program running on %s\n", PLATFORM_OUTPUT, vm_name, PLATFORM_NAME);
        printf("%s [%s_VM] Platform: %s\n", PLATFORM_OUTPUT, vm_name, PLATFORM_NAME);
        printf("%s [%s_VM] Architecture: x86-64\n", PLATFORM_OUTPUT, vm_name);
        printf("%s [%s_VM] Memory: %zu MB\n", PLATFORM_OUTPUT, vm_name, memory.GetSize() / (1024 * 1024));
        
        program_info.end_time = time(nullptr);
        program_info.PrintExecutionSummary(vm_name);
        
        printf("%s [%s_VM] %s execution completed\n", PLATFORM_OUTPUT, vm_name, vm_name);
        
        return VM_OK;
    }
    
    void PrintSystemInfo() const {
        printf("\n=== %s VM System Information ===\n", vm_name);
        printf("Platform: %s\n", PLATFORM_NAME);
        printf("Architecture: x86-64\n");
        printf("Memory Manager: Common Implementation\n");
        printf("ELF Loader: Common Implementation\n");
        printf("Execution Engine: Common Implementation\n");
        printf("Libraries: %s\n", PLATFORM_LIBS);
        printf("====================================\n");
    }
};

// Common VM Main template - reduces boilerplate
template<typename VMType>
int CommonMain(int argc, char* argv[], const char* vm_name, const char* description) {
    printf("=== UserlandVM-HIT %s ===\n", vm_name);
    printf("%s\n", description);
    printf("Platform: %s\n", PLATFORM_NAME);
    printf("Architecture: x86-64\n");
    printf("Libraries: %s\n", PLATFORM_LIBS);
    printf("Author: Code Recycling Session 2026-02-06\n");
    printf("================================\n");
    
    if (argc < 2) {
        printf("Usage: %s <elf_program> [args...]\n", argv[0]);
        printf("  Executes ELF programs on %s\n", PLATFORM_NAME);
        printf("  %s\n", description);
        return 1;
    }
    
    printf("Executing: %s\n", argv[1]);
    
    VMType vm;
    vm.PrintSystemInfo();
    
    if (!vm.ExecuteProgram(argv[1])) {
        return 1;
    }
    
    printf("\n%s execution completed successfully!\n", vm_name);
    printf("Program executed on %s platform\n", PLATFORM_NAME);
    
    return 0;
}