#pragma once

#include "PlatformTypes.h"
#include <cstring>
#include <cstdio>
#include <map>

// Minimal AddressSpace for Guest execution
class GuestAddressSpace {
private:
    uint8_t *memory;
    size_t size;
    
public:
    GuestAddressSpace(void *base, size_t sz) 
        : memory((uint8_t *)base), size(sz) {}
    
    ~GuestAddressSpace() {}
    
    bool Read(uint32_t addr, void *buf, size_t len) {
        if (addr + len > size) return false;
        memcpy(buf, memory + addr, len);
        return true;
    }
    
    bool Write(uint32_t addr, const void *buf, size_t len) {
        if (addr + len > size) return false;
        memcpy(memory + addr, buf, len);
        return true;
    }
    
    uint32_t ReadU32(uint32_t addr) {
        if (addr + 4 > size) return 0;
        return *(uint32_t *)(memory + addr);
    }
    
    void WriteU32(uint32_t addr, uint32_t val) {
        if (addr + 4 <= size) {
            *(uint32_t *)(memory + addr) = val;
        }
    }
    
    void *GetPointer(uint32_t addr) {
        if (addr >= size) return nullptr;
        return memory + addr;
    }
};

// Minimal GuestContext for x86-32
struct GuestContext {
    // x86 registers
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    
    // TLS/FS base (for __thread support)
    uint32_t fs_base;
    uint32_t gs_base;
    
    // State
    bool halted;
    int exit_code;
    
    GuestContext() : 
        eax(0), ebx(0), ecx(0), edx(0),
        esi(0), edi(0), ebp(0), esp(0x30000000),
        eip(0), eflags(0x202),
        fs_base(0), gs_base(0),
        halted(false), exit_code(0) {}
};

// Minimal SyscallDispatcher for x86-32
class GuestSyscallDispatcher {
public:
    GuestSyscallDispatcher() : heap_top(0x40000000) {}
    
    ~GuestSyscallDispatcher() {}
    
    // Handle INT 0x80 syscalls
    bool HandleSyscall(GuestContext &ctx) {
        uint32_t syscall_num = ctx.eax;
        
        printf("[Syscall] %u (", syscall_num);
        fflush(stdout);
        
        switch (syscall_num) {
            case 1:  // exit
                printf("exit(%u))\n", ctx.ebx);
                ctx.halted = true;
                ctx.exit_code = ctx.ebx;
                return true;
                
            case 4:  // write
            {
                int fd = ctx.ebx;
                uint32_t buf_addr = ctx.ecx;
                uint32_t count = ctx.edx;
                printf("write(%d, 0x%x, %u))\n", fd, buf_addr, count);
                ctx.eax = count;  // Return bytes written
                return false;
            }
                
            case 45:  // brk
            {
                uint32_t new_brk = ctx.ebx;
                printf("brk(0x%x))\n", new_brk);
                if (new_brk == 0) {
                    ctx.eax = heap_top;
                } else {
                    heap_top = new_brk;
                    ctx.eax = 0;
                }
                return false;
            }
                
            case 192:  // mmap
            {
                uint32_t len = ctx.ecx;  // Simplified - normally in stack
                printf("mmap(*len=%u))\n", len);
                ctx.eax = heap_top;
                heap_top += len;
                return false;
            }
                
            default:
                printf("UNIMPL=%u))\n", syscall_num);
                ctx.eax = -1;
                return false;
        }
    }
    
private:
    uint32_t heap_top;
};
