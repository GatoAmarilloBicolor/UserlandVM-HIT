/*
 * SimpleSyscallDispatcher.cpp - Simplified Syscall Dispatcher Implementation
 */

#include "SimpleSyscallDispatcher.h"
#include "X86_32GuestContext.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

SimpleSyscallDispatcher::SimpleSyscallDispatcher(AddressSpace& addressSpace)
    : fAddressSpace(addressSpace), next_fd_(3) {
    memset(file_descriptors_, 0, sizeof(file_descriptors_));
    current_process_ = {1000, 0, true};
    memset(&stats_, 0, sizeof(stats_));
}

SimpleSyscallDispatcher::~SimpleSyscallDispatcher() {
}

void SimpleSyscallDispatcher::LogSyscall(GuestContext& context, uint32_t syscall_num, const char* name) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    printf("[SYSCALL] %s(%u) EBX=0x%08x ECX=0x%08x EDX=0x%08x\n", 
           name, syscall_num, regs.ebx, regs.ecx, regs.edx);
}

uint32_t SimpleSyscallDispatcher::GetArgument(GuestContext& context, int arg_num) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    switch(arg_num) {
        case 0: return regs.ebx;
        case 1: return regs.ecx;
        case 2: return regs.edx;
        case 3: return regs.esi;
        case 4: return regs.edi;
        case 5: return regs.ebp;
        default: return 0;
    }
}

const char* SimpleSyscallDispatcher::GetString(GuestContext& context, uint32_t addr) {
    static char buffer[256];
    if (fAddressSpace.Read(addr, buffer, sizeof(buffer) - 1) == B_OK) {
        buffer[sizeof(buffer) - 1] = '\0';
        return buffer;
    }
    return "";
}

int SimpleSyscallDispatcher::GetHostFd(uint32_t guest_fd) {
    for (int i = 0; i < 32; i++) {
        if (file_descriptors_[i].fd == (int)guest_fd) {
            return file_descriptors_[i].host_fd;
        }
    }
    return -1;
}

void SimpleSyscallDispatcher::RemoveFd(uint32_t guest_fd) {
    for (int i = 0; i < 32; i++) {
        if (file_descriptors_[i].fd == (int)guest_fd) {
            file_descriptors_[i].fd = -1;
            break;
        }
    }
}

int SimpleSyscallDispatcher::GetNextFd() {
    return next_fd_++;
}

status_t SimpleSyscallDispatcher::HandleSyscall(GuestContext& context, uint32_t syscall_num) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    stats_.total_syscalls++;
    
    switch (syscall_num) {
        case 1: // exit
            return Syscall_Exit(context);
        case 3: // read
            return Syscall_Read(context);
        case 4: // write
            return Syscall_Write(context);
        case 5: // open
            return Syscall_Open(context);
        case 6: // close
            return Syscall_Close(context);
        case 9: // mmap
            return Syscall_Mmap(context);
        case 10: // mprotect
            return Syscall_Mprotect(context);
        case 11: // munmap
            return Syscall_Munmap(context);
        case 12: // brk
            return Syscall_Brk(context);
        case 20: // getpid
            regs.eax = 1000;
            return B_OK;
        default:
            LogSyscall(context, syscall_num, "Unknown");
            regs.eax = 0;
            return B_OK;
    }
}

status_t SimpleSyscallDispatcher::Syscall_Exit(GuestContext& context) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    printf("[EXIT] Process exiting with code %d\n", regs.ebx);
    current_process_.exit_status = regs.ebx;
    current_process_.is_running = false;
    
    return B_ERROR;
}

status_t SimpleSyscallDispatcher::Syscall_Read(GuestContext& context) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    stats_.read_syscalls++;
    
    int host_fd = GetHostFd(regs.ebx);
    if (host_fd < 0) {
        regs.eax = -EBADF;
        return B_OK;
    }
    
    char buffer[4096];
    uint32_t count = regs.edx;
    if (count > sizeof(buffer)) count = sizeof(buffer);
    
    ssize_t bytes_read = read(host_fd, buffer, count);
    if (bytes_read > 0) {
        fAddressSpace.Write(regs.ecx, buffer, bytes_read);
    }
    regs.eax = bytes_read;
    
    return B_OK;
}

status_t SimpleSyscallDispatcher::Syscall_Write(GuestContext& context) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    stats_.write_syscalls++;
    
    int host_fd = GetHostFd(regs.ebx);
    if (host_fd < 0) {
        regs.eax = -EBADF;
        return B_OK;
    }
    
    char buffer[4096];
    uint32_t count = regs.edx;
    if (count > sizeof(buffer)) count = sizeof(buffer);
    
    fAddressSpace.Read(regs.ecx, buffer, count);
    ssize_t bytes_written = write(host_fd, buffer, count);
    regs.eax = bytes_written;
    
    return B_OK;
}

status_t SimpleSyscallDispatcher::Syscall_Open(GuestContext& context) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    const char* path = GetString(context, regs.ebx);
    int flags = regs.ecx;
    int mode = regs.edx;
    
    int host_fd = open(path, flags, mode);
    if (host_fd < 0) {
        regs.eax = -errno;
        return B_OK;
    }
    
    int guest_fd = GetNextFd();
    file_descriptors_[guest_fd % 32].fd = guest_fd;
    file_descriptors_[guest_fd % 32].host_fd = host_fd;
    regs.eax = guest_fd;
    
    return B_OK;
}

status_t SimpleSyscallDispatcher::Syscall_Close(GuestContext& context) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    int host_fd = GetHostFd(regs.ebx);
    if (host_fd >= 0) {
        close(host_fd);
        RemoveFd(regs.ebx);
    }
    regs.eax = 0;
    
    return B_OK;
}

status_t SimpleSyscallDispatcher::Syscall_Mmap(GuestContext& context) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    void* ptr = mmap((void*)regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi, regs.ebp);
    if (ptr == MAP_FAILED) {
        regs.eax = -ENOMEM;
    } else {
        regs.eax = (uint32_t)(uintptr_t)ptr;
    }
    
    return B_OK;
}

status_t SimpleSyscallDispatcher::Syscall_Mprotect(GuestContext& context) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    regs.eax = mprotect((void*)regs.ebx, regs.ecx, regs.edx);
    return B_OK;
}

status_t SimpleSyscallDispatcher::Syscall_Munmap(GuestContext& context) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    regs.eax = munmap((void*)regs.ebx, regs.ecx);
    return B_OK;
}

status_t SimpleSyscallDispatcher::Syscall_Brk(GuestContext& context) {
    X86_32GuestContext &x86_context = static_cast<X86_32GuestContext&>(context);
    X86_32Registers &regs = x86_context.Register();
    
    regs.eax = regs.ebx;
    return B_OK;
}

void SimpleSyscallDispatcher::PrintStats() const {
    printf("=== SYSCALL STATS ===\n");
    printf("Total: %lu\n", stats_.total_syscalls);
    printf("Read: %lu\n", stats_.read_syscalls);
    printf("Write: %lu\n", stats_.write_syscalls);
    printf("==================\n");
}
