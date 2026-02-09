#include "haikuOSIPCSystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <errno.h>

#ifndef __HAIKU__
// Implementación de ports para Linux (usando POSIX pipes y sockets)
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

// Implementación de áreas para Linux
struct port_info {
    int fd;                    // File descriptor para el pipe/socket
    int server_fd;             // Server side fd
    std::string name;         // Port name
    pthread_mutex_t mutex;
    std::queue<void*> message_queue;
    std::condition_variable cv;
};

static std::map<int32_t, port_info> g_ports;
static std::mutex g_ports_mutex;
static int32_t g_next_port_id = 1000;

// Implementación de semáforos para Linux
struct sem_info {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int32_t count;
    std::string name;
    int32_t ref_count;
};

static std::map<int32_t, sem_info> g_semaphores;
static std::mutex g_semaphores_mutex;
static int32_t g_next_sem_id = 2000;
#endif

HaikuOSIPCSystem::HaikuOSIPCSystem() 
    : app_server_running(false), app_server_thread(nullptr),
      app_server_main_port(-1),
      audio_initialized(false), audio_device(nullptr), audio_volume(1.0f),
      host_fb_connected(false), fb_thread(nullptr),
      haiku_libroot_handle(nullptr) {
    printf("[HaikuIPC] Initializing Haiku OS IPC System...\n");
}

HaikuOSIPCSystem::~HaikuOSIPCSystem() {
    printf("[HaikuIPC] Shutting down Haiku OS IPC System...\n");
    Shutdown();
}

bool HaikuOSIPCSystem::Initialize() {
    printf("[HaikuIPC] Starting IPC system initialization...\n");
    
    // Initialize audio system
    if (InitializeAudio() != B_OK) {
        printf("[HaikuIPC] WARNING: Audio initialization failed\n");
    }
    
    // Try to locate and load libroot.so
    if (LocateAndLoadLibroot() != B_OK) {
        printf("[HaikuIPC] WARNING: Could not load libroot.so\n");
        printf("[HaikuIPC] Will use simulation mode for app_server communication\n");
    }
    
    // Start app server
    if (StartAppServer() != B_OK) {
        printf("[HaikuIPC] ERROR: Failed to start app_server\n");
        return false;
    }
    
    printf("[HaikuIPC] IPC System initialization complete\n");
    return true;
}

void HaikuOSIPCSystem::Shutdown() {
    printf("[HaikuIPC] Shutting down...\n");
    
    // Stop app_server
    if (app_server_running) {
        StopAppServer();
    }
    
    // Shutdown audio
    if (audio_initialized) {
        printf("[HaikuIPC] Shutting down audio system...\n");
        audio_initialized = false;
    }
    
    // Disconnect host framebuffer
    if (host_fb_connected) {
        printf("[HaikuIPC] Disconnecting host framebuffer...\n");
        host_fb_connected = false;
    }
    
    // Unload libroot.so
    if (haiku_libroot_handle) {
        dlclose(haiku_libroot_handle);
        haiku_libroot_handle = nullptr;
        printf("[HaikuIPC] Unloaded libroot.so\n");
    }
}

// Port-based IPC implementation
int32_t HaikuOSIPCSystem::CreatePort(int32 capacity, const char *name) {
    if (!name) return B_BAD_VALUE;
    
    if (capacity <= 0 || capacity > B_PORT_MAX_CAPACITY) {
        capacity = B_PORT_DEFAULT_CAPACITY;
    }
    
    #ifndef __HAIKU__
    // Linux implementation using sockets
    int32_t port_id = g_next_port_id++;
    port_info &port = g_ports[port_id];
    
    // Create socket pair
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        printf("[HaikuIPC] ERROR: socketpair failed: %s\n", strerror(errno));
        return B_ERROR;
    }
    
    port.fd = sv[0];
    port.server_fd = sv[1];
    port.name = name;
    pthread_mutex_init(&port.mutex, nullptr);
    
    printf("[HaikuIPC] Created port %d '%s' (fd=%d, server_fd=%d)\n",
           port_id, name, port.fd, port.server_fd);
    
    return port_id;
    #else
    // Haiku implementation
    port_id port = create_port(capacity, name);
    if (port < B_OK) {
        printf("[HaikuIPC] ERROR: create_port failed: 0x%08x\n", port);
        return B_ERROR;
    }
    
    printf("[HaikuIPC] Created Haiku port %d '%s'\n", port_id, name);
    return port_id;
    #endif
}

int32_t HaikuOSIPCSystem::WritePort(int32_t port, int32_t msg_code, const void *buffer, size_t size) {
    if (!buffer || size == 0) return B_BAD_VALUE;
    
    #ifndef __HAIKU__
    std::lock_guard<std::mutex> lock(g_ports_mutex);
    auto it = g_ports.find(port);
    if (it == g_ports.end()) {
        printf("[HaikuIPC] ERROR: Port %d not found\n", port);
        return B_BAD_PORT;
    }
    
    port_info &port_info = it->second;
    std::lock_guard<std::mutex> port_lock(port_info.mutex);
    
    // Write message code and size
    if (write(port_info.fd, &msg_code, sizeof(msg_code)) != sizeof(msg_code) ||
        write(port_info.fd, &size, sizeof(size)) != sizeof(size)) {
        printf("[HaikuIPC] ERROR: Failed to write message header\n");
        return B_ERROR;
    }
    
    // Write message data
    if (write(port_info.fd, buffer, size) != (ssize_t)size) {
        printf("[HaikuIPC] ERROR: Failed to write message data: %s\n", strerror(errno));
        return B_ERROR;
    }
    
    printf("[HaikuIPC] Wrote message to port %d: code=0x%08x size=%zu\n", port, msg_code, size);
    return B_OK;
    #else
    status_t result = write_port(port, msg_code, buffer, size);
    if (result != B_OK) {
        printf("[HaikuIPC] ERROR: write_port failed: 0x%08x\n", result);
        return B_ERROR;
    }
    
    printf("[HaikuIPC] Wrote message to Haiku port %d: code=0x%08x size=%zu\n", port, msg_code, size);
    return B_OK;
    #endif
}

int32_t HaikuOSIPCSystem::ReadPort(int32_t port, int32_t *msg_code, void *buffer, size_t size, int64_t timeout) {
    if (!msg_code || !buffer || size == 0) return B_BAD_VALUE;
    
    #ifndef __HAIKU__
    std::lock_guard<std::mutex> lock(g_ports_mutex);
    auto it = g_ports.find(port);
    if (it == g_ports.end()) {
        printf("[HaikuIPC] ERROR: Port %d not found\n", port);
        return B_BAD_PORT;
    }
    
    port_info &port_info = it->second;
    
    // Check if there's data available
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(port_info.fd, &readfds);
    
    struct timeval tv;
    tv.tv_sec = timeout / 1000000;
    tv.tv_usec = (timeout % 1000000);
    
    int result = select(port_info.fd + 1, &readfds, nullptr, nullptr, timeout > 0 ? &tv : nullptr);
    if (result <= 0) {
        if (result == 0) {
            printf("[HaikuIPC] Port %d read timeout\n", port);
            return B_TIMED_OUT;
        } else {
            printf("[HaikuIPC] ERROR: select failed: %s\n", strerror(errno));
            return B_ERROR;
        }
    }
    
    // Read message code and size
    if (read(port_info.fd, msg_code, sizeof(*msg_code)) != sizeof(*msg_code)) {
        printf("[HaikuIPC] ERROR: Failed to read message code: %s\n", strerror(errno));
        return B_ERROR;
    }
    
    size_t message_size;
    if (read(port_info.fd, &message_size, sizeof(message_size)) != sizeof(message_size)) {
        printf("[HaikuIPC] ERROR: Failed to read message size: %s\n", strerror(errno));
        return B_ERROR;
    }
    
    // Limit read size to buffer capacity
    if (message_size > size) {
        printf("[HaikuIPC] WARNING: Message too large for buffer (%zu > %zu), truncating\n", message_size, size);
        message_size = size;
    }
    
    // Read message data
    ssize_t bytes_read = read(port_info.fd, buffer, message_size);
    if (bytes_read != (ssize_t)message_size) {
        printf("[HaikuIPC] ERROR: Failed to read message data: %s\n", strerror(errno));
        return B_ERROR;
    }
    
    printf("[HaikuIPC] Read message from port %d: code=0x%08x size=%zu\n", port, *msg_code, bytes_read);
    return B_OK;
    #else
    bigtime_t timeout_big = (bigtime_t)timeout * 1000;
    status_t result = read_port(port, msg_code, buffer, size, &timeout_big);
    
    if (result == B_TIMED_OUT) {
        printf("[HaikuIPC] Port %d read timeout\n", port);
        return B_TIMED_OUT;
    } else if (result != B_OK) {
        printf("[HaikuIPC] ERROR: read_port failed: 0x%08x\n", result);
        return B_ERROR;
    }
    
    size_t actual_size;
    result = read_port(port, nullptr, nullptr, 0, &actual_size);
    
    printf("[HaikuIPC] Read message from Haiku port %d: code=0x%08x size=%zu\n", port, *msg_code, actual_size);
    return B_OK;
    #endif
}

int32_t HaikuOSIPCSystem::ClosePort(int32_t port) {
    #ifndef __HAIKU__
    std::lock_guard<std::mutex> lock(g_ports_mutex);
    auto it = g_ports.find(port);
    if (it == g_ports.end()) {
        return B_BAD_PORT;
    }
    
    port_info &port_info = it->second;
    close(port_info.fd);
    close(port_info.server_fd);
    
    pthread_mutex_destroy(&port_info.mutex);
    g_ports.erase(it);
    
    printf("[HaikuIPC] Closed port %d\n", port);
    return B_OK;
    #else
    status_t result = delete_port(port);
    if (result == B_OK) {
        printf("[HaikuIPC] Closed Haiku port %d\n", port);
    } else {
        printf("[HaikuIPC] ERROR: delete_port failed: 0x%08x\n", result);
        return B_ERROR;
    }
    return B_OK;
    #endif
}

// Semaphore implementation
int32_t HaikuOSIPCSystem::CreateSemaphore(int32_t count, const char *name) {
    if (!name) return B_BAD_VALUE;
    if (count < 0) return B_BAD_VALUE;
    
    #ifndef __HAIKU__
    int32_t sem_id = g_next_sem_id++;
    sem_info &sem = g_semaphores[sem_id];
    
    pthread_mutex_init(&sem.mutex, nullptr);
    pthread_cond_init(&sem.cond, nullptr);
    sem.count = count;
    sem.name = name;
    sem.ref_count = 1;
    
    printf("[HaikuIPC] Created semaphore %d '%s' with count=%d\n", sem_id, name, count);
    return sem_id;
    #else
    sem_id sem = create_sem(count, name);
    if (sem < B_OK) {
        printf("[HaikuIPC] ERROR: create_sem failed: 0x%08x\n", sem);
        return B_ERROR;
    }
    
    printf("[HaikuIPC] Created Haiku semaphore %d '%s' with count=%d\n", sem, name, count);
    return sem;
    #endif
}

int32_t HaikuOSIPCSystem::AcquireSemaphore(int32_t sem) {
    #ifndef __HAIKU__
    std::lock_guard<std::mutex> lock(g_semaphores_mutex);
    auto it = g_semaphores.find(sem);
    if (it == g_semaphores.end()) {
        printf("[HaikuIPC] ERROR: Semaphore %d not found\n", sem);
        return B_BAD_SEM_ID;
    }
    
    sem_info &sem_info = it->second;
    pthread_mutex_lock(&sem_info.mutex);
    
    while (sem_info.count <= 0) {
        printf("[HaikuIPC] Waiting on semaphore %d '%s'\n", sem, sem_info.name.c_str());
        pthread_cond_wait(&sem_info.cond, &sem_info.mutex);
    }
    
    sem_info.count--;
    
    pthread_mutex_unlock(&sem_info.mutex);
    printf("[HaikuIPC] Acquired semaphore %d (remaining: %d)\n", sem, sem_info.count);
    return B_OK;
    #else
    status_t result = acquire_sem(sem);
    if (result != B_OK) {
        printf("[HaikuIPC] ERROR: acquire_sem failed: 0x%08x\n", result);
        return B_ERROR;
    }
    
    printf("[HaikuIPC] Acquired Haiku semaphore %d\n", sem);
    return B_OK;
    #endif
}

int32_t HaikuOSIPCSystem::ReleaseSemaphore(int32_t sem) {
    #ifndef __HAIKU__
    std::lock_guard<std::mutex> lock(g_semaphores_mutex);
    auto it = g_semaphores.find(sem);
    if (it == g_semaphores.end()) {
        printf("[HaikuIPC] ERROR: Semaphore %d not found\n", sem);
        return B_BAD_SEM_ID;
    }
    
    sem_info &sem_info = it->second;
    pthread_mutex_lock(&sem_info.mutex);
    
    sem_info.count++;
    pthread_cond_signal(&sem_info.cond);
    
    pthread_mutex_unlock(&sem_info.mutex);
    printf("[HaikuIPC] Released semaphore %d (available: %d)\n", sem, sem_info.count);
    return B_OK;
    #else
    status_t result = release_sem(sem);
    if (result != B_OK) {
        printf("[HaikuIPC] ERROR: release_sem failed: 0x%08x\n", result);
        return B_ERROR;
    }
    
    printf("[HaikuIPC] Released Haiku semaphore %d\n", sem);
    return B_OK;
    #endif
}

int32_t HaikuOSIPCSystem::DeleteSemaphore(int32_t sem) {
    #ifndef __HAIKU__
    std::lock_guard<std::mutex> lock(g_semaphores_mutex);
    auto it = g_semaphores.find(sem);
    if (it == g_semaphores.end()) {
        return B_BAD_SEM_ID;
    }
    
    sem_info &sem_info = it->second;
    pthread_mutex_destroy(&sem_info.mutex);
    pthread_cond_destroy(&sem_info.cond);
    
    g_semaphores.erase(it);
    printf("[HaikuIPC] Deleted semaphore %d\n", sem);
    return B_OK;
    #else
    status_t result = delete_sem(sem);
    if (result == B_OK) {
        printf("[HaikuIPC] Deleted Haiku semaphore %d\n", sem);
    } else {
        printf("[HaikuIPC] ERROR: delete_sem failed: 0x%08x\n", result);
        return B_ERROR;
    }
    return B_OK;
    #endif
}

// Area management
int32_t HaikuOSIPCSystem::CreateArea(const char *name, void **address, size_t size, uint32_t flags, uint32_t protection) {
    if (!address || size == 0) return B_BAD_VALUE;
    
    #ifndef __HAIKU__
    // Linux implementation using mmap
    int32_t area_id = g_next_area_id++;
    
    // Determine protection flags
    int prot = PROT_NONE;
    if (flags & B_READ_AREA) prot |= PROT_READ;
    if (flags & B_WRITE_AREA) prot |= PROT_WRITE;
    if (flags & B_EXECUTE_AREA) prot |= PROT_EXEC;
    if (prot == PROT_NONE) prot = PROT_READ | PROT_WRITE; // Default to readable/writable
    
    void *ptr = mmap(nullptr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        printf("[HaikuIPC] ERROR: mmap failed: %s\n", strerror(errno));
        return B_NO_MEMORY;
    }
    
    *address = ptr;
    
    {
        std::lock_guard<std::mutex> lock(g_area_mutex);
        mapped_areas[area_id] = ptr;
    }
    
    printf("[HaikuIPC] Created area %d '%s' at %p size=%zu flags=0x%08x\n",
           area_id, name ? name : "(null)", ptr, size, flags);
    
    return area_id;
    #else
    area_id area = create_area(name, address, size, flags, protection);
    if (area < B_OK) {
        printf("[HaikuIPC] ERROR: create_area failed: 0x%08x\n", area);
        return B_ERROR;
    }
    
    printf("[HaikuIPC] Created Haiku area %d '%s' at %p size=%zu flags=0x%08x\n",
           area, name ? name : "(null)", *address, size, flags);
    
    return area;
    #endif
}

int32_t HaikuOSIPCSystem::DeleteArea(int32_t area) {
    #ifndef __HAIKU__
    std::lock_guard<std::mutex> lock(g_area_mutex);
    auto it = mapped_areas.find(area);
    if (it == mapped_areas.end()) {
        return B_BAD_VALUE;
    }
    
    void *ptr = it->second;
    munmap(ptr, 0); // We don't track the size here
    
    mapped_areas.erase(it);
    printf("[HaikuIPC] Deleted area %d\n", area);
    return B_OK;
    #else
    status_t result = delete_area(area);
    if (result == B_OK) {
        printf("[HaikuIPC] Deleted Haiku area %d\n", area);
    } else {
        printf("[HaikuIPC] ERROR: delete_area failed: 0x%08x\n", result);
        return B_ERROR;
    }
    return B_OK;
    #endif
}