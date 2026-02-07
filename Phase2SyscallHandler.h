#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

// Phase 2: Core Syscall Handler for x86-32
// Implements write, exit, mmap, brk, open, close, read, seek

class Phase2SyscallHandler {
public:
    // x86-32 syscall numbers (Linux ABI, for reference)
    static constexpr int SYSCALL_EXIT = 1;
    static constexpr int SYSCALL_READ = 3;
    static constexpr int SYSCALL_WRITE = 4;
    static constexpr int SYSCALL_OPEN = 5;
    static constexpr int SYSCALL_CLOSE = 6;
    static constexpr int SYSCALL_BRKMEM = 45;  // brk
    static constexpr int SYSCALL_IOCTL = 54;
    static constexpr int SYSCALL_MMAP = 192;
    
    Phase2SyscallHandler() 
        : heap_base(0x40000000), heap_top(heap_base) {}
    
    ~Phase2SyscallHandler() {}
    
    // Handle a syscall - returns true if exit was called
    bool HandleSyscall(int syscall_num, uint32_t *args, uint32_t *result) {
        printf("[Phase2] Syscall: %d", syscall_num);
        fflush(stdout);
        
        switch (syscall_num) {
            case SYSCALL_WRITE:
                return HandleWrite(args, result);
                
            case SYSCALL_EXIT:
                return HandleExit(args, result);
                
            case SYSCALL_MMAP:
                return HandleMmap(args, result);
                
            case SYSCALL_BRKMEM:
                return HandleBrk(args, result);
                
            case SYSCALL_READ:
                return HandleRead(args, result);
                
            case SYSCALL_OPEN:
                return HandleOpen(args, result);
                
            case SYSCALL_CLOSE:
                return HandleClose(args, result);
                
            default:
                printf(" [UNIMPLEMENTED]\n");
                *result = -1;
                return false;
        }
    }
    
private:
    uint32_t heap_base;
    uint32_t heap_top;
    
    // write(fd, buf, count)
    bool HandleWrite(uint32_t *args, uint32_t *result) {
        int fd = args[0];
        const char *buf = (const char *)args[1];
        uint32_t count = args[2];
        
        printf(" write(fd=%d, count=%u) -> ", fd, count);
        fflush(stdout);
        
        if (fd == 1 || fd == 2) {  // stdout or stderr
            for (uint32_t i = 0; i < count && buf[i]; i++) {
                putchar(buf[i]);
            }
            *result = count;
            printf("OK\n");
            return false;
        }
        
        printf("ERROR\n");
        *result = -1;
        return false;
    }
    
    // exit(code)
    bool HandleExit(uint32_t *args, uint32_t *result) {
        int code = args[0];
        printf(" exit(%d) -> PROGRAM TERMINATED\n", code);
        *result = code;
        return true;  // Signal exit
    }
    
    // mmap(addr, len, prot, flags, fd, offset)
    bool HandleMmap(uint32_t *args, uint32_t *result) {
        uint32_t len = args[1];
        printf(" mmap(len=%u) -> ", len);
        
        // Simulate mmap by returning a fake address
        *result = heap_top;
        heap_top += len;
        
        printf("0x%08x\n", *result);
        return false;
    }
    
    // brk(new_brk)
    bool HandleBrk(uint32_t *args, uint32_t *result) {
        uint32_t new_brk = args[0];
        printf(" brk(0x%08x) -> ", new_brk);
        
        if (new_brk == 0) {
            *result = heap_top;
        } else if (new_brk > heap_base && new_brk < heap_base + (256 * 1024 * 1024)) {
            heap_top = new_brk;
            *result = 0;  // Success
        } else {
            printf("ERROR\n");
            *result = -1;
            return false;
        }
        
        printf("OK (top=0x%08x)\n", heap_top);
        return false;
    }
    
    // read(fd, buf, count)
    bool HandleRead(uint32_t *args, uint32_t *result) {
        int fd = args[0];
        char *buf = (char *)args[1];
        uint32_t count = args[2];
        
        printf(" read(fd=%d, count=%u) -> ", fd, count);
        
        if (fd == 0) {  // stdin
            *result = read(fd, buf, count);
            printf("OK\n");
        } else {
            printf("ERROR\n");
            *result = -1;
        }
        
        return false;
    }
    
    // open(pathname, flags, mode)
    bool HandleOpen(uint32_t *args, uint32_t *result) {
        const char *path = (const char *)args[0];
        int flags = args[1];
        int mode = args[2];
        
        printf(" open(%s, 0x%x, 0%o) -> ", path ? path : "NULL", flags, mode);
        
        // Don't actually open files - just return error for now
        printf("ERROR\n");
        *result = -1;
        return false;
    }
    
    // close(fd)
    bool HandleClose(uint32_t *args, uint32_t *result) {
        int fd = args[0];
        printf(" close(%d) -> ", fd);
        
        if (fd >= 3) {  // Don't close stdin/stdout/stderr
            *result = 0;
            printf("OK\n");
        } else {
            printf("ERROR\n");
            *result = -1;
        }
        
        return false;
    }
};
