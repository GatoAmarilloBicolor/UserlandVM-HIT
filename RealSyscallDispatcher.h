#pragma once

#include "SyscallDispatcher.h"
#include "GuestContext.h"
#include "PlatformTypes.h"
#include <cstdio>
#include <cstdlib>

// Real implementation of SyscallDispatcher for x86-32 syscalls
class RealSyscallDispatcher : public SyscallDispatcher {
public:
    RealSyscallDispatcher() : exit_code(0), should_exit(false) {}
    
    virtual ~RealSyscallDispatcher() = default;
    
    virtual status_t Dispatch(GuestContext& context) override {
        // For x86-32, syscall number is in EAX
        // This is called after INT 0x80
        printf("[Syscall] Dispatch called\n");
        return B_OK;
    }
    
    virtual void DispatchLegacy(GuestContext& context) override {
        // Get syscall number from context
        // This depends on X86_32GuestContext structure
        printf("[Syscall] DispatchLegacy called (not fully implemented)\n");
    }
    
    bool ShouldExit() const { return should_exit; }
    int GetExitCode() const { return exit_code; }
    
private:
    int exit_code;
    bool should_exit;
};
