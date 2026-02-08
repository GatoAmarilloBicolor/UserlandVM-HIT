#include "SimpleSyscallDispatcher.h"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <vector>

SimpleSyscallDispatcher::SimpleSyscallDispatcher(AddressSpace& addressSpace)
    : fAddressSpace(addressSpace), fHeapBase(0x08000000), fHeapCurrent(0x08000000) {
}

SimpleSyscallDispatcher::~SimpleSyscallDispatcher() {
    // Cleanup if needed
}

uint32_t SimpleSyscallDispatcher::HandleSyscall(uint32_t number, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    std::cout << "SYSCALL: " << number << "(" << arg1 << ", " << arg2 << ", " << arg3 << ")" << std::endl;
    
    switch (number) {
        case 1: // exit
            std::cout << "Process exiting with status: " << arg1 << std::endl;
            return 0;
            
        case 3: { // read
            uint32_t fd = arg1;
            uint32_t buffer = arg2;
            uint32_t count = arg3;
            
            std::cout << "READ: fd=" << fd << ", buffer=0x" << std::hex << buffer 
                     << ", count=" << std::dec << count << std::endl;
            
            if (fd == 0) { // stdin
                // For testing, return 0 (EOF)
                return 0;
            } else {
                return -EBADF;
            }
        }
        
        case 4: { // write
            uint32_t fd = arg1;
            uint32_t buffer = arg2;
            uint32_t count = arg3;
            
            std::cout << "WRITE: fd=" << fd << ", buffer=0x" << std::hex << buffer 
                     << ", count=" << std::dec << count << std::endl;
            
            if (fd == 1 || fd == 2) { // stdout or stderr
                // Read the message from guest memory
                std::vector<char> msg(count + 1, 0);
                if (fAddressSpace.Read(buffer, reinterpret_cast<uint8_t*>(msg.data()), count)) {
                    std::cout << "GUEST OUTPUT: " << msg.data() << std::endl;
                    return count;
                } else {
                    std::cout << "Failed to read from guest memory" << std::endl;
                    return -EFAULT;
                }
            } else {
                return -EBADF;
            }
        }
        
        case 45: { // brk
            uint32_t new_brk = arg1;
            
            std::cout << "BRK: new_brk=0x" << std::hex << new_brk << std::dec << std::endl;
            
            if (new_brk == 0) {
                // Return current break
                return fHeapCurrent;
            } else if (new_brk >= fHeapBase && new_brk < fHeapBase + 0x10000000) {
                // Valid break address
                fHeapCurrent = new_brk;
                return fHeapCurrent;
            } else {
                return -ENOMEM;
            }
        }
        
        case 20: { // getpid
            std::cout << "GETPID" << std::endl;
            return 1234;  // Fake process ID
        }
        
        case 90: { // mprotect
            uint32_t addr = arg1;
            uint32_t len = arg2;
            uint32_t prot = arg3;
            
            std::cout << "MPROTECT: addr=0x" << std::hex << addr << ", len=" << std::dec << len 
                     << ", prot=" << std::hex << prot << std::dec << std::endl;
            
            // For simplicity, always succeed
            return 0;
        }
        
        case 122: { // uname
            uint32_t buf = arg1;
            
            std::cout << "UNAME: buf=0x" << std::hex << buf << std::dec << std::endl;
            
            // For simplicity, return error
            return -ENOSYS;
        }
        
        case 125: { // mprotect (alternative)
            uint32_t addr = arg1;
            uint32_t len = arg2;
            uint32_t prot = arg3;
            
            std::cout << "MPROTECT(125): addr=0x" << std::hex << addr << ", len=" << std::dec << len 
                     << ", prot=" << std::hex << prot << std::dec << std::endl;
            
            return 0;
        }
        
        case 192: { // mmap2
            uint32_t addr = arg1;
            uint32_t length = arg2;
            uint32_t prot = arg3;
            uint32_t flags = arg4;
            uint32_t fd = arg5;
            uint32_t offset = arg6;
            
            std::cout << "MMAP2: addr=0x" << std::hex << addr << ", length=" << std::dec << length 
                     << ", prot=" << std::hex << prot << ", flags=" << flags 
                     << ", fd=" << std::dec << fd << ", offset=" << offset << std::endl;
            
            // For simplicity, return a fake address
            static uint32_t mmap_counter = 0x60000000;
            uint32_t result = mmap_counter;
            mmap_counter += 0x1000;
            
            return result;
        }
        
        case 195: { // stat64
            uint32_t filename = arg1;
            uint32_t statbuf = arg2;
            
            std::cout << "STAT64: filename=0x" << std::hex << filename 
                     << ", statbuf=0x" << statbuf << std::dec << std::endl;
            
            // For simplicity, return file not found
            return -ENOENT;
        }
        
        case 240: { // fcntl64
            uint32_t fd = arg1;
            uint32_t cmd = arg2;
            uint32_t arg = arg3;
            
            std::cout << "FCNTL64: fd=" << fd << ", cmd=" << cmd << ", arg=" << arg << std::endl;
            
            // For simplicity, always succeed
            return 0;
        }
        
        default:
            std::cout << "UNHANDLED SYSCALL: " << number << std::endl;
            return -ENOSYS;
    }
}

void SimpleSyscallDispatcher::LogRegisterState(const std::string& operation, uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    std::cout << "REGS [" << operation << "]: EAX=0x" << std::hex << eax 
             << ", EBX=0x" << ebx << ", ECX=0x" << ecx 
             << ", EDX=0x" << edx << std::dec << std::endl;
}

bool SimpleSyscallDispatcher::ValidateMemoryAccess(uint32_t address, uint32_t size) {
    // Simple validation - check if address is in reasonable range
    if (address < 0x08000000 || address > 0xC0000000) {
        return false;
    }
    if (address + size > 0xC0000000) {
        return false;
    }
    return true;
}

void SimpleSyscallDispatcher::SetProcessArgs(int argc, char* argv[], char* envp[]) {
    // Store process arguments for later use
    fArgc = argc;
    fArgv = argv;
    fEnvp = envp;
}

uint32_t SimpleSyscallDispatcher::GetProcessArgsSize() {
    if (!fArgv || fArgc == 0) {
        return 0;
    }
    
    uint32_t size = sizeof(uint32_t) * (fArgc + 1); // argv array
    
    for (int i = 0; i < fArgc; i++) {
        if (fArgv[i]) {
            size += strlen(fArgv[i]) + 1;
        }
    }
    
    return size;
}