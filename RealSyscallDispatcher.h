#pragma once

#include "SyscallDispatcher.h"
#include "GuestContext.h"
#include "PlatformTypes.h"
#include "Phase4GUISyscalls.h"
#include "X86_32GuestContext.h"
#include "RecycledBasicSyscalls.h"
#include <cstdio>
#include <cstdlib>
#include <memory>

// Real implementation of SyscallDispatcher for x86-32 syscalls
class RealSyscallDispatcher : public SyscallDispatcher {
public:
    RealSyscallDispatcher() 
        : exit_code(0), should_exit(false),
          gui_handler(std::make_unique<Phase4GUISyscallHandler>()) {}
    
    virtual ~RealSyscallDispatcher() = default;
    
    virtual status_t Dispatch(GuestContext& context) override {
        // For x86-32, syscall number is in EAX
        // This is called after INT 0x80
        X86_32GuestContext& x86_context = dynamic_cast<X86_32GuestContext&>(context);
        X86_32Registers& regs = x86_context.Registers();
        
        int syscall_num = regs.eax;
        printf("[Syscall] Dispatching syscall %d\n", syscall_num);
        printf("[Syscall]   EAX=%u (syscall), EBX=%u (arg0), ECX=%u (arg1), EDX=%u (arg2)\n",
               regs.eax, regs.ebx, regs.ecx, regs.edx);
        
        // Call recycled syscall dispatcher
        int result = RecycledBasicSyscallDispatcher::DispatchSyscall(syscall_num, regs.ebx, regs.ecx, regs.edx);
        regs.eax = result;
        
        printf("[Syscall] Syscall %d returned: %d\n", syscall_num, result);
        return B_OK;
    }
    
    virtual void DispatchLegacy(GuestContext& context) override {
        // Get syscall number from context
        // This depends on X86_32GuestContext structure
        printf("[Syscall] DispatchLegacy called (not fully implemented)\n");
    }
    
    // Handle GUI syscalls
    bool HandleGUISyscall(int syscall_num, uint32_t *args, uint32_t *result) {
        if (gui_handler) {
            return gui_handler->HandleGUISyscall(syscall_num, args, result);
        }
        return false;
    }
    
    // Get GUI handler for window info
    Phase4GUISyscallHandler *GetGUIHandler() {
        return gui_handler.get();
    }
    
    bool ShouldExit() const { return should_exit; }
    int GetExitCode() const { return exit_code; }
    
private:
    int exit_code;
    bool should_exit;
    std::unique_ptr<Phase4GUISyscallHandler> gui_handler;
};
