/*
 * HaikuSyscallDispatcher.h - Haiku-Specific System Call Dispatcher
 * 
 * Handles all Haiku/BeOS system calls and routes them to appropriate kits
 * This is the core syscall interception layer for Haiku API virtualization
 */

#pragma once

#include "SyscallDispatcher.h"
#include "HaikuAPIVirtualizer.h"
#include <cstdint>
#include <map>
#include <string>
#include <functional>

// Haiku-specific syscall numbers
#define HAIKU_SYSCALL_BASE           0x1000
#define HAIKU_SYSCALL_INTERFACE_KIT   (HAIKU_SYSCALL_BASE + 0x100)
#define HAIKU_SYSCALL_STORAGE_KIT     (HAIKU_SYSCALL_BASE + 0x200)
#define HAIKU_SYSCALL_APPLICATION_KIT (HAIKU_SYSCALL_BASE + 0x300)
#define HAIKU_SYSCALL_SUPPORT_KIT     (HAIKU_SYSCALL_BASE + 0x400)
#define HAIKU_SYSCALL_NETWORK_KIT     (HAIKU_SYSCALL_BASE + 0x500)
#define HAIKU_SYSCALL_MEDIA_KIT       (HAIKU_SYSCALL_BASE + 0x600)

// Haiku syscalls - Interface Kit
#define HAIKU_SYSCALL_CREATE_WINDOW        (HAIKU_SYSCALL_INTERFACE_KIT + 1)
#define HAIKU_SYSCALL_SHOW_WINDOW          (HAIKU_SYSCALL_INTERFACE_KIT + 2)
#define HAIKU_SYSCALL_HIDE_WINDOW          (HAIKU_SYSCALL_INTERFACE_KIT + 3)
#define HAIKU_SYSCALL_DESTROY_WINDOW       (HAIKU_SYSCALL_INTERFACE_KIT + 4)
#define HAIKU_SYSCALL_DRAW_LINE            (HAIKU_SYSCALL_INTERFACE_KIT + 5)
#define HAIKU_SYSCALL_FILL_RECT            (HAIKU_SYSCALL_INTERFACE_KIT + 6)
#define HAIKU_SYSCALL_DRAW_STRING          (HAIKU_SYSCALL_INTERFACE_KIT + 7)
#define HAIKU_SYSCALL_FLUSH                (HAIKU_SYSCALL_INTERFACE_KIT + 8)
#define HAIKU_SYSCALL_ADD_CHILD            (HAIKU_SYSCALL_INTERFACE_KIT + 9)
#define HAIKU_SYSCALL_REMOVE_CHILD         (HAIKU_SYSCALL_INTERFACE_KIT + 10)

// Haiku syscalls - Storage Kit
#define HAIKU_SYSCALL_OPEN_FILE            (HAIKU_SYSCALL_STORAGE_KIT + 1)
#define HAIKU_SYSCALL_CLOSE_FILE           (HAIKU_SYSCALL_STORAGE_KIT + 2)
#define HAIKU_SYSCALL_READ_FILE            (HAIKU_SYSCALL_STORAGE_KIT + 3)
#define HAIKU_SYSCALL_WRITE_FILE           (HAIKU_SYSCALL_STORAGE_KIT + 4)
#define HAIKU_SYSCALL_SEEK_FILE            (HAIKU_SYSCALL_STORAGE_KIT + 5)
#define HAIKU_SYSCALL_SET_FILE_SIZE        (HAIKU_SYSCALL_STORAGE_KIT + 6)
#define HAIKU_SYSCALL_OPEN_DIRECTORY       (HAIKU_SYSCALL_STORAGE_KIT + 7)
#define HAIKU_SYSCALL_CLOSE_DIRECTORY      (HAIKU_SYSCALL_STORAGE_KIT + 8)
#define HAIKU_SYSCALL_READ_DIRECTORY       (HAIKU_SYSCALL_STORAGE_KIT + 9)
#define HAIKU_SYSCALL_REWIND_DIRECTORY     (HAIKU_SYSCALL_STORAGE_KIT + 10)
#define HAIKU_SYSCALL_GET_ENTRY_INFO       (HAIKU_SYSCALL_STORAGE_KIT + 11)
#define HAIKU_SYSCALL_CREATE_ENTRY         (HAIKU_SYSCALL_STORAGE_KIT + 12)
#define HAIKU_SYSCALL_DELETE_ENTRY         (HAIKU_SYSCALL_STORAGE_KIT + 13)
#define HAIKU_SYSCALL_MOVE_ENTRY           (HAIKU_SYSCALL_STORAGE_KIT + 14)

// Haiku syscalls - Application Kit
#define HAIKU_SYSCALL_CREATE_APPLICATION    (HAIKU_SYSCALL_APPLICATION_KIT + 1)
#define HAIKU_SYSCALL_RUN_APPLICATION       (HAIKU_SYSCALL_APPLICATION_KIT + 2)
#define HAIKU_SYSCALL_QUIT_APPLICATION     (HAIKU_SYSCALL_APPLICATION_KIT + 3)
#define HAIKU_SYSCALL_CREATE_MESSAGE        (HAIKU_SYSCALL_APPLICATION_KIT + 4)
#define HAIKU_SYSCALL_SEND_MESSAGE         (HAIKU_SYSCALL_APPLICATION_KIT + 5)
#define HAIKU_SYSCALL_POST_MESSAGE         (HAIKU_SYSCALL_APPLICATION_KIT + 6)

// Haiku syscalls - Support Kit
#define HAIKU_SYSCALL_CREATE_STRING        (HAIKU_SYSCALL_SUPPORT_KIT + 1)
#define HAIKU_SYSCALL_SET_STRING           (HAIKU_SYSCALL_SUPPORT_KIT + 2)
#define HAIKU_SYSCALL_GET_STRING           (HAIKU_SYSCALL_SUPPORT_KIT + 3)
#define HAIKU_SYSCALL_DELETE_STRING        (HAIKU_SYSCALL_SUPPORT_KIT + 4)
#define HAIKU_SYSCALL_CREATE_LIST          (HAIKU_SYSCALL_SUPPORT_KIT + 5)
#define HAIKU_SYSCALL_ADD_ITEM             (HAIKU_SYSCALL_SUPPORT_KIT + 6)
#define HAIKU_SYSCALL_REMOVE_ITEM          (HAIKU_SYSCALL_SUPPORT_KIT + 7)
#define HAIKU_SYSCALL_GET_ITEM             (HAIKU_SYSCALL_SUPPORT_KIT + 8)
#define HAIKU_SYSCALL_COUNT_ITEMS          (HAIKU_SYSCALL_SUPPORT_KIT + 9)

// Haiku syscalls - Network Kit
#define HAIKU_SYSCALL_CREATE_SOCKET        (HAIKU_SYSCALL_NETWORK_KIT + 1)
#define HAIKU_SYSCALL_CONNECT_SOCKET       (HAIKU_SYSCALL_NETWORK_KIT + 2)
#define HAIKU_SYSCALL_BIND_SOCKET          (HAIKU_SYSCALL_NETWORK_KIT + 3)
#define HAIKU_SYSCALL_LISTEN_SOCKET       (HAIKU_SYSCALL_NETWORK_KIT + 4)
#define HAIKU_SYSCALL_ACCEPT_SOCKET        (HAIKU_SYSCALL_NETWORK_KIT + 5)
#define HAIKU_SYSCALL_CLOSE_SOCKET        (HAIKU_SYSCALL_NETWORK_KIT + 6)
#define HAIKU_SYSCALL_SEND_SOCKET          (HAIKU_SYSCALL_NETWORK_KIT + 7)
#define HAIKU_SYSCALL_RECEIVE_SOCKET       (HAIKU_SYSCALL_NETWORK_KIT + 8)

// Haiku syscalls - Media Kit
#define HAIKU_SYSCALL_CREATE_SOUND_PLAYER  (HAIKU_SYSCALL_MEDIA_KIT + 1)
#define HAIKU_SYSCALL_START_SOUND_PLAYER   (HAIKU_SYSCALL_MEDIA_KIT + 2)
#define HAIKU_SYSCALL_STOP_SOUND_PLAYER    (HAIKU_SYSCALL_MEDIA_KIT + 3)
#define HAIKU_SYSCALL_SET_SOUND_VOLUME     (HAIKU_SYSCALL_MEDIA_KIT + 4)
#define HAIKU_SYSCALL_CREATE_SOUND_RECORDER (HAIKU_SYSCALL_MEDIA_KIT + 5)

// Legacy Haiku syscall compatibility (for existing apps)
#define HAIKU_SYSCALL_LEGACY_BWINDOW_CREATE     41
#define HAIKU_SYSCALL_LEGACY_BWINDOW_SHOW       114
#define HAIKU_SYSCALL_LEGACY_BWINDOW_HIDE       121
#define HAIKU_SYSCALL_LEGACY_BVIEW_DRAW_LINE   146
#define HAIKU_SYSCALL_LEGACY_BVIEW_FILL_RECT   147
#define HAIKU_SYSCALL_LEGACY_BVIEW_DRAW_STRING 148
#define HAIKU_SYSCALL_LEGACY_BVIEW_FLUSH       149
#define HAIKU_SYSCALL_LEGACY_BFILE_OPEN        150
#define HAIKU_SYSCALL_LEGACY_BFILE_READ        151

// ============================================================================
// HAIKU SYSCALL DISPATCHER
// ============================================================================

/**
 * Haiku-specific system call dispatcher
 * 
 * This class handles all Haiku/BeOS syscalls and routes them to the 
 * appropriate kit implementations. It provides compatibility with both
 * new Haiku API syscalls and legacy applications.
 */
class HaikuSyscallDispatcher : public SyscallDispatcher {
private:
    // Kit instances
    std::unique_ptr<HaikuInterfaceKit> interface_kit;
    std::unique_ptr<HaikuStorageKit> storage_kit;
    std::unique_ptr<HaikuApplicationKit> application_kit;
    std::unique_ptr<HaikuSupportKit> support_kit;
    std::unique_ptr<HaikuNetworkKit> network_kit;
    std::unique_ptr<HaikuMediaKit> media_kit;
    
    // Syscall routing table
    std::map<uint32_t, std::function<status_t(X86_32GuestContext&)>> syscall_table;
    
    // Performance and debugging
    bool debug_mode;
    bool verbose_mode;
    std::map<uint32_t, uint64_t> syscall_counters;
    
public:
    /**
     * Constructor
     * 
     * @param debug_mode Enable detailed syscall logging
     * @param verbose_mode Enable verbose output
     */
    explicit HaikuSyscallDispatcher(bool debug_mode = false, bool verbose_mode = false);
    
    /**
     * Destructor
     */
    virtual ~HaikuSyscallDispatcher();
    
    /**
     * Initialize all Haiku kits and syscall routing
     */
    virtual status_t Initialize();
    
    /**
     * Shutdown all kits and cleanup resources
     */
    virtual void Shutdown();
    
    /**
     * Main syscall dispatch method
     * 
     * @param context Guest execution context with syscall parameters
     * @return Haiku status code
     */
    virtual status_t Dispatch(GuestContext& context) override;
    
    /**
     * Get syscall statistics
     */
    virtual std::map<uint32_t, uint64_t> GetSyscallStatistics() const;
    
    /**
     * Reset syscall statistics
     */
    virtual void ResetStatistics();
    
    /**
     * Get kit instances
     */
    HaikuInterfaceKit* GetInterfaceKit() const { return interface_kit.get(); }
    HaikuStorageKit* GetStorageKit() const { return storage_kit.get(); }
    HaikuApplicationKit* GetApplicationKit() const { return application_kit.get(); }
    HaikuSupportKit* GetSupportKit() const { return support_kit.get(); }
    HaikuNetworkKit* GetNetworkKit() const { return network_kit.get(); }
    HaikuMediaKit* GetMediaKit() const { return media_kit.get(); }
    
    // Configuration methods
    void SetDebugMode(bool enable) { debug_mode = enable; }
    void SetVerboseMode(bool enable) { verbose_mode = enable; }
    bool IsDebugMode() const { return debug_mode; }
    bool IsVerboseMode() const { return verbose_mode; }
    
private:
    /**
     * Initialize syscall routing table
     */
    void InitializeSyscallTable();
    
    /**
     * Register a syscall handler
     */
    void RegisterSyscall(uint32_t syscall_num, 
                        std::function<status_t(X86_32GuestContext&)> handler);
    
    /**
     * Interface Kit syscall handlers
     */
    status_t HandleCreateWindow(X86_32GuestContext& context);
    status_t HandleShowWindow(X86_32GuestContext& context);
    status_t HandleHideWindow(X86_32GuestContext& context);
    status_t HandleDestroyWindow(X86_32GuestContext& context);
    status_t HandleDrawLine(X86_32GuestContext& context);
    status_t HandleFillRect(X86_32GuestContext& context);
    status_t HandleDrawString(X86_32GuestContext& context);
    status_t HandleFlush(X86_32GuestContext& context);
    status_t HandleAddChild(X86_32GuestContext& context);
    status_t HandleRemoveChild(X86_32GuestContext& context);
    
    /**
     * Storage Kit syscall handlers
     */
    status_t HandleOpenFile(X86_32GuestContext& context);
    status_t HandleCloseFile(X86_32GuestContext& context);
    status_t HandleReadFile(X86_32GuestContext& context);
    status_t HandleWriteFile(X86_32GuestContext& context);
    status_t HandleSeekFile(X86_32GuestContext& context);
    status_t HandleSetFileSize(X86_32GuestContext& context);
    status_t HandleOpenDirectory(X86_32GuestContext& context);
    status_t HandleCloseDirectory(X86_32GuestContext& context);
    status_t HandleReadDirectory(X86_32GuestContext& context);
    status_t HandleRewindDirectory(X86_32GuestContext& context);
    status_t HandleGetEntryInfo(X86_32GuestContext& context);
    status_t HandleCreateEntry(X86_32GuestContext& context);
    status_t HandleDeleteEntry(X86_32GuestContext& context);
    status_t HandleMoveEntry(X86_32GuestContext& context);
    
    /**
     * Application Kit syscall handlers
     */
    status_t HandleCreateApplication(X86_32GuestContext& context);
    status_t HandleRunApplication(X86_32GuestContext& context);
    status_t HandleQuitApplication(X86_32GuestContext& context);
    status_t HandleCreateMessage(X86_32GuestContext& context);
    status_t HandleSendMessage(X86_32GuestContext& context);
    status_t HandlePostMessage(X86_32GuestContext& context);
    
    /**
     * Support Kit syscall handlers
     */
    status_t HandleCreateString(X86_32GuestContext& context);
    status_t HandleSetString(X86_32GuestContext& context);
    status_t HandleGetString(X86_32GuestContext& context);
    status_t HandleDeleteString(X86_32GuestContext& context);
    status_t HandleCreateList(X86_32GuestContext& context);
    status_t HandleAddItem(X86_32GuestContext& context);
    status_t HandleRemoveItem(X86_32GuestContext& context);
    status_t HandleGetItem(X86_32GuestContext& context);
    status_t HandleCountItems(X86_32GuestContext& context);
    
    /**
     * Network Kit syscall handlers
     */
    status_t HandleCreateSocket(X86_32GuestContext& context);
    status_t HandleConnectSocket(X86_32GuestContext& context);
    status_t HandleBindSocket(X86_32GuestContext& context);
    status_t HandleListenSocket(X86_32GuestContext& context);
    status_t HandleAcceptSocket(X86_32GuestContext& context);
    status_t HandleCloseSocket(X86_32GuestContext& context);
    status_t HandleSendSocket(X86_32GuestContext& context);
    status_t HandleReceiveSocket(X86_32GuestContext& context);
    
    /**
     * Media Kit syscall handlers
     */
    status_t HandleCreateSoundPlayer(X86_32GuestContext& context);
    status_t HandleStartSoundPlayer(X86_32GuestContext& context);
    status_t HandleStopSoundPlayer(X86_32GuestContext& context);
    status_t HandleSetSoundVolume(X86_32GuestContext& context);
    status_t HandleCreateSoundRecorder(X86_32GuestContext& context);
    
    /**
     * Legacy compatibility handlers
     */
    status_t HandleLegacyWindowCreate(X86_32GuestContext& context);
    status_t HandleLegacyWindowShow(X86_32GuestContext& context);
    status_t HandleLegacyWindowHide(X86_32GuestContext& context);
    status_t HandleLegacyViewDrawLine(X86_32GuestContext& context);
    status_t HandleLegacyViewFillRect(X86_32GuestContext& context);
    status_t HandleLegacyViewDrawString(X86_32GuestContext& context);
    status_t HandleLegacyViewFlush(X86_32GuestContext& context);
    status_t HandleLegacyFileOpen(X86_32GuestContext& context);
    status_t HandleLegacyFileRead(X86_32GuestContext& context);
    
    /**
     * Utility methods
     */
    std::string GetSyscallName(uint32_t syscall_num);
    void LogSyscall(uint32_t syscall_num, const char* kit, const char* function);
    void ReadGuestString(uint32_t guest_address, char* buffer, size_t max_size);
    void WriteGuestString(uint32_t guest_address, const char* string);
    uint32_t ReadGuestUInt32(uint32_t guest_address);
    void WriteGuestUInt32(uint32_t guest_address, uint32_t value);
    
    // Guest context convenience methods
    X86_32Registers& GetRegisters(X86_32GuestContext& context);
    DirectAddressSpace& GetAddressSpace(X86_32GuestContext& context);
};

// ============================================================================
// SYSCALL HANDLER MACROS
// ============================================================================

// Helper macros for syscall parameter handling
#define GET_REGISTER(reg) (context.GetRegisters().reg)
#define SET_REGISTER(reg, value) (context.GetRegisters().reg = value)
#define GET_EAX() GET_REGISTER(eax)
#define GET_EBX() GET_REGISTER(ebx)
#define GET_ECX() GET_REGISTER(ecx)
#define GET_EDX() GET_REGISTER(edx)
#define GET_ESI() GET_REGISTER(esi)
#define GET_EDI() GET_REGISTER(edi)
#define SET_EAX(value) SET_REGISTER(eax, value)
#define SET_EBX(value) SET_REGISTER(ebx, value)
#define SET_ECX(value) SET_REGISTER(ecx, value)
#define SET_EDX(value) SET_REGISTER(edx, value)

// Guest memory access macros
#define READ_GUEST_STRING(addr, buffer) ReadGuestString(addr, buffer, sizeof(buffer))
#define READ_GUEST_UINT32(addr) ReadGuestUInt32(addr)
#define WRITE_GUEST_STRING(addr, str) WriteGuestString(addr, str)
#define WRITE_GUEST_UINT32(addr, value) WriteGuestUInt32(addr, value)

// Syscall result macros
#define RETURN_SUCCESS() SET_EAX(B_OK); return B_OK
#define RETURN_ERROR(err) SET_EAX(err); return err
#define RETURN_UINT32(value) SET_EAX(value); return B_OK

// Logging macros
#define LOG_SYSCALL(name) do { \
    if (debug_mode) { \
        printf("[HAIKU_SYSCALL] %s\n", name); \
    } \
} while(0)

#define LOG_SYSCALL_PARAMS(name, ...) do { \
    if (verbose_mode) { \
        printf("[HAIKU_SYSCALL] %s(" __VA_ARGS__ ")\n", name); \
    } \
} while(0)