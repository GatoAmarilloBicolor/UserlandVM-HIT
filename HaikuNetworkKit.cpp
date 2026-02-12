/*
 * HaikuNetworkKit.cpp - Complete Haiku Network Kit Implementation (Part 1/2)
 * 
 * Implements all Haiku network functionality:
 * - Socket operations (TCP/UDP sockets)
 * - Address management (BNetAddress)
 * - Network buffering (BNetBuffer)
 * - HTTP client functionality (BHttpRequest)
 * - DNS resolution (DNS queries)
 * - Cross-platform network abstraction
 */

#include "HaikuNetworkKit.h"
#include "UnifiedStatusCodes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <ifaddrs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>

// ============================================================================
// HAIKU NETWORK KIT IMPLEMENTATION (PART 1/2)
// ============================================================================

// Static instance initialization
HaikuNetworkKitImpl* HaikuNetworkKitImpl::instance = nullptr;
std::mutex HaikuNetworkKitImpl::instance_mutex;

HaikuNetworkKitImpl& HaikuNetworkKitImpl::GetInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (!instance) {
        instance = new HaikuNetworkKitImpl();
    }
    return *instance;
}

HaikuNetworkKitImpl::HaikuNetworkKitImpl() : HaikuKit("Network Kit") {
    next_socket_id = 1;
    next_buffer_id = 1;
    next_url_id = 1;
    next_request_id = 1;
    next_dns_query_id = 1;
    
    memset(&network_stats, 0, sizeof(network_stats));
    
    printf("[HAIKU_NETWORK] Initializing Network Kit...\n");
}

HaikuNetworkKitImpl::~HaikuNetworkKitImpl() {
    if (is_initialized) {
        Shutdown();
    }
    delete instance;
    instance = nullptr;
}

status_t HaikuNetworkKitImpl::Initialize() {
    if (is_initialized) {
        return B_OK;
    }
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    printf("[HAIKU_NETWORK] ✅ Network Kit initialized\n");
    printf("[HAIKU_NETWORK] 🌐 Socket system ready\n");
    printf("[HAIKU_NETWORK] 🌍 Address management ready\n");
    printf("[HAIKU_NETWORK] 📦 Network buffering ready\n");
    printf("[HAIKU_NETWORK] 🌐 HTTP client ready\n");
    printf("[HAIKU_NETWORK] 🔍 DNS resolution ready\n");
    
    is_initialized = true;
    return B_OK;
}

void HaikuNetworkKitImpl::Shutdown() {
    if (!is_initialized) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    printf("[HAIKU_NETWORK] Shutting down Network Kit...\n");
    
    // Close all sockets
    for (auto& pair : sockets) {
        if (pair.second && pair.second->IsValid()) {
            printf("[HAIKU_NETWORK] 🗑️ Closing socket %u\n", pair.first);
        }
        delete pair.second;
    }
    sockets.clear();
    
    // Delete all buffers
    for (auto& pair : buffers) {
        if (pair.second && pair.second->IsValid()) {
            printf("[HAIKU_NETWORK] 🗑️ Deleting buffer %u (%zu bytes)\n", 
                   pair.first, pair.second->size);
        }
        delete pair.second;
    }
    buffers.clear();
    
    // Delete all URLs
    for (auto& pair : urls) {
        delete pair.second;
    }
    urls.clear();
    
    // Delete all HTTP requests
    for (auto& pair : requests) {
        delete pair.second;
    }
    requests.clear();
    
    // Clear DNS cache
    dns_cache.clear();
    
    is_initialized = false;
    
    printf("[HAIKU_NETWORK] ✅ Network Kit shutdown complete\n");
}

// ============================================================================
// SOCKET OPERATIONS (PART 1/2)
// ============================================================================

uint32_t HaikuNetworkKitImpl::CreateSocket(uint32_t family, uint32_t type, uint32_t protocol) {
    if (!is_initialized) return 0;
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    uint32_t socket_id = next_socket_id++;
    
    // Create socket
    int socket_fd = socket(family, type | SOCK_NONBLOCK, protocol);
    if (socket_fd < 0) {
        printf("[HAIKU_NETWORK] ❌ Failed to create socket (errno: %d)\n", errno);
        return 0;
    }
    
    HaikuNetEndpoint* endpoint = new HaikuEndpoint();
    endpoint->socket_fd = socket_fd;
    endpoint->socket_type = type;
    endpoint->socket_family = family;
    endpoint->socket_protocol = protocol;
    endpoint->is_connected = false;
    endpoint->is_bound = false;
    endpoint->is_listening = false;
    endpoint->id = socket_id;
    
    sockets[socket_id] = endpoint;
    
    network_stats.sockets_created++;
    
    printf("[HAIKU_NETWORK] 🔌 Created socket %u (fd=%d, family=%s, type=%s, protocol=%s)\n",
           socket_id, socket_fd,
           (family == HAIKU_SOCKET_FAMILY_INET) ? "IPv4" : "Other",
           (type == HAIKU_SOCKET_TYPE_STREAM) ? "Stream" : "Datagram",
           (protocol == HAIKU_PROTOCOL_TCP) ? "TCP" : "UDP");
    
    return socket_id;
}

status_t HaikuNetworkKitImpl::ConnectSocket(uint32_t socket_id, const char* address, uint16_t port) {
    if (!is_initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = sockets.find(socket_id);
    if (it == sockets.end() || !it->second->IsValid()) {
        return B_BAD_VALUE;
    }
    
    HaikuEndpoint* endpoint = it->second;
    
    // Create address
    HaikuNetAddress* addr = CreateAddress(address, port, endpoint->socket_family);
    if (!addr || !addr->IsValid()) {
        printf("[HAIKU_NETWORK] ❌ Invalid address: %s:%d\n", address, port);
        DeleteAddress(addr->id);
        return B_BAD_VALUE;
    }
    
    // Connect socket
    int result = connect(endpoint->socket_fd, 
                       reinterpret_cast<struct sockaddr*>(&addr->ip4_address),
                       sizeof(addr->ip4_address));
    
    if (result < 0 && errno != EINPROGRESS) {
        printf("[HAIKU_NETWORK] ❌ Failed to connect socket %u to %s:%d (errno: %d)\n",
               socket_id, address, port, errno);
        DeleteAddress(addr->id);
        return B_ERROR;
    }
    
    // Set socket to non-blocking if connecting
    if (result < 0 && errno == EINPROGRESS) {
        printf("[HAIKU_NETWORK] 🔌 Connecting socket %u to %s:%d...\n",
               socket_id, address, port);
    } else if (result == 0) {
        endpoint->is_connected = true;
        endpoint->remote_address = *addr;
        network_stats.connections_made++;
        
        printf("[HAIKU_NETWORK] ✅ Connected socket %u to %s:%d\n",
               socket_id, address, port);
    }
    
    DeleteAddress(addr->id);
    return (result == 0 || errno == EINPROGRESS) ? B_OK : B_ERROR;
}

status_t HaikuNetworkKitImpl::BindSocket(uint32_t socket_id, const char* address, uint16_t port) {
    if (!is_initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = sockets.find(socket_id);
    if (it == sockets.end() || !it->second->IsValid()) {
        return B_BAD_VALUE;
    }
    
    HaikuEndpoint* endpoint = it->second;
    
    // Create address
    HaikuNetAddress* addr = CreateAddress(address, port, endpoint->socket_family);
    if (!addr || !addr->IsValid()) {
        printf("[HAIKU_NETWORK] ❌ Invalid address: %s:%d\n", address, port);
        DeleteAddress(addr->id);
        return B_BAD_VALUE;
    }
    
    // Set SO_REUSEADDR option
    int reuse = 1;
    setsockopt(endpoint->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // Bind socket
    int result = bind(endpoint->socket_fd,
                     reinterpret_cast<struct sockaddr*>(&addr->ip4_address),
                     sizeof(addr->ip4_address));
    
    if (result < 0) {
        printf("[HAIKU_NETWORK] ❌ Failed to bind socket %u to %s:%d (errno: %d)\n",
               socket_id, address, port, errno);
        DeleteAddress(addr->id);
        return B_ERROR;
    }
    
    endpoint->is_bound = true;
    endpoint->local_address = *addr;
    
    printf("[HAIKU_NETWORK] ✅ Bound socket %u to %s:%d\n", socket_id, address, port);
    
    DeleteAddress(addr->id);
    return B_OK;
}

status_t HaikuNetworkKitImpl::ListenSocket(uint32_t socket_id, int32_t backlog) {
    if (!is_initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = sockets.find(socket_id);
    if (it == sockets.end() || !it->second->IsValid()) {
        return B_BAD_VALUE;
    }
    
    HaikuEndpoint* endpoint = it->second;
    
    if (!endpoint->is_bound) {
        printf("[HAIKU_NETWORK] ❌ Socket %u is not bound\n", socket_id);
        return B_BAD_VALUE;
    }
    
    // Listen socket
    int result = listen(endpoint->socket_fd, backlog);
    if (result < 0) {
        printf("[HAIKU_NETWORK] ❌ Failed to listen on socket %u (errno: %d)\n",
               socket_id, errno);
        return B_ERROR;
    }
    
    endpoint->is_listening = true;
    
    printf("[HAIKU_NETWORK] ✅ Socket %u listening (backlog=%d)\n", socket_id, backlog);
    
    return B_OK;
}

uint32_t HaikuNetworkKitImpl::AcceptSocket(uint32_t socket_id, char* client_address, uint16_t* port) {
    if (!initialized || !client_address || !port) {
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = sockets.find(socket_id);
    if (it == sockets.end() || !it->second->IsValid() || !it->second->is_listening) {
        return 0;
    }
    
    HaikuEndpoint* server_endpoint = it->second;
    
    // Accept connection
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_endpoint->socket_fd,
                           reinterpret_cast<struct sockaddr*>(&client_addr),
                           &addr_len);
    
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("[HAIKU_NETWORK] ❌ Failed to accept connection (errno: %d)\n", errno);
        }
        return 0;
    }
    
    // Create new socket for client
    uint32_t client_socket_id = CreateSocket(server_endpoint->socket_family,
                                          server_endpoint->socket_type,
                                          server_endpoint->socket_protocol);
    if (client_socket_id == 0) {
        close(client_fd);
        return 0;
    }
    
    // Replace socket fd with client fd
    auto client_it = sockets.find(client_socket_id);
    if (client_it == sockets.end()) {
        close(client_fd);
        return 0;
    }
    
    HaikuEndpoint* client_endpoint = client_it->second;
    close(client_endpoint->socket_fd);
    client_endpoint->socket_fd = client_fd;
    
    // Set client address
    HaikuNetAddress client_addr;
    client_addr.SetIPv4(client_addr.sin_addr.s_addr,
                       client_addr.sin_addr.s_port);
    
    client_endpoint->is_connected = true;
    client_endpoint->remote_address = client_addr;
    
    // Copy client address and port to parameters
    if (client_address) {
        strcpy(client_address, client_addr.GetIPString().c_str());
    }
    if (port) {
        *port = client_addr.port;
    }
    
    network_stats.connections_made++;
    
    printf("[HAIKU_NETWORK] ✅ Accepted connection on socket %u (client_fd=%d, client=%s:%d)\n",
           socket_id, client_fd, client_address, *port);
    
    return client_socket_id;
}

status_t HaikuNetworkKitImpl::CloseSocket(uint32_t socket_id) {
    if (!initialized) return B_BAD_VALUE;
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = sockets.find(socket_id);
    if (it == sockets.end()) {
        return B_BAD_VALUE;
    }
    
    HaikuEndpoint* endpoint = it->second;
    if (endpoint->IsValid()) {
        printf("[HAIKU_NETWORK] 🗑️ Closing socket %u (fd=%d)\n", 
               socket_id, endpoint->socket_fd);
        
        endpoint->is_connected = false;
        endpoint->is_listening = false;
    }
    
    delete endpoint;
    sockets.erase(it);
    
    return B_OK;
}

// ============================================================================
// ADDRESS OPERATIONS
// ============================================================================

uint32_t HaikuNetworkKitImpl::CreateAddress(const char* host, uint16_t port, uint32_t family) {
    if (!initialized) return 0;
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    uint32_t address_id = next_address_id++;
    
    HaikuNetAddress* address = new HaikuAddress();
    address->id = address_id;
    address->port = port;
    address->family = family;
    
    // Set hostname
    if (host) {
        strncpy(address->host, host, sizeof(address->host) - 1);
        
        // Try to resolve hostname
        if (ResolveHostname(host, address) != B_OK) {
            // Use as-is if resolution fails
            printf("[HAIKU_NETWORK] ⚠️  Using hostname as-is: %s\n", host);
        }
    }
    
    printf("[HAIKU_NETWORK] 📍 Created address %u: %s:%d (%s)\n",
           address_id, host ? host : "any", port, address->GetFamilyString());
    
    return address_id;
}

status_t HaikuNetworkKitImpl::ResolveHostname(const char* hostname, HaikuNetAddress* address) {
    if (!hostname || !address) return B_BAD_VALUE;
    
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int ret = getaddrinfo(hostname, nullptr, &hints, &result);
    if (ret != 0) {
        printf("[HAIKU_NETWORK] ❌ Failed to resolve hostname: %s (errno: %d)\n", hostname, errno);
        return B_ERROR;
    }
    
    // Use first result
    if (result->ai_family == AF_INET) {
        struct sockaddr_in* addr_in = reinterpret_cast<sockaddr_in*>(result->ai_addr);
        address->SetIPv4(addr_in->sin_addr.s_addr, addr_in->sin_port);
        address->family = HAIKU_SOCKET_FAMILY_INET;
    } else if (result->ai_family == AF_INET6) {
        struct sockaddr_in6* addr_in6 = reinterpret_cast<sockaddr_in6*>(result->ai_addr);
        address->SetIPv6(addr_in6->sin6_addr);
        address->family = HAIKU_SOCKET_FAMILY_INET6;
    }
    
    freeaddrinfo(result);
    
    printf("[HAIKU_NETWORK] ✅ Resolved hostname: %s -> %s\n",
           hostname, address->GetIPString().c_str());
    
    return B_OK;
}

std::string HaikuNetworkKitImpl::GetAddressString(const HaikuNetAddress& address) {
    std::ostringstream oss;
    oss << address.GetIPString();
    if (address.port != 0) {
        oss << ":" << address.port;
    }
    return oss.str();
}

void HaikuNetworkKitImpl::DeleteAddress(uint32_t address_id) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = addresses.find(address_id);
    if (it != addresses.end()) {
        printf("[HAIKU_NETWORK] 🗑️  Deleted address %u\n", address_id);
        delete it->second;
        addresses.erase(it);
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

std::string HaikuNetworkKitImpl::InternalResolveHostname(const char* hostname, struct sockaddr* addr, socklen_t* addrlen) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int ret = getaddrinfo(hostname, nullptr, &hints, &result);
    if (ret != 0) {
        return B_ERROR;
    }
    
    // Copy first result
    memcpy(addr, result->ai_addr, result->ai_addrlen);
    *addrlen = result->ai_addrlen;
    
    freeaddrinfo(result);
    return B_OK;
}

bool HaikuNetworkKitImpl::IsSocketConnected(uint32_t socket_id) const {
    if (!initialized) return false;
    
    std::lock_guard<std::mutex> lock(network_mutex);
    
    auto it = sockets.find(socket_id);
    if (it != sockets.end()) {
        return it->second->is_connected;
    }
    
    return false;
}

std::string HaikuNetworkKitImpl::FormatHttpRequest(const HaikuHttpRequest& request) {
    std::ostringstream oss;
    oss << request.method << " " << request.url;
    
    // Add headers
    for (const auto& header : request.headers) {
        oss << "\r\n" << header.first << ": " << header.second;
    }
    
    // Add body if present
    if (request.body.size > 0) {
        oss << "\r\n\r\n";
        oss.write(reinterpret_cast<const char*>(request.body.data), request.body.size);
    }
    
    return oss.str();
}

bool HaikuNetworkKitImpl::ParseUrl(const std::string& url_string, HaikuUrl* haiku_url) {
    // Parse URL: protocol://host:port/path?query#fragment
    // Example: https://www.example.com:8080/path/to/resource?param=value#fragment
    
    if (url_string.empty()) {
        return false;
    }
    
    std::string::size_t protocol_end = url_string.find("://");
    if (protocol_end == std::string::npos) {
        return false;
    }
    
    std::string::size_t host_end = url_string.find("/", protocol_end + 3);
    if (host_end == std::string::npos) {
        host_end = url_string.find("?", protocol_end + 3);
        if (host_end == std::string::npos) {
            host_end = url_string.find("#", protocol_end + 3);
        }
        if (host_end == std::string::npos) {
            host_end = url_string.length();
        }
    }
    
    haiku_url->protocol = url_string.substr(0, protocol_end);
    std::string host_port = url_string.substr(protocol_end + 3, host_end - protocol_end - 3);
    
    // Parse host and port
    std::string::size_t colon_pos = host_port.find(":");
    if (colon_pos != std::string::npos) {
        haiku_url->host = host_port.substr(0, colon_pos);
        haiku_url->port = std::stoi(host_port.substr(colon_pos + 1));
    } else {
        haiku_host_port = host_port;
        haiku_url->port = 80; // Default HTTP port
    }
    
    // Extract path, query, and fragment
    std::string remaining = url_string.substr(host_end);
    std::string::size_t query_pos = remaining.find("?");
    std::string::size_t fragment_pos = remaining.find("#");
    
    std::string::size_t path_end = std::string::npos;
    if (query_pos != std::string::npos) {
        path_end = query_pos;
    } else if (fragment_pos != std::string::npos) {
        path_end = fragment_pos;
    } else {
        path_end = remaining.length();
    }
    
    haiku_url->path = remaining.substr(0, path_end);
    
    if (query_pos != std::string::npos) {
        std::string::size_t fragment_pos2 = remaining.find("#", query_pos + 1);
        haiku_url->query = remaining.substr(query_pos + 1, 
                                   fragment_pos2 == std::string::npos ? remaining.length() : fragment_pos2 - query_pos - 1);
    } else {
        haiku_url->query = "";
    }
    
    if (fragment_pos != std::string::npos) {
        haiku_url->fragment = remaining.substr(fragment_pos + 1);
    } else {
        haiku_url->fragment = "";
    }
    
    // Set protocol defaults
    if (haiku_url->protocol.empty()) {
        haiku_url->protocol = HAIKU_URL_PROTOCOL_HTTP;
    }
    
    // Detect HTTPS
    if (haiku_url->protocol == HAIKU_URL_PROTOCOL_HTTPS) {
        haiku_url->is_secure = true;
        haiku_url->port = haiku_url->port == 80 ? 443 : haiku_url->port;
    } else {
        haiku_url->is_secure = false;
    }
    
    return true;
}

void HaikuNetworkKitImpl::CleanupResources() {
    // This would be called to clean up any stale resources
    // Implementation depends on specific needs
}

// C compatibility wrapper
extern "C" {
    HaikuNetworkKit* GetHaikuNetworkKit() {
        return &HaikuNetworkKitImpl::GetInstance();
    }
}