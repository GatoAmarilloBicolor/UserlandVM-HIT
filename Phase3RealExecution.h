#pragma once

#include "PlatformTypes.h"
#include "AddressSpace.h"
#include "SyscallDispatcher.h"
#include "GuestContext.h"
#include "InterpreterX86_32.h"
#include <cstdio>

// Wrapper para usar el InterpreterX86_32 real compilado

class Phase3RealExecutor {
private:
    // Stub AddressSpace que se adapta a nuestro modelo
    class StubAddressSpace : public AddressSpace {
    private:
        void *base;
        size_t size;
        
    public:
        StubAddressSpace(void *b, size_t s) : base(b), size(s) {}
        
        virtual ~StubAddressSpace() {}
        
        virtual status_t Read(uint32_t addr, void *buf, size_t len) override {
            if (addr + len > size) return B_BAD_VALUE;
            memcpy(buf, (uint8_t *)base + addr, len);
            return B_OK;
        }
        
        virtual status_t Write(uint32_t addr, const void *buf, size_t len) override {
            if (addr + len > size) return B_BAD_VALUE;
            memcpy((uint8_t *)base + addr, buf, len);
            return B_OK;
        }
        
        virtual void *GetPointer(uint32_t addr) override {
            if (addr >= size) return nullptr;
            return (uint8_t *)base + addr;
        }
    };
    
    // Stub SyscallDispatcher
    class StubSyscallDispatcher : public SyscallDispatcher {
    public:
        virtual ~StubSyscallDispatcher() = default;
        
        virtual status_t Dispatch(GuestContext& context) override {
            printf("[Syscall] eax=%u\n", context.regs[0]);  // eax
            return B_OK;
        }
        
        virtual void DispatchLegacy(GuestContext& context) override {
            Dispatch(context);
        }
    };
    
public:
    Phase3RealExecutor(void *image_base, size_t image_size)
        : addr_space(image_base, image_size),
          dispatcher() {}
    
    bool Execute(uint32_t entry_point) {
        printf("[Phase3Real] Starting real x86 interpreter\n");
        printf("[Phase3Real] Entry point: 0x%08x\n", entry_point);
        
        try {
            // Create guest context
            GuestContext ctx;
            ctx.regs[7] = entry_point;  // Set entry to eip
            
            // Create interpreter
            InterpreterX86_32 interpreter(addr_space, dispatcher);
            
            // Execute
            printf("[Phase3Real] Executing...\n");
            status_t result = interpreter.Run(ctx);
            
            printf("[Phase3Real] Execution completed with status: %d\n", result);
            return result == B_OK;
        }
        catch (const std::exception &e) {
            printf("[Phase3Real] Exception: %s\n", e.what());
            return false;
        }
        catch (...) {
            printf("[Phase3Real] Unknown exception\n");
            return false;
        }
    }
    
private:
    StubAddressSpace addr_space;
    StubSyscallDispatcher dispatcher;
};
