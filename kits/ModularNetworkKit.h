#pragma once

#include "HaikuEmulationFramework.h"
#include <vector>
#include <string>
#include <map>

namespace HaikuEmulation {

/////////////////////////////////////////////////////////////////////////////
// Modular NetworkKit - Universal Internet Kit
/////////////////////////////////////////////////////////////////////////////

class ModularNetworkKit : public UniversalKit<0x03, "NetworkKit", "1.0.0"> {
public:
    // Network address
    struct NetworkAddress {
        std::string hostname;
        std::string ip_address;
        uint16_t port;
        bool is_ipv6;
        bool resolved;
        
        NetworkAddress() : port(0), is_ipv6(false), resolved(false) {}
    };
    
    // Network connection
    struct NetworkConnection {
        int32_t connection_id;
        NetworkAddress remote_address;
        NetworkAddress local_address;
        bool is_connected;
        bool is_server;
        uint32_t timeout_ms;
        std::string protocol;
        void* native_endpoint;
        
        NetworkConnection() : connection_id(-1), is_connected(false), is_server(false), 
                              timeout_ms(30000), native_endpoint(nullptr) {}
    };
    
    // Network packet
    struct NetworkPacket {
        std::vector<uint8_t> data;
        NetworkAddress source;
        NetworkAddress destination;
        uint32_t protocol;
        uint64_t timestamp;
        
        NetworkPacket() : protocol(0), timestamp(0) {}
    };
    
    // Protocol constants
    static constexpr uint32_t PROTOCOL_TCP = 0x01;
    static constexpr uint32_t PROTOCOL_UDP = 0x02;
    static constexpr uint32_t PROTOCOL_ICMP = 0x03;
    static constexpr uint32_t PROTOCOL_HTTP = 0x04;
    static constexpr uint32_t PROTOCOL_HTTPS = 0x05;
    static constexpr uint32_t PROTOCOL_FTP = 0x06;
    static constexpr uint32_t PROTOCOL_SMTP = 0x07;
    static constexpr uint32_t PROTOCOL_DNS = 0x08;
    
    // Syscall numbers
    static constexpr uint32_t SYSCALL_INIT_NETWORK = 0x030001;
    static constexpr uint32_t SYSCALL_CLEANUP_NETWORK = 0x030002;
    static constexpr uint32_t SYSCALL_GET_NETWORK_INTERFACES = 0x030003;
    static constexpr uint32_t SYSCALL_GET_HOST_BY_NAME = 0x030004;
    static constexpr uint32_t SYSCALL_GET_HOST_BY_ADDR = 0x030005;
    
    static constexpr uint32_t SYSCALL_RESOLVE_ADDRESS = 0x030100;
    static constexpr uint32_t SYSCALL_CREATE_CONNECTION = 0x030101;
    static constexpr uint32_t SYSCALL_DESTROY_CONNECTION = 0x030102;
    static constexpr uint32_t SYSCALL_CONNECT = 0x030103;
    static constexpr uint32_t SYSCALL_DISCONNECT = 0x030104;
    static constexpr uint32_t SYSCALL_LISTEN = 0x030105;
    static constexpr uint32_t SYSCALL_ACCEPT = 0x030106;
    
    static constexpr uint32_t SYSCALL_SEND_DATA = 0x030200;
    static constexpr uint32_t SYSCALL_RECEIVE_DATA = 0x030201;
    static constexpr uint32_t SYSCALL_SEND_PACKET = 0x030202;
    static constexpr uint32_t SYSCALL_RECEIVE_PACKET = 0x030203;
    static constexpr uint32_t SYSCALL_BROADCAST_PACKET = 0x030204;
    
    static constexpr uint32_t SYSCALL_HTTP_GET = 0x030300;
    static constexpr uint32_t SYSCALL_HTTP_POST = 0x030301;
    static constexpr uint32_t SYSCALL_HTTP_PUT = 0x030302;
    static constexpr uint32_t SYSCALL_HTTP_DELETE = 0x030303;
    static constexpr uint32_t SYSCALL_HTTP_HEAD = 0x030304;
    static constexpr uint32_t SYSCALL_SET_HTTP_HEADERS = 0x030305;
    
    static constexpr uint32_t SYSCALL_DNS_QUERY = 0x030400;
    static constexpr uint32_t SYSCALL_DNS_REVERSE_QUERY = 0x030401;
    static constexpr uint32_t SYSCALL_SET_DNS_SERVER = 0x030402;
    static constexpr uint32_t SYSCALL_FLUSH_DNS_CACHE = 0x030403;
    
    // IEmulationKit implementation
    bool Initialize() override;
    void Shutdown() override;
    
    bool HandleSyscall(uint32_t syscall_num, uint32_t* args, uint32_t* result) override;
    std::vector<uint32_t> GetSupportedSyscalls() const override;
    
    // Network system management
    bool InitNetwork();
    void CleanupNetwork();
    std::vector<std::string> GetNetworkInterfaces();
    bool GetHostByName(const std::string& hostname, NetworkAddress& address);
    bool GetHostByAddr(const std::string& ip_address, std::string& hostname);
    
    // Address resolution
    bool ResolveAddress(const std::string& hostname, uint16_t port, NetworkAddress& address);
    bool ResolveAddressIPv6(const std::string& hostname, uint16_t port, NetworkAddress& address);
    bool ReverseResolve(const NetworkAddress& address, std::string& hostname);
    
    // Connection management
    bool CreateConnection(const std::string& protocol, const NetworkAddress& remote_address, 
                          int32_t* connection_id);
    bool DestroyConnection(int32_t connection_id);
    bool Connect(int32_t connection_id);
    bool Disconnect(int32_t connection_id);
    bool Listen(int32_t connection_id, uint32_t backlog);
    bool Accept(int32_t connection_id, int32_t* client_connection_id);
    
    // Data transfer
    bool SendData(int32_t connection_id, const void* data, size_t size);
    bool ReceiveData(int32_t connection_id, void* buffer, size_t buffer_size, size_t* bytes_received);
    bool SendPacket(int32_t connection_id, const NetworkPacket& packet);
    bool ReceivePacket(int32_t connection_id, NetworkPacket& packet);
    bool BroadcastPacket(uint32_t protocol, const NetworkPacket& packet);
    
    // HTTP operations
    bool HttpGet(const std::string& url, std::string& response, std::map<std::string, std::string>& headers);
    bool HttpPost(const std::string& url, const std::string& data, std::string& response, 
                  std::map<std::string, std::string>& headers);
    bool HttpPut(const std::string& url, const std::string& data, std::string& response);
    bool HttpDelete(const std::string& url, std::string& response);
    bool HttpHead(const std::string& url, std::map<std::string, std::string>& headers);
    bool SetHttpHeaders(int32_t connection_id, const std::map<std::string, std::string>& headers);
    
    // DNS operations
    bool DnsQuery(const std::string& domain, uint32_t record_type, std::vector<std::string>& results);
    bool DnsReverseQuery(const std::string& ip_address, std::string& domain);
    bool SetDnsServer(const std::string& dns_server);
    bool FlushDnsCache();
    
    // State management
    bool SaveState(void** data, size_t* size) override;
    bool LoadState(const void* data, size_t size) override;
    
    // Network monitoring
    bool EnableNetworkMonitoring(bool enable);
    bool IsNetworkMonitoringEnabled() const;
    std::vector<NetworkPacket> GetPacketHistory(uint32_t max_packets = 100);
    void ClearPacketHistory();
    
    // Security
    bool EnableSslTls(bool enable);
    bool IsSslTlsEnabled() const;
    bool SetCertificate(const std::string& cert_file, const std::string& key_file);
    bool VerifyCertificate(const std::string& hostname);

private:
    // Network system state
    bool network_initialized;
    std::vector<std::string> network_interfaces;
    std::string dns_server;
    
    // Connection management
    std::map<int32_t, NetworkConnection> connections;
    int32_t next_connection_id;
    
    // DNS cache
    std::map<std::string, NetworkAddress> dns_cache;
    std::map<std::string, std::string> reverse_dns_cache;
    
    // HTTP state
    std::map<int32_t, std::map<std::string, std::string>> http_headers;
    bool ssl_tls_enabled;
    std::string cert_file;
    std::string key_file;
    
    // Network monitoring
    bool network_monitoring_enabled;
    std::vector<NetworkPacket> packet_history;
    size_t max_packet_history;
    
    // Native Haiku integration
    void* native_net_server;
    std::map<int32_t, void*> native_endpoints;
    
    // Internal methods
    void InitializeNativeHaiku();
    void InitializeSslTls();
    void InitializeNetworkMonitoring();
    void CleanupNativeHaiku();
    void CleanupSslTls();
    void CleanupNetworkMonitoring();
    
    // Protocol handlers
    bool HandleTcpConnection(int32_t connection_id);
    bool HandleUdpConnection(int32_t connection_id);
    bool HandleHttpRequest(const std::string& method, const std::string& url, 
                          const std::string& data, std::string& response);
    
    // Address utilities
    bool IsValidIpAddress(const std::string& ip);
    bool IsValidHostname(const std::string& hostname);
    std::string UrlEncode(const std::string& str);
    std::string UrlDecode(const std::string& str);
    
    // Packet monitoring
    void LogPacket(const NetworkPacket& packet);
    void FilterPacket(NetworkPacket& packet);
    
    // Configuration hooks
    bool OnConfigure(const std::map<std::string, std::string>& config) override;
    
    // SSL/TLS utilities
    bool CreateSslContext();
    void DestroySslContext();
    bool VerifySslCertificate(const std::string& hostname);
};

/////////////////////////////////////////////////////////////////////////////
// Auto-registration
/////////////////////////////////////////////////////////////////////////////

HAIKU_REGISTER_KIT(ModularNetworkKit);

} // namespace HaikuEmulation