/*
 * HaikuAPIVirtualizer.cpp - Main Haiku API Virtualizer Implementation
 * 
 * Connects all Haiku kits together and provides unified interface
 * for cross-platform Haiku application execution
 */

#include "HaikuAPIVirtualizer.h"
#include "HaikuApplicationServer.h"
#include "HaikuSyscallDispatcher.h"
#include "HaikuSupportKit.h"
#include "HaikuStorageKit.h"
#include "HaikuInterfaceKitSimple.h"
#include "HaikuApplicationKit.h"
#include "HaikuNetworkKit.h"
#include "HaikuMediaKit.h"

#include <iostream>
#include <cstring>

// Global instance
static HaikuAPIVirtualizer* sInstance = nullptr;

// ============================================================================
// HaikuAPIVirtualizer Implementation
// ============================================================================

class HaikuAPIVirtualizerImpl : public HaikuAPIVirtualizer {
private:
    // All Haiku kits
    HaikuApplicationServer* appServer;
    HaikuSyscallDispatcher* syscallDispatcher;
    HaikuSupportKit* supportKit;
    HaikuStorageKit* storageKit;
    HaikuInterfaceKitSimple* interfaceKit;
    HaikuApplicationKit* applicationKit;
    HaikuNetworkKit* networkKit;
    HaikuMediaKit* mediaKit;
    
    // State
    bool initialized;
    std::mutex mutex;
    
public:
    HaikuAPIVirtualizerImpl()
        : appServer(nullptr),
          syscallDispatcher(nullptr),
          supportKit(nullptr),
          storageKit(nullptr),
          interfaceKit(nullptr),
          applicationKit(nullptr),
          networkKit(nullptr),
          mediaKit(nullptr),
          initialized(false)
    {
    }
    
    virtual ~HaikuAPIVirtualizerImpl() {
        Shutdown();
    }
    
    status_t Initialize() override {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (initialized) {
            return B_OK;
        }
        
        printf("[HaikuAPI] Initializing Haiku API Virtualizer...\n");
        
        // Initialize Support Kit first (foundation)
        supportKit = new HaikuSupportKit();
        if (!supportKit) {
            std::cerr << "[HaikuAPI] ERROR: Failed to create Support Kit" << std::endl;
            return B_ERROR;
        }
        printf("[HaikuAPI] ✓ Support Kit initialized\n");
        
        // Initialize Storage Kit
        storageKit = new HaikuStorageKit();
        if (!storageKit) {
            std::cerr << "[HaikuAPI] ERROR: Failed to create Storage Kit" << std::endl;
            return B_ERROR;
        }
        printf("[HaikuAPI] ✓ Storage Kit initialized\n");
        
        // Initialize Interface Kit
        interfaceKit = &HaikuInterfaceKitSimple::GetInstance();
        if (interfaceKit) {
            interfaceKit->Initialize();
            printf("[HaikuAPI] ✓ Interface Kit initialized\n");
        }
        
        // Initialize Application Kit
        applicationKit = new HaikuApplicationKit();
        if (!applicationKit) {
            std::cerr << "[HaikuAPI] ERROR: Failed to create Application Kit" << std::endl;
            return B_ERROR;
        }
        printf("[HaikuAPI] ✓ Application Kit initialized\n");
        
        // Initialize Network Kit
        networkKit = new HaikuNetworkKit();
        if (!networkKit) {
            std::cerr << "[HaikuAPI] ERROR: Failed to create Network Kit" << std::endl;
            return B_ERROR;
        }
        printf("[HaikuAPI] ✓ Network Kit initialized\n");
        
        // Initialize Media Kit
        mediaKit = new HaikuMediaKit();
        if (!mediaKit) {
            std::cerr << "[HaikuAPI] ERROR: Failed to create Media Kit" << std::endl;
            return B_ERROR;
        }
        printf("[HaikuAPI] ✓ Media Kit initialized\n");
        
        // Initialize Application Server
        appServer = new HaikuApplicationServer();
        if (!appServer) {
            std::cerr << "[HaikuAPI] ERROR: Failed to create Application Server" << std::endl;
            return B_ERROR;
        }
        appServer->Initialize();
        printf("[HaikuAPI] ✓ Application Server initialized\n");
        
        // Initialize Syscall Dispatcher
        syscallDispatcher = new HaikuSyscallDispatcher();
        if (!syscallDispatcher) {
            std::cerr << "[HaikuAPI] ERROR: Failed to create Syscall Dispatcher" << std::endl;
            return B_ERROR;
        }
        syscallDispatcher->Initialize(appServer);
        printf("[HaikuAPI] ✓ Syscall Dispatcher initialized\n");
        
        initialized = true;
        printf("[HaikuAPI] ✅ Haiku API Virtualizer fully initialized!\n");
        printf("[HaikuAPI] All 6 kits connected: Support, Storage, Interface, Application, Network, Media\n");
        
        return B_OK;
    }
    
    void Shutdown() override {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (!initialized) {
            return;
        }
        
        printf("[HaikuAPI] Shutting down Haiku API Virtualizer...\n");
        
        // Shutdown in reverse order
        if (syscallDispatcher) {
            delete syscallDispatcher;
            syscallDispatcher = nullptr;
        }
        
        if (appServer) {
            appServer->Shutdown();
            delete appServer;
            appServer = nullptr;
        }
        
        if (mediaKit) {
            delete mediaKit;
            mediaKit = nullptr;
        }
        
        if (networkKit) {
            delete networkKit;
            networkKit = nullptr;
        }
        
        if (applicationKit) {
            delete applicationKit;
            applicationKit = nullptr;
        }
        
        if (interfaceKit) {
            // InterfaceKitSimple is singleton, don't delete
            interfaceKit = nullptr;
        }
        
        if (storageKit) {
            delete storageKit;
            storageKit = nullptr;
        }
        
        if (supportKit) {
            delete supportKit;
            supportKit = nullptr;
        }
        
        initialized = false;
        printf("[HaikuAPI] ✅ Shutdown complete\n");
    }
    
    bool IsInitialized() const override {
        return initialized;
    }
    
    HaikuApplicationServer* GetApplicationServer() override {
        return appServer;
    }
    
    HaikuSyscallDispatcher* GetSyscallDispatcher() override {
        return syscallDispatcher;
    }
    
    // Kit accessors
    HaikuSupportKit* GetSupportKit() { return supportKit; }
    HaikuStorageKit* GetStorageKit() { return storageKit; }
    HaikuInterfaceKitSimple* GetInterfaceKit() { return interfaceKit; }
    HaikuApplicationKit* GetApplicationKit() { return applicationKit; }
    HaikuNetworkKit* GetNetworkKit() { return networkKit; }
    HaikuMediaKit* GetMediaKit() { return mediaKit; }
};

// ============================================================================
// Factory Implementation
// ============================================================================

namespace HaikuAPI {

HaikuAPIVirtualizer* CreateVirtualizer() {
    if (!sInstance) {
        sInstance = new HaikuAPIVirtualizerImpl();
    }
    return sInstance;
}

void DestroyVirtualizer() {
    if (sInstance) {
        delete sInstance;
        sInstance = nullptr;
    }
}

HaikuAPIVirtualizer* GetVirtualizer() {
    return sInstance;
}

} // namespace HaikuAPI

// ============================================================================
// Global Functions
// ============================================================================

HaikuAPIVirtualizer* CreateHaikuVirtualizer() {
    return HaikuAPI::CreateVirtualizer();
}

void DestroyHaikuVirtualizer() {
    HaikuAPI::DestroyVirtualizer();
}

HaikuAPIVirtualizer* GetHaikuVirtualizer() {
    return HaikuAPI::GetVirtualizer();
}
