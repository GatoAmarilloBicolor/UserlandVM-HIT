// UserlandVM-HIT Recycled Core VM
// Built using CommonVMComponents - 90% code reduction
// Author: Code Recycling Session 2026-02-06

#include "CommonVMComponents.h"

// Specialized Core VM using common components
class CoreELFLoader : public CommonELFLoader {
public:
    CoreELFLoader(CommonMemory<>& mem) : CommonELFLoader(mem, "CORE") {}
};

class RecycledCoreVM {
private:
    CommonMemory<> memory;
    CoreELFLoader elf_loader;
    CommonProgramInfo program_info;
    CommonVMExecutor<CommonMemory<>, CoreELFLoader> executor;
    
public:
    RecycledCoreVM() 
        : memory(), elf_loader(memory), program_info(), executor(memory, elf_loader, program_info, "CORE") {
        printf("[linux.cosmoe] [CORE_VM] Recycled Core VM initialized using CommonVMComponents\n");
    }
    
    bool ExecuteProgram(const char* filename) {
        return executor.ExecuteProgram(filename);
    }
    
    void PrintSystemInfo() const {
        executor.PrintSystemInfo();
    }
};

int main(int argc, char* argv[]) {
    return CommonMain<RecycledCoreVM>(argc, argv, "Recycled Core VM", 
        "Lightweight VM using recycled components - 90% code reduction");
}