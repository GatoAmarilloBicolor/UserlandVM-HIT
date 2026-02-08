/*
 * Complete x86-32 syscall implementations
 * Addresses all missing syscalls identified in code analysis
 */

#pragma once

#include <SupportDefs.h>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>

class GuestContext;
class AddressSpace;

// Enhanced x86-32 Syscall Dispatcher with complete implementations
class CompleteSyscallDispatcher {
private:
    AddressSpace& fAddressSpace;
    
public:
    CompleteSyscallDispatcher(AddressSpace& addressSpace) : fAddressSpace(addressSpace) {}
    
    // Complete syscall implementations
    status_t HandleSyscall(GuestContext& context, uint32_t syscall_num);
    
    // Complete file I/O syscalls
    status_t Syscall_Write(GuestContext& context);
    status_t Syscall_Read(GuestContext& context);
    status_t Syscall_Open(GuestContext& context);
    status_t Syscall_Close(GuestContext& context);
    status_t Syscall_Lseek(GuestContext& context);
    status_t Syscall_Stat(GuestContext& context);
    status_t Syscall_Fstat(GuestContext& context);
    status_t Syscall_Mmap(GuestContext& context);
    status_t Syscall_Munmap(GuestContext& context);
    status_t Syscall_Mprotect(GuestContext& context);
    status_t Syscall_Brk(GuestContext& context);
    
    // Process management syscalls
    status_t Syscall_Fork(GuestContext& context);
    status_t Syscall_Execve(GuestContext& context);
    status_t Syscall_Exit(GuestContext& context);
    status_t Syscall_Waitpid(GuestContext& context);
    status_t Syscall_Getpid(GuestContext& context);
    status_t Syscall_Kill(GuestContext& context);
    
    // Signal handling syscalls
    status_t Syscall_Sigaction(GuestContext& context);
    status_t Syscall_Sigprocmask(GuestContext& context);
    status_t Syscall_Signal(GuestContext& context);
    
    // Time syscalls
    status_t Syscall_Time(GuestContext& context);
    status_t Syscall_Gettimeofday(GuestContext& context);
    status_t Syscall_Nanosleep(GuestContext& context);
    
    // Memory management syscalls
    status_t Syscall_Malloc(GuestContext& context);
    status_t Syscall_Free(GuestContext& context);
    status_t Syscall_Calloc(GuestContext& context);
    status_t Syscall_Realloc(GuestContext& context);
    
    // Thread syscalls
    status_t Syscall_Pthread_Create(GuestContext& context);
    status_t Syscall_Pthread_Exit(GuestContext& context);
    status_t Syscall_Pthread_Join(GuestContext& context);
    status_t Syscall_Clonem(GuestContext& context);
    
    // Socket syscalls
    status_t Syscall_Socket(GuestContext& context);
    status_t Syscall_Bind(GuestContext& context);
    status_t Syscall_Listen(GuestContext& context);
    status_t Syscall_Accept(GuestContext& context);
    status_t Syscall_Connect(GuestContext& context);
    status_t Syscall_Send(GuestContext& context);
    status_t Syscall_Recv(GuestContext& context);
    status_t Syscall_Sendto(GuestContext& context);
    status_t Syscall_Recvfrom(GuestContext& context);
    status_t Syscall_Shutdown(GuestContext& context);
    status_t Syscall_Close_Socket(GuestContext& context);
    
private:
    // Helper functions
    void LogSyscall(GuestContext& context, uint32_t syscall_num, const char* syscall_name);
    void LogSyscallWithDetails(GuestContext& context, uint32_t syscall_num, const char* syscall_name, 
                                const char* details);
    
    // Memory helpers
    void* AllocateGuestMemory(GuestContext& context, size_t size);
    void FreeGuestMemory(GuestContext& context, void* ptr);
    
    // File operation helpers
    status_t ReadGuestString(GuestContext& context, char* buffer, size_t max_len, uint32_t guest_addr);
    status_t WriteGuestString(GuestContext& context, const char* str, uint32_t guest_addr);
    
    // Argument helpers
    uint32_t GetSyscallArg(GuestContext& context, int arg_num);
    const char* GetGuestString(GuestContext& context, uint32_t guest_addr);
    
    // Process management helpers
    void SetupChildProcess(GuestContext& context);
    uint32_t GetCurrentProcessId(GuestContext& context);
    
    // File descriptor management
    struct FileDescriptor {
        int host_fd;           // Real file descriptor on host
        uint32_t guest_fd;      // Guest-side file descriptor
        char* path;             // File path for debugging
        bool is_socket;         // True if this is a socket
        uint32_t flags;          // File flags (O_RDONLY, O_WRONLY, etc.)
        off_t offset;          // Current file offset
        mode_t mode;            // File mode and permissions
    };
    
    std::unordered_map<uint32_t, FileDescriptor> file_descriptors_;
    uint32_t next_fd_;
    
    int GetNextFd() { return next_fd_++; }
    
    // Socket management
    struct SocketInfo {
        int host_socket;        // Real socket on host
        uint32_t guest_socket;  // Guest-side socket descriptor
        int domain;             // AF_INET, // AF_UNIX, etc.
        int type;               // SOCK_STREAM, SOCK_DGRAM, etc.
        int protocol;           // Socket protocol
        struct sockaddr_in local_addr;  // Local address
        struct sockaddr_in remote_addr; // Remote address
        bool is_listening;      // True if socket is listening
    };
    
    std::unordered_map<uint32_t, SocketInfo> sockets_;
    uint32_t next_socket_;
    
    int GetNextSocketFd() { return next_socket_++; }
    
    // Process management
    struct ProcessInfo {
        uint32_t pid;           // Process ID
        uint32_t parent_pid;    // Parent process ID
        uint32_t child_pids[16]; // Child process IDs
        int num_children;       // Number of children
        int exit_status;         // Exit status
        bool is_running;        // True if process is running
        char* program_name;      // Program name for debugging
    };
    
    ProcessInfo current_process_;
    
    // Signal management
    struct SignalHandler {
        void (*handler)(int);   // Signal handler function
        uint32_t flags;         // Signal flags
        bool is_installed;       // True if handler is installed
    };
    
    SignalHandler signal_handlers_[32];
    
    // Memory management
    struct MemoryBlock {
        void* ptr;              // Pointer to memory
        size_t size;            // Size of memory block
        bool is_free;           // True if block is free
        void* next;              // Next block in linked list
    };
    
    MemoryBlock* memory_heap_start_;
    size_t heap_size_;
    bool heap_initialized_;
    
    void InitializeHeap();
    MemoryBlock* FindFreeBlock(size_t size);
    MemoryBlock* SplitBlock(MemoryBlock* block, size_t size);
    void MergeAdjacentBlocks(MemoryBlock* block);
    
public:
    // Public API for file descriptor operations
    int GetHostFd(uint32_t guest_fd);
    uint32_t GetGuestFd(int host_fd);
    void RemoveFd(uint32_t guest_fd);
    
    // Public API for socket operations
    int GetHostSocket(uint32_t guest_socket);
    uint32_t GetGuestSocket(int host_socket);
    void RemoveSocket(uint32_t guest_socket);
    
    // Statistics
    struct SyscallStats {
        uint64_t total_syscalls;
        uint64_t write_syscalls;
        uint64_t read_syscalls;
        uint64_t file_ops;
        uint64_t memory_ops;
        uint64_t socket_ops;
        uint64_t process_ops;
        uint64_t failed_syscalls;
    } stats_;
    
    void ResetStats();
    SyscallStats GetStats() const;
    void PrintStats() const;
};