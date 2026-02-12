/*
 * HaikuNetworkKit.h - Complete Haiku Network Kit Interface
 * 
 * Interface for all Haiku network operations: BNetAddress, BNetBuffer, BNetEndpoint, BUrl, BHttpRequest
 * Provides cross-platform Haiku network functionality
 */

#pragma once

#include "HaikuAPIVirtualizer.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Haiku Network Kit constants
#define HAIKU_MAX_SOCKETS           256
#define HAIKU_MAX_URL_LENGTH         2048
#define HAIKU_MAX_HOST_NAME         256
#define HAIKU_MAX_NET_BUFFER_SIZE   65536
#define HAIKU_MAX_HEADER_COUNT      64
#define HAIKU_MAX_DNS_RESPONSE     1024

// Haiku socket types
#define HAIKU_SOCKET_TYPE_STREAM      SOCK_STREAM
#define HAIKU_SOCKET_TYPE_DGRAM       SOCK_DGRAM
#define HAIKU_SOCKET_TYPE_RAW         SOCK_RAW

// Haiku socket families
#define HAIKU_SOCKET_FAMILY_INET      AF_INET
#define HAIKU_SOCKET_FAMILY_INET6     AF_INET6
#define HAIKU_SOCKET_FAMILY_UNIX      AF_UNIX

// Haiku network protocols
#define HAIKU_PROTOCOL_TCP            IPPROTO_TCP
#define HAIKU_PROTOCOL_UDP            IPPROTO_UDP
#define HAIKU_PROTOCOL_ICMP           IPPROTO_ICMP
#define HAIKU_PROTOCOL_RAW            IPPROTO_RAW

// Haiku URL protocols
#define HAIKU_URL_PROTOCOL_HTTP         "http"
#define HAIKU_URL_PROTOCOL_HTTPS        "https"
#define HAIKU_URL_PROTOCOL_FTP          "ftp"
#define HAIKU_URL_PROTOCOL_FILE         "file"

// Haiku HTTP methods
#define HAIKU_HTTP_METHOD_GET           "GET"
#define HAIKU_HTTP_METHOD_POST          "POST"
#define HAIKU_HTTP_METHOD_PUT           "PUT"
#define HAIKU_HTTP_METHOD_DELETE        "DELETE"
#define HAIKU_HTTP_METHOD_HEAD          "HEAD"

// Haiku HTTP status codes
#define HAIKU_HTTP_STATUS_OK            200
#define HAIKU_HTTP_STATUS_NOT_FOUND     404
#define HAIKU_HTTP_STATUS_INTERNAL_ERROR 500
#define HAIKU_HTTP_STATUS_UNAUTHORIZED 401
#define HAIKU_HTTP_STATUS_FORBIDDEN    403

// ============================================================================
// HAIKU NETWORK KIT DATA STRUCTURES
// ============================================================================

/**
 * Haiku network address information
 */
struct HaikuNetAddress {
    char host[HAIKU_MAX_HOST_NAME];
    uint16_t port;
    uint32_t family;
    uint32_t ip4_address;
    char ip6_address[46];
    uint32_t id;
    
    HaikuNetAddress() : port(0), family(0), ip4_address(0), id(0) {
        for (size_t i = 0; i < sizeof(host); i++) host[i] = 0;
        for (size_t i = 0; i < sizeof(ip6_address); i++) ip6_address[i] = 0;
    }
    
    bool IsValid() const {
        return family != 0 && port > 0;
    }
    
    const char* GetFamilyString() const {
        switch (family) {
            case HAIKU_SOCKET_FAMILY_INET: return "IPv4";
            case HAIKU_SOCKET_FAMILY_INET6: return "IPv6";
            case HAIKU_SOCKET_FAMILY_UNIX: return "Unix";
            default: return "Unknown";
        }
    }
    
    void SetIPv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        ip4_address = (a << 24) | (b << 16) | (c << 8) | d;
        family = HAIKU_SOCKET_FAMILY_INET;
    }
    
    void SetIPv6(const uint8_t* address) {
        if (address) {
            memcpy(ip6_address, address, 16);
            family = HAIKU_SOCKET_FAMILY_INET6;
        }
    }
    
    std::string GetIPString() const {
        if (family == HAIKU_SOCKET_FAMILY_INET) {
            uint8_t a = (ip4_address >> 24) & 0xFF;
            uint8_t b = (ip4_address >> 16) & 0xFF;
            uint8_t c = (ip4_address >> 8) & 0xFF;
            uint8_t d = ip4_address & 0xFF;
            return std::to_string(a) + "." + std::to_string(b) + "." + std::to_string(c) + "." + std::to_string(d);
        }
        return "";
    }
};

/**
 * Haiku network buffer information
 */
struct HaikuNetBuffer {
    uint8_t* data;
    size_t size;
    size_t capacity;
    size_t position;
    bool owns_data;
    uint32_t id;
    
    HaikuNetBuffer() : data(nullptr), size(0), capacity(0), position(0), owns_data(false), id(0) {}
    
    ~HaikuNetBuffer() {
        if (owns_data && data) {
            delete[] data;
        }
    }
    
    bool IsValid() const {
        return data != nullptr && capacity > 0;
    }
    
    size_t Available() const {
        return capacity - position;
    }
    
    size_t Remaining() const {
        return size - position;
    }
};

/**
 * Haiku network endpoint information
 */
struct HaikuNetEndpoint {
    int socket_fd;
    HaikuNetAddress local_address;
    HaikuNetAddress remote_address;
    uint32_t socket_type;
    uint32_t socket_family;
    uint32_t socket_protocol;
    bool is_connected;
    bool is_bound;
    bool is_listening;
    uint32_t id;
    
    HaikuNetEndpoint() : socket_fd(-1), socket_type(0), socket_family(0), 
                        socket_protocol(0), is_connected(false), is_bound(false), 
                        is_listening(false), id(0) {}
    
    ~HaikuNetEndpoint() {
        if (socket_fd >= 0) {
            close(socket_fd);
        }
    }
    
    bool IsValid() const {
        return socket_fd >= 0;
    }
};

/**
 * Haiku URL information
 */
struct HaikuUrl {
    std::string protocol;
    std::string host;
    std::string path;
    std::string query;
    std::string fragment;
    uint16_t port;
    bool is_secure;
    uint32_t id;
    
    HaikuUrl() : port(0), is_secure(false), id(0) {}
    
    bool IsValid() const {
        return !protocol.empty() && !host.empty();
    }
    
    std::string GetFullUrl() const {
        std::string url = protocol + "://";
        url += host;
        if (port != 0) {
            url += ":" + std::to_string(port);
        }
        if (!path.empty()) {
            url += path;
        }
        if (!query.empty()) {
            url += "?" + query;
        }
        if (!fragment.empty()) {
            url += "#" + fragment;
        }
        return url;
    }
    
    void Parse(const std::string& url_string);
};

/**
 * Haiku HTTP request information
 */
struct HaikuHttpRequest {
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    HaikuNetBuffer body;
    std::map<std::string, std::string> response_headers;
    int status_code;
    HaikuNetBuffer response_body;
    uint32_t id;
    
    HaikuHttpRequest() : status_code(0), id(0) {}
    
    bool IsValid() const {
        return !method.empty() && !url.empty();
    }
    
    void AddHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }
    
    std::string GetHeader(const std::string& key) const {
        auto it = headers.find(key);
        return it != headers.end() ? it->second : "";
    }
    
    void AddResponseHeader(const std::string& key, const std::string& value) {
        response_headers[key] = value;
    }
    
    std::string GetResponseHeader(const std::string& key) const {
        auto it = response_headers.find(key);
        return it != response_headers.end() ? it->second : "";
    }
};

/**
 * DNS query information
 */
struct HaikuDnsQuery {
    std::string hostname;
    uint32_t query_type;  // A, AAA, CNAME, MX, etc.
    std::vector<std::string> results;
    uint32_t id;
    
    HaikuDnsQuery() : query_type(0), id(0) {}
    
    bool IsValid() const {
        return !hostname.empty();
    }
};

// ============================================================================
// HAIKU NETWORK KIT INTERFACE
// ============================================================================

/**
 * Haiku Network Kit implementation class
 * 
 * Provides complete Haiku network functionality including:
 * - Socket operations (TCP/UDP sockets)
 * - Address management (BNetAddress)
 * - Network buffering (BNetBuffer)
 * - HTTP client functionality (BHttpRequest)
 * - DNS resolution (DNS queries)
 * - URL parsing and handling
 */
class HaikuNetworkKitImpl : public HaikuKit {
private:
    // Network resources
    std::map<uint32_t, HaikuNetEndpoint*> sockets;
    std::map<uint32_t, HaikuNetBuffer*> buffers;
    std::map<uint32_t, HaikuUrl*> urls;
    std::map<uint32_t, HaikuHttpRequest*> requests;
    std::map<uint32_t, HaikuNetAddress*> addresses;
    
    // DNS cache
    std::map<std::string, HaikuDnsQuery> dns_cache;
    
    // ID management
    uint32_t next_socket_id;
    uint32_t next_buffer_id;
    uint32_t next_url_id;
    uint32_t next_request_id;
    uint32_t next_dns_query_id;
    
    // Network state
    bool is_initialized;
    mutable std::mutex network_mutex;
    
    // Network statistics
    struct NetworkStats {
        uint32_t sockets_created;
        uint32_t connections_made;
        uint32_t bytes_sent;
        uint32_t bytes_received;
        uint32_t http_requests;
        uint32_t dns_queries;
    } network_stats;
    
public:
    /**
     * Constructor
     */
    HaikuNetworkKitImpl();
    
    /**
     * Destructor
     */
    virtual ~HaikuNetworkKitImpl();
    
    // HaikuKit interface
    virtual status_t Initialize() override;
    virtual void Shutdown() override;
    
    /**
     * Get singleton instance
     */
    static HaikuNetworkKitImpl& GetInstance();
    
    // ========================================================================
    // SOCKET OPERATIONS
    // ========================================================================
    
    /**
     * Create a new socket
     */
    virtual uint32_t CreateSocket(uint32_t family, uint32_t type, uint32_t protocol);
    
    /**
     * Connect socket to address
     */
    virtual status_t ConnectSocket(uint32_t socket_id, const char* address, uint16_t port);
    
    /**
     * Bind socket to address
     */
    virtual status_t BindSocket(uint32_t socket_id, const char* address, uint16_t port);
    
    /**
     * Listen for connections
     */
    virtual status_t ListenSocket(uint32_t socket_id, int32_t backlog);
    
    /**
     * Accept connection
     */
    virtual uint32_t AcceptSocket(uint32_t socket_id, char* client_address, uint16_t* port);
    
    /**
     * Close socket
     */
    virtual status_t CloseSocket(uint32_t socket_id);
    
    // ========================================================================
    // DATA TRANSFER
    // ========================================================================
    
    /**
     * Send data through socket
     */
    virtual ssize_t SendSocket(uint32_t socket_id, const void* buffer, size_t size, uint32_t flags);
    
    /**
     * Receive data from socket
     */
    virtual ssize_t ReceiveSocket(uint32_t socket_id, void* buffer, size_t size, uint32_t flags);
    
    /**
     * Send to address (UDP)
     */
    virtual ssize_t SendToSocket(uint32_t socket_id, const void* buffer, size_t size,
                                uint32_t flags, const char* address, uint16_t port);
    
    /**
     * Receive from address (UDP)
     */
    virtual ssize_t ReceiveFromSocket(uint32_t socket_id, void* buffer, size_t size, uint32_t flags,
                                      char* source_address, uint16_t* source_port);
    
    /**
     * Set socket options
     */
    virtual status_t SetSocketOption(uint32_t socket_id, int option, int value);
    
    /**
     * Get socket options
     */
    virtual int GetSocketOption(uint32_t socket_id, int option);
    
    // ========================================================================
    // ADDRESS OPERATIONS
    // ========================================================================
    
    /**
     * Create network address
     */
    virtual uint32_t CreateAddress(const char* host, uint16_t port, uint32_t family = HAIKU_SOCKET_FAMILY_INET);
    
    /**
     * Resolve hostname to IP
     */
    virtual status_t ResolveHostname(const char* hostname, HaikuNetAddress* address);
    
    /**
     * Get address string representation
     */
    virtual std::string GetAddressString(const HaikuNetAddress& address);
    
    /**
     * Delete address
     */
    virtual void DeleteAddress(uint32_t address_id);
    
    // ========================================================================
    // BUFFER OPERATIONS
    // ========================================================================
    
    /**
     * Create network buffer
     */
    virtual uint32_t CreateBuffer(size_t initial_capacity = HAIKU_MAX_NET_BUFFER_SIZE);
    
    /**
     * Write to buffer
     */
    virtual status_t WriteToBuffer(uint32_t buffer_id, const void* data, size_t size);
    
    /**
     * Read from buffer
     */
    virtual size_t ReadFromBuffer(uint32_t buffer_id, void* data, size_t max_size);
    
    /**
     * Set buffer position
     */
    virtual status_t SetBufferPosition(uint32_t buffer_id, size_t position);
    
    /**
     * Get buffer size
     */
    virtual size_t GetBufferSize(uint32_t buffer_id);
    
    /**
     * Delete buffer
     */
    virtual void DeleteBuffer(uint32_t buffer_id);
    
    // ========================================================================
    // HTTP OPERATIONS
    // ========================================================================
    
    /**
     * Create HTTP request
     */
    virtual uint32_t CreateHttpRequest(const char* url, const char* method = HAIKU_HTTP_METHOD_GET);
    
    /**
     * Execute HTTP request
     */
    virtual status_t ExecuteHttpRequest(uint32_t request_id);
    
    /**
     * Get HTTP response status
     */
    virtual int32_t GetHttpResponseStatus(uint32_t request_id);
    
    /**
     * Get HTTP response body
     */
    virtual std::string GetHttpResponseBody(uint32_t request_id);
    
    /**
     * Delete HTTP request
     */
    virtual void DeleteHttpRequest(uint32_t request_id);
    
    // ========================================================================
    // DNS OPERATIONS
    // ========================================================================
    
    /**
     * Create DNS query
     */
    virtual uint32_t CreateDnsQuery(const char* hostname, uint32_t query_type = 1);  // A record by default
    
    /**
     * Execute DNS query
     */
    virtual status_t ExecuteDnsQuery(uint32_t query_id);
    
    /**
     * Get DNS query results
     */
    virtual std::vector<std::string> GetDnsResults(uint32_t query_id);
    
    /**
     * Delete DNS query
     */
    virtual void DeleteDnsQuery(uint32_t query_id);
    
    /**
     * Clear DNS cache
     */
    virtual void ClearDnsCache();
    
    // ========================================================================
    // UTILITY METHODS
    // ========================================================================
    
    /**
     * Get network statistics
     */
    virtual void GetNetworkStatistics(uint32_t* socket_count, uint32_t* buffer_count,
                                     uint32_t* request_count, uint32_t* dns_cache_size) const;
    
    /**
     * Get network statistics detailed
     */
    virtual void GetDetailedNetworkStats(NetworkStats* stats) const;
    
    /**
     * Dump network state for debugging
     */
    virtual void DumpNetworkState() const;
    
    /**
     * Test network connectivity
     */
    virtual status_t TestConnectivity(const char* host, uint16_t port);
    
private:
    /**
     * Resolve hostname (internal implementation)
     */
    status_t InternalResolveHostname(const char* hostname, struct sockaddr* addr, socklen_t* addrlen);
    
    /**
     * Parse HTTP response
     */
    status_t ParseHttpResponse(uint32_t request_id, const char* response_data);
    
    /**
     * Format HTTP request
     */
    std::string FormatHttpRequest(const HaikuHttpRequest& request);
    
    /**
     * Parse URL
     */
    bool ParseUrl(const std::string& url, HaikuUrl* haiku_url);
    
    /**
     * Check if socket is connected
     */
    bool IsSocketConnected(uint32_t socket_id) const;
    
    /**
     * Clean up resources
     */
    void CleanupResources();
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

namespace HaikuNetworkUtils {
    /**
     * Network utility functions
     */
    std::string GetLocalIPAddress();
    std::string GetMacAddress();
    bool IsPortAvailable(uint16_t port);
    std::vector<std::string> GetNetworkInterfaces();
    bool IsValidIpAddress(const std::string& ip);
    std::string FormatSize(size_t bytes);
    std::string FormatSpeed(double bytes_per_second);
}