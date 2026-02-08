#ifndef NETWORK_SYSCALLS_H
#define NETWORK_SYSCALLS_H

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <string>
#include <atomic>
#include <functional>
#include <cstring>
#include <cstdio>

// Forward declarations to avoid circular dependencies
typedef uint32_t vm_pid_t;
typedef uint32_t tid_t;
typedef int32_t vm_sockfd_t;

// Network address structures
struct VMInetAddress {
    uint8_t family;           // AF_INET, AF_INET6, AF_UNIX
    uint16_t port;           // Network byte order
    union {
        uint32_t ipv4;       // IPv4 address (network byte order)
        uint8_t ipv6[16];   // IPv6 address
        char unix_path[108]; // Unix domain socket path
    } addr;
};

// Socket types
enum class VMSocketType {
    VM_SOCK_STREAM = 1,      // TCP
    VM_SOCK_DGRAM = 2,       // UDP
    VM_SOCK_RAW = 3          // Raw socket
};

// Socket families
enum class VMSocketFamily {
    VM_AF_INET = 2,          // IPv4
    VM_AF_INET6 = 10,        // IPv6
    VM_AF_UNIX = 1           // Unix domain
};

// Socket states
enum class VMSocketState {
    SOCKET_UNINITIALIZED,
    SOCKET_BOUND,
    SOCKET_LISTENING,
    SOCKET_CONNECTING,
    SOCKET_CONNECTED,
    SOCKET_CLOSING,
    SOCKET_CLOSED
};

// Network packet structure
struct VMPacket {
    std::vector<uint8_t> data;
    VMInetAddress src_addr;
    VMInetAddress dst_addr;
    uint64_t timestamp;
    uint32_t sequence_number;
    bool is_ack;
    bool is_syn;
    bool is_fin;
};

// Virtual socket implementation
class VMSocket {
private:
    vm_sockfd_t socket_fd_;
    VMSocketType type_;
    VMSocketFamily family_;
    VMSocketState state_;
    VMInetAddress local_addr_;
    VMInetAddress remote_addr_;
    
    // Buffer management
    std::vector<uint8_t> receive_buffer_;
    std::vector<uint8_t> send_buffer_;
    size_t receive_buffer_size_;
    size_t send_buffer_size_;
    
    // Socket options
    bool blocking_;
    int receive_timeout_;
    int send_timeout_;
    bool reuse_addr_;
    bool keep_alive_;
    
    // Statistics
    struct {
        std::atomic<uint64_t> bytes_sent{0};
        std::atomic<uint64_t> bytes_received{0};
        std::atomic<uint64_t> packets_sent{0};
        std::atomic<uint64_t> packets_received{0};
        std::atomic<uint64_t> connections_established{0};
        std::atomic<uint64_t> connections_dropped{0};
    } stats_;
    
    // Thread safety
    mutable std::mutex socket_mutex_;
    
public:
    VMSocket(vm_sockfd_t fd, VMSocketType type, VMSocketFamily family)
        : socket_fd_(fd), type_(type), family_(family), state_(VMSocketState::SOCKET_UNINITIALIZED),
          receive_buffer_size_(65536), send_buffer_size_(65536),
          blocking_(true), receive_timeout_(0), send_timeout_(0),
          reuse_addr_(false), keep_alive_(false) {
        
        receive_buffer_.reserve(receive_buffer_size_);
        send_buffer_.reserve(send_buffer_size_);
    }
    
    // Socket operations
    bool bind(const VMInetAddress& addr) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        
        if (state_ != VMSocketState::SOCKET_UNINITIALIZED) {
            return false;
        }
        
        local_addr_ = addr;
        state_ = VMSocketState::SOCKET_BOUND;
        return true;
    }
    
    bool listen(int backlog = 128) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        
        if (state_ != VMSocketState::SOCKET_BOUND || type_ != VMSocketType::VM_SOCK_STREAM) {
            return false;
        }
        
        state_ = VMSocketState::SOCKET_LISTENING;
        stats_.connections_established++;
        return true;
    }
    
    bool connect(const VMInetAddress& addr) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        
        if (state_ != VMSocketState::SOCKET_UNINITIALIZED && 
            state_ != VMSocketState::SOCKET_BOUND) {
            return false;
        }
        
        remote_addr_ = addr;
        state_ = VMSocketState::SOCKET_CONNECTING;
        
        // Simulate connection establishment
        state_ = VMSocketState::SOCKET_CONNECTED;
        stats_.connections_established++;
        return true;
    }
    
    std::unique_ptr<VMSocket> accept() {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        
        if (state_ != VMSocketState::SOCKET_LISTENING) {
            return nullptr;
        }
        
        // Create new socket for accepted connection
        static vm_sockfd_t next_fd = 1000;
        auto new_socket = std::make_unique<VMSocket>(next_fd++, type_, family_);
        new_socket->local_addr_ = local_addr_;
        new_socket->state_ = VMSocketState::SOCKET_CONNECTED;
        new_socket->stats_.connections_established++;
        
        return new_socket;
    }
    
    ssize_t send(const void* data, size_t len, int flags = 0) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        
        if (state_ != VMSocketState::SOCKET_CONNECTED) {
            return -1;
        }
        
        const uint8_t* byte_data = static_cast<const uint8_t*>(data);
        
        // Add data to send buffer
        if (send_buffer_.size() + len > send_buffer_size_) {
            return -1; // Buffer full
        }
        
        send_buffer_.insert(send_buffer_.end(), byte_data, byte_data + len);
        
        // Update statistics
        stats_.bytes_sent += len;
        stats_.packets_sent++;
        
        // Simulate network transmission
        send_buffer_.clear();
        
        return static_cast<ssize_t>(len);
    }
    
    ssize_t receive(void* buffer, size_t len, int flags = 0) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        
        if (state_ != VMSocketState::SOCKET_CONNECTED) {
            return -1;
        }
        
        if (receive_buffer_.empty()) {
            // Simulate receiving data
            std::vector<uint8_t> simulated_data(1024, 0x42); // Test data
            receive_buffer_ = simulated_data;
        }
        
        size_t bytes_to_copy = std::min(len, receive_buffer_.size());
        if (bytes_to_copy > 0) {
            std::memcpy(buffer, receive_buffer_.data(), bytes_to_copy);
            receive_buffer_.erase(receive_buffer_.begin(), 
                                 receive_buffer_.begin() + bytes_to_copy);
            
            // Update statistics
            stats_.bytes_received += bytes_to_copy;
            stats_.packets_received++;
        }
        
        return static_cast<ssize_t>(bytes_to_copy);
    }
    
    ssize_t send_to(const void* data, size_t len, const VMInetAddress& dest_addr, int flags = 0) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        
        if (type_ != VMSocketType::VM_SOCK_DGRAM) {
            return -1;
        }
        
        const uint8_t* byte_data = static_cast<const uint8_t*>(data);
        
        // Simulate UDP transmission
        stats_.bytes_sent += len;
        stats_.packets_sent++;
        
        return static_cast<ssize_t>(len);
    }
    
    ssize_t receive_from(void* buffer, size_t len, VMInetAddress* src_addr, int flags = 0) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        
        if (type_ != VMSocketType::VM_SOCK_DGRAM) {
            return -1;
        }
        
        // Simulate UDP reception
        std::vector<uint8_t> simulated_data(512, 0x43); // Test data
        size_t bytes_to_copy = std::min(len, simulated_data.size());
        
        if (bytes_to_copy > 0) {
            std::memcpy(buffer, simulated_data.data(), bytes_to_copy);
            
            if (src_addr) {
                src_addr->family = static_cast<uint8_t>(VMSocketFamily::VM_AF_INET);
                src_addr->port = 8080;
                src_addr->addr.ipv4 = 0x7F000001; // 127.0.0.1
            }
            
            stats_.bytes_received += bytes_to_copy;
            stats_.packets_received++;
        }
        
        return static_cast<ssize_t>(bytes_to_copy);
    }
    
    bool close() {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        
        if (state_ == VMSocketState::SOCKET_CLOSED) {
            return false;
        }
        
        state_ = VMSocketState::SOCKET_CLOSING;
        
        // Clear buffers
        receive_buffer_.clear();
        send_buffer_.clear();
        
        state_ = VMSocketState::SOCKET_CLOSED;
        stats_.connections_dropped++;
        
        return true;
    }
    
    // Getters and setters
    vm_sockfd_t get_fd() const { return socket_fd_; }
    VMSocketType get_type() const { return type_; }
    VMSocketFamily get_family() const { return family_; }
    VMSocketState get_state() const { return state_; }
    const VMInetAddress& get_local_addr() const { return local_addr_; }
    const VMInetAddress& get_remote_addr() const { return remote_addr_; }
    
    // Socket options
    bool set_blocking(bool blocking) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        blocking_ = blocking;
        return true;
    }
    
    bool set_reuse_addr(bool reuse) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        reuse_addr_ = reuse;
        return true;
    }
    
    bool set_keep_alive(bool keep_alive) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        keep_alive_ = keep_alive;
        return true;
    }
    
    // Statistics
    struct SocketStats {
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint64_t packets_sent;
        uint64_t packets_received;
        uint64_t connections_established;
        uint64_t connections_dropped;
    };
    
    SocketStats get_statistics() const {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        return {
            stats_.bytes_sent.load(),
            stats_.bytes_received.load(),
            stats_.packets_sent.load(),
            stats_.packets_received.load(),
            stats_.connections_established.load(),
            stats_.connections_dropped.load()
        };
    }
};

// Network syscall dispatcher
class NetworkSyscallDispatcher {
private:
    std::unordered_map<vm_sockfd_t, std::unique_ptr<VMSocket>> sockets_;
    std::unordered_map<vm_pid_t, std::vector<vm_sockfd_t>> process_sockets_;
    std::atomic<vm_sockfd_t> next_socket_fd_{1000};
    
    mutable std::mutex network_mutex_;
    
    // Network statistics
    struct {
        std::atomic<uint64_t> total_sockets_created{0};
        std::atomic<uint64_t> total_sockets_closed{0};
        std::atomic<uint64_t> total_bytes_sent{0};
        std::atomic<uint64_t> total_bytes_received{0};
        std::atomic<uint64_t> total_connections{0};
    } network_stats_;
    
public:
    NetworkSyscallDispatcher() = default;
    
    // Socket system calls
    vm_sockfd_t socket_create(vm_pid_t pid, VMSocketFamily family, VMSocketType type, int protocol = 0) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        vm_sockfd_t socket_fd = next_socket_fd_++;
        
        auto new_socket = std::make_unique<VMSocket>(socket_fd, type, family);
        
        sockets_[socket_fd] = std::move(new_socket);
        process_sockets_[pid].push_back(socket_fd);
        
        network_stats_.total_sockets_created++;
        
        return socket_fd;
    }
    
    int socket_bind(vm_sockfd_t sockfd, const VMInetAddress* addr) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        if (it == sockets_.end() || !addr) {
            return -1;
        }
        
        return it->second->bind(*addr) ? 0 : -1;
    }
    
    int socket_listen(vm_sockfd_t sockfd, int backlog) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        if (it == sockets_.end()) {
            return -1;
        }
        
        return it->second->listen(backlog) ? 0 : -1;
    }
    
    vm_sockfd_t socket_accept(vm_sockfd_t sockfd, VMInetAddress* client_addr) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        if (it == sockets_.end()) {
            return -1;
        }
        
        auto new_socket = it->second->accept();
        if (!new_socket) {
            return -1;
        }
        
        vm_sockfd_t new_fd = new_socket->get_fd();
        sockets_[new_fd] = std::move(new_socket);
        
        if (client_addr) {
            // Simulate client address
            client_addr->family = static_cast<uint8_t>(VMSocketFamily::VM_AF_INET);
            client_addr->port = 12345;
            client_addr->addr.ipv4 = 0xC0A80101; // 192.168.1.1
        }
        
        network_stats_.total_connections++;
        
        return new_fd;
    }
    
    int socket_connect(vm_sockfd_t sockfd, const VMInetAddress* addr) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        if (it == sockets_.end() || !addr) {
            return -1;
        }
        
        bool success = it->second->connect(*addr);
        if (success) {
            network_stats_.total_connections++;
        }
        
        return success ? 0 : -1;
    }
    
    ssize_t socket_send(vm_sockfd_t sockfd, const void* data, size_t len, int flags) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        if (it == sockets_.end()) {
            return -1;
        }
        
        ssize_t result = it->second->send(data, len, flags);
        if (result > 0) {
            network_stats_.total_bytes_sent += result;
        }
        
        return result;
    }
    
    ssize_t socket_receive(vm_sockfd_t sockfd, void* buffer, size_t len, int flags) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        if (it == sockets_.end()) {
            return -1;
        }
        
        ssize_t result = it->second->receive(buffer, len, flags);
        if (result > 0) {
            network_stats_.total_bytes_received += result;
        }
        
        return result;
    }
    
    ssize_t socket_send_to(vm_sockfd_t sockfd, const void* data, size_t len, 
                          const VMInetAddress* dest_addr, int flags) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        if (it == sockets_.end() || !dest_addr) {
            return -1;
        }
        
        ssize_t result = it->second->send_to(data, len, *dest_addr, flags);
        if (result > 0) {
            network_stats_.total_bytes_sent += result;
        }
        
        return result;
    }
    
    ssize_t socket_receive_from(vm_sockfd_t sockfd, void* buffer, size_t len, 
                                VMInetAddress* src_addr, int flags) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        if (it == sockets_.end()) {
            return -1;
        }
        
        ssize_t result = it->second->receive_from(buffer, len, src_addr, flags);
        if (result > 0) {
            network_stats_.total_bytes_received += result;
        }
        
        return result;
    }
    
    int socket_close(vm_sockfd_t sockfd) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        if (it == sockets_.end()) {
            return -1;
        }
        
        it->second->close();
        sockets_.erase(it);
        
        // Remove from process socket lists
        for (auto& pair : process_sockets_) {
            auto& vec = pair.second;
            for (size_t i = 0; i < vec.size(); i++) {
                if (vec[i] == sockfd) {
                    vec.erase(vec.begin() + i);
                    break;
                }
            }
        }
        
        network_stats_.total_sockets_closed++;
        
        return 0;
    }
    
    // Process cleanup
    void cleanup_process_sockets(vm_pid_t pid) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = process_sockets_.find(pid);
        if (it != process_sockets_.end()) {
            for (vm_sockfd_t sockfd : it->second) {
                auto socket_it = sockets_.find(sockfd);
                if (socket_it != sockets_.end()) {
                    socket_it->second->close();
                    sockets_.erase(socket_it);
                    network_stats_.total_sockets_closed++;
                }
            }
            process_sockets_.erase(it);
        }
    }
    
    // Socket information
    VMSocket* get_socket(vm_sockfd_t sockfd) {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        auto it = sockets_.find(sockfd);
        return (it != sockets_.end()) ? it->second.get() : nullptr;
    }
    
    // Statistics
    struct NetworkStats {
        uint64_t total_sockets_created;
        uint64_t total_sockets_closed;
        uint64_t total_bytes_sent;
        uint64_t total_bytes_received;
        uint64_t total_connections;
        uint64_t active_sockets;
        uint64_t processes_with_sockets;
    };
    
    NetworkStats get_statistics() const {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        return {
            network_stats_.total_sockets_created.load(),
            network_stats_.total_sockets_closed.load(),
            network_stats_.total_bytes_sent.load(),
            network_stats_.total_bytes_received.load(),
            network_stats_.total_connections.load(),
            sockets_.size(),
            process_sockets_.size()
        };
    }
    
    void print_statistics() const {
        auto stats = get_statistics();
        
        printf("\n=== NETWORK STATISTICS ===\n");
        printf("Total Sockets Created: %lu\n", stats.total_sockets_created);
        printf("Total Sockets Closed: %lu\n", stats.total_sockets_closed);
        printf("Active Sockets: %lu\n", stats.active_sockets);
        printf("Processes with Sockets: %lu\n", stats.processes_with_sockets);
        printf("Total Connections: %lu\n", stats.total_connections);
        printf("Total Bytes Sent: %lu\n", stats.total_bytes_sent);
        printf("Total Bytes Received: %lu\n", stats.total_bytes_received);
        printf("=============================\n\n");
    }
    
    // Cleanup
    void cleanup() {
        std::lock_guard<std::mutex> lock(network_mutex_);
        
        for (auto& pair : sockets_) {
            pair.second->close();
        }
        
        sockets_.clear();
        process_sockets_.clear();
    }
};

// Global network syscall dispatcher instance
extern std::unique_ptr<NetworkSyscallDispatcher> g_network_dispatcher;

// Network syscall interface functions
vm_sockfd_t vm_socket(vm_pid_t pid, VMSocketFamily family, VMSocketType type, int protocol);
int vm_bind(vm_sockfd_t sockfd, const VMInetAddress* addr);
int vm_listen(vm_sockfd_t sockfd, int backlog);
vm_sockfd_t vm_accept(vm_sockfd_t sockfd, VMInetAddress* client_addr);
int vm_connect(vm_sockfd_t sockfd, const VMInetAddress* addr);
ssize_t vm_send(vm_sockfd_t sockfd, const void* data, size_t len, int flags);
ssize_t vm_recv(vm_sockfd_t sockfd, void* buffer, size_t len, int flags);
ssize_t vm_sendto(vm_sockfd_t sockfd, const void* data, size_t len, const VMInetAddress* dest_addr, int flags);
ssize_t vm_recvfrom(vm_sockfd_t sockfd, void* buffer, size_t len, VMInetAddress* src_addr, int flags);
int vm_close_socket(vm_sockfd_t sockfd);
void vm_cleanup_network();

#endif // NETWORK_SYSCALLS_H