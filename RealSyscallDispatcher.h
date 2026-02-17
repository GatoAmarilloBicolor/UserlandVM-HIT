/*
 * RealSyscallDispatcher.h - Real syscall dispatcher implementation
 * Handles actual Haiku syscalls
 */

#ifndef REAL_SYSCALL_DISPATCHER_H
#define REAL_SYSCALL_DISPATCHER_H

#include <cstdint>
#include <string>
#include <map>

namespace HaikuVM {

enum class SyscallCategory {
    FileSystem,
    Process,
    Memory,
    Thread,
    Network,
    Device,
    IPC,
    Other
};

struct SyscallInfo {
    uint32_t number;
    const char* name;
    SyscallCategory category;
};

class RealSyscallDispatcher {
public:
    RealSyscallDispatcher();
    ~RealSyscallDispatcher();
    
    void Initialize();
    void Shutdown();
    
    uint32_t GetSyscallNumber(const char* name);
    const char* GetSyscallName(uint32_t number);
    
    int32_t ExecuteSyscall(uint32_t syscall_number, void** args);
    bool HandleGUISyscall(uint32_t syscall_number, uint32_t* args, uint32_t* result);
    
    bool IsValidSyscall(uint32_t number) const;
    uint32_t GetSyscallCount() const { return syscall_count; }

private:
    void RegisterSyscalls();
    
    std::map<uint32_t, SyscallInfo> syscalls;
    std::map<std::string, uint32_t> syscall_map;
    uint32_t syscall_count;
    bool initialized;
};

} // namespace HaikuVM

#endif // REAL_SYSCALL_DISPATCHER_H
