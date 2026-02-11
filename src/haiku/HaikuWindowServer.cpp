/*
 * HaikuWindowServer.cpp - Servidor de Ventanas de Haiku OS
 * 
 * Implementa el app_server emulado de Haiku para que las aplicaciones invitadas
 * puedan usar las interfaces nativas BWindow, BApplication, etc.
 */

#include "HaikuWindowServer.h"
#include "HaikuOSIPCSystem.h"
#include "Phase4GUISyscalls.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <mutex>

// Implementación del servidor emulado
class HaikuWindowServer {
private:
    int server_socket;
    uint32_t next_client_id;
    std::thread server_thread;
    bool running;
    std::mutex server_mutex;
    
    // Configuración
    struct ServerConfig {
        uint32_t display_width = 1024;
        uint32_t display_height = 768;
        uint32_t port = 53000; // Puerto para app_server
        std::string app_signature = "application/x-vnd.beos-haiku";
        bool debug_mode = false;
    };
    
    ServerConfig config;
    
    // Métodos privados
    void RunServerLoop();
    void HandleClientConnection(int client_socket);
    void HandleClientRequest(HaikuAppClient* client, const std::string& request);
    status_t ProcessDesktopLinkRequest(HaikuAppClient* client, const std::string& request);
    status_t ProcessServerProtocolRequest(HaikuAppClient* client, const std::string& request);
    
    // Protocolos simplificados
    std::string GenerateDesktopLinkResponse(const std::string& command, const std::string& status);
    std::string GenerateServerProtocolResponse(const std::string& command, const std::string& status);
    
    // Envío de syscalls a la VM
    status_t SendSyscallToGuest(const HaikuMessage& msg);
    
    // Validaciones
    bool IsValidRequest(const std::string& request);
    bool IsValidDesktopLinkCommand(const std::string& command);

public:
    HaikuWindowServer(const ServerConfig& cfg = ServerConfig());
    ~HaikuWindowServer();
    
    status_t Initialize();
    void Shutdown();
    
    bool IsRunning() const { return running; }
    uint32_t GetWindowCount() const { return windows.size(); }
    uint32_t GetClientCount() const { return clients.size(); }
    
    // Estado del servidor
    void PrintServerInfo() const;
    
    // Métodos públicos para manejo de ventanas
    uint32_t CreateWindow(const std::string& title, uint32_t width, uint32_t height);
    status_t DestroyWindow(uint32_t window_id);
    HaikuNativeWindow* GetWindow(uint32_t window_id);
    
    // Métodos para envío de eventos
    void HandleMouseEvent(const HaikuMessage& msg);
    void HandleKeyboardEvent(const HaikuMessage& msg);
    void HandleFocusEvent(const HaikuMessage& msg);
};

// Instancia global
HaikuWindowServer* g_haiku_server = nullptr;

// Implementación principal
HaikuWindowServer::HaikuWindowServer(const ServerConfig& cfg) 
    : config(cfg), server_socket(-1), next_client_id(1), 
      running(false), server_mutex() {
    
    printf("[HAIKU_SERVER] Inicializando servidor de ventanas...\n");
    printf("[HAIKU_SERVER] Display: %ux%u, Puerto: %u\n", 
           config.display_width, config.display_height, config.port);
    printf("[HAIKU_SERVER] Debug mode: %s\n", config.debug_mode ? "ON" : "OFF");
}

HaikuWindowServer::~HaikuWindowServer() {
    Shutdown();
}

status_t HaikuWindowServer::Initialize() {
    std::lock_guard<std::mutex> lock(server_mutex);
    
    if (running) {
        printf("[HAIKU_SERVER] ERROR: Servidor ya está corriendo\n");
        return B_ERROR;
    }
    
    // Crear socket del servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("[HAIKU_SERVER] ERROR: No se pudo crear socket del servidor\n");
        return B_ERROR;
    }
    
    // Configurar socket para reusar
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Enlazar a localhost
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.port);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("[HAIKU_SERVER] ERROR: No se pudo enlazar socket del servidor\n");
        close(server_socket);
        return B_ERROR;
    }
    
    // Escuchar para conexiones entrantes
    if (listen(server_socket, 5) < 0) {
        printf("[HAIKU_SERVER] ERROR: No se pudo poner socket en modo escucha\n");
        close(server_socket);
        return B_ERROR;
    }
    
    printf("[HAIKU_SERVER] Servidor escuchando en puerto %u\n", config.port);
    
    running = true;
    server_thread = std::thread(&HaikuWindowServer::RunServerLoop, this);
    
    return B_OK;
}

void HaikuWindowServer::Shutdown() {
    if (!running) return;
    
    printf("[HAIKU_SERVER] Deteniendo servidor de ventanas...\n");
    running = false;
    
    // Cerrar socket del servidor
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
    
    // Esperar que termine el hilo del servidor
    if (server_thread.joinable()) {
        server_thread.join();
    }
    
    printf("[HAIKU_SERVER] Servidor detenido\n");
}

void HaikuWindowServer::RunServerLoop() {
    printf("[HAIKU_SERVER] Iniciando bucle del servidor...\n");
    
    // Bucle principal del servidor
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);
        
        if (client_socket < 0) {
            if (config.debug_mode) {
                printf("[HAIKU_SERVER] Error al aceptar conexión: %d\n", errno);
            }
            continue;
        }
        
        printf("[HAIKU_SERVER] Cliente conectado desde %s:%u\n", 
               inet_ntoa(client_addr.sin_addr));
        
        // Crear cliente
        auto client = std::make_unique<HaikuAppClient>(next_client_id++, client_socket);
        
        clients[client->GetClientID()] = client;
        
        printf("[HAIKU_SERVER] Registrando cliente %d\n", client->GetClientID());
        
        // Manejar cliente en hilo separado
        std::thread client_thread(&HaikuWindowServer::HandleClientConnection, client.get());
        
        client_thread.detach();
    }
}

void HaikuServer::HandleClientConnection(int client_socket) {
    char buffer[4096];
    
    // Leer mensaje inicial del cliente
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received <= 0) {
        if (config.debug_mode) {
            printf("[HAIKU_SERVER] Cliente %d: error en conexión\n", client_socket);
        }
        close(client_socket);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    // Esperar solicitud de DesktopLink
    bool got_request = false;
    std::string request_line;
    size_t request_start = 0;
    
    while (bytes_received < sizeof(buffer) - 1) {
        ssize_t bytes_read = recv(client_socket, buffer + bytes_received, sizeof(buffer) - bytes_received - 1, 0);
        
        if (bytes_read <= 0) {
            if (config.debug_mode) {
                printf("[HAIKU_SERVER] Error al leer de cliente %d\n", client_socket);
            }
            break;
        }
        
        buffer[bytes_received + bytes_read] = '\0';
        
        // Buscar línea completa
        for (size_t i = request_start; i < bytes_received + bytes_read; i++) {
            if (buffer[i] == '\n') {
                request_line = std::string(buffer, request_start, i - request_start);
                request_start = i + 1;
                got_request = true;
                break;
            }
        }
        
        if (got_request) {
            break;
        }
        
        bytes_received += bytes_read;
    }
    
    if (!got_request) {
        if (config.debug_mode) {
            printf("[HAIKU_SERVER] Cliente %d: mensaje incompleto\n", client_socket);
        }
        close(client_socket);
        return;
    }
    
    printf("[HAIKU_SERVER] Solicitud de cliente %d: %s\n", client->GetClientID(), request_line.c_str());
    
    // Procesar solicitud
    if (request_line.empty()) return;
    
    bool is_desktop_link = IsValidDesktopLinkRequest(request_line);
    bool is_server_protocol = IsValidServerProtocolRequest(request_line);
    
    std::string response;
    if (is_desktop_link) {
        response = GenerateDesktopLinkResponse(request_line, "OK");
    } else if (is_server_protocol) {
        response = GenerateServerProtocolResponse(request_line, "OK");
    } else {
        response = "ERROR: Comando desconocido\r\n";
    }
    
    // Enviar respuesta
    ssize_t bytes_sent = send(client_socket, response.c_str(), response.length(), 0);
    if (bytes_sent < 0) {
        if (config.debug_mode) {
            printf("[HAIKU_SERVER] Error al enviar respuesta a cliente %d\n", client_socket);
        }
    } else {
        if (config.debug_mode) {
            printf("[HAIK_SERVER] Enviada respuesta a cliente %d (%zu bytes): %.*s\n", 
                   client->GetClientID(), bytes_sent, 
                   response.substr(0, 50)); // Primeros 50 chars para debugging
        }
    }
}

void HaikuServer::HandleClientRequest(HaikuAppClient* client, const std::string& request) {
    if (!IsValidRequest(request)) {
        // Enviar error
        std::string error_response = "ERROR: Solicitud inválida\r\n";
        send(client->socket_fd, error_response.c_str(), error_response.length(), 0);
        return;
    }
    
    bool is_desktop_link = IsValidDesktopLinkCommand(request);
    bool is_server_protocol = IsValidServerProtocolCommand(request);
    
    std::string response;
    if (is_desktop_link) {
        response = ProcessDesktopLinkRequest(client, request);
    } else if (is_server_protocol) {
        response = ProcessServerProtocolRequest(client, request);
    } else {
        response = "ERROR: Comando desconocido\r\n";
    }
    
    send(client->socket_fd, response.c_str(), response.length(), 0);
    
    if (config.debug_mode) {
        printf("[HAIKU_SERVER] Procesada solicitud %d: %s\n", 
               client->GetClientID(), request.c_str());
        printf("[HAIKU_SERVER] Respuesta: %.*s\n", response.substr(0, 100));
    }
}

status_t HaikuServer::ProcessDesktopLinkRequest(HaikuAppClient* client, const std::string& request) {
    // Formato: "GET /app/server/..." o "POST /app/server/..."
    
    std::string response;
    if (request.substr(0, 3) == "GET") {
        // Solicitud GET
        if (request.substr(4, 20) == "/app/server/") {
            if (request.length() > 24) {
                std::string path = request.substr(24);
                response = "Content-Type: text/plain\r\n\r\n" + path + "\r\n";
            } else {
                response = "Content-Type: text/plain\r\n\r\n/ (Directorio raíz)\r\n";
            }
        } else if (request.substr(4, 4) == "/app/") {
            response = "Content-Type: text/plain\r\n\r\n/ (Directorio raíz)\r\n";
        } else {
            response = "Content-Type: text/plain\r\n\r\n/ (Error)\r\n";
        }
    } else if (request.substr(0, 4) == "POST") {
        if (request.substr(5, 19) == "/app/server/" && request.length() > 20) {
            std::string command = request.substr(20);
            
            if (command == "d" || command == "info") {
                if (command == "d") {
                    response = "Directory: /\n";
                    // Listar contenido (simplificado)
                    response += "index.html\n";
                    response += "index.css\n";
                    response += "background.jpg\n";
                    response += "GIF\r\n\r\n";
                } else if (command == "info") {
                    response = "UserlandVM-Haiku Server v1.0\r\n\r\n";
                    response += "Modo: " + (config.debug_mode ? "Debug" : "Producción") + "\r\n";
                    response += "Windows: 0, Clients: " + std::to_string(GetClientCount()) + "\r\n";
                }
            }
        } else {
            response = "Content-Type: text/plain\r\n\r\nComando POST desconocido: " + command + "\r\n";
        }
    } else {
        response = "Content-Type: text/plain\r\n\r\nERROR: Solicitud inválida\r\n";
    }
    
    send(client->socket_fd, response.c_str(), response.length(), 0);
    
    return B_OK;
}

status_t HaikuWindowServer::ProcessServerProtocolRequest(HaikuAppClient* client, const std::request& request) {
    // Protocolo simplificado: espera comandos como "WINDOW_CREATE", "WINDOW_DESTROY", etc.
    
    std::string response;
    
    if (request.find("WINDOW_CREATE") != std::string::npos) {
        uint32_t window_id = CreateWindow(request.substr(12));
        response = GenerateServerProtocolResponse("WINDOW_CREATE", std::to_string(window_id));
        printf("[HAIKU_SERVER] Ventana creada: %u\n", window_id);
    } else if (request.find("WINDOW_DESTROY") != std::string::npos) {
        uint32_t window_id = std::stoi(request.substr(15));
        if (DestroyWindow(window_id) == B_OK) {
            response = GenerateServerProtocolResponse("WINDOW_DESTROY", "OK");
            printf("[HAIKU_SERVER] Ventana destruida: %u\n", window_id);
        } else {
            response = GenerateServerProtocolResponse("WINDOW_DESTROY", "ERROR");
        }
    } else if (request.find("DRAW_RECT") != std::string::npos) {
        response = GenerateServerProtocolResponse("DRAW_RECT", "OK");
        printf("[HAIKU_SERVER] Dibujo rectángulo solicitado\n");
    } else if (request.find("DRAW_STRING") != std::string::npos) {
        response = GenerateServerProtocolResponse("DRAW_STRING", "OK");
        printf("[HAIKU_SERVER] Dibujo string solicitado\n");
    } else if (request.find("FLUSH_DISPLAY") != std::string::npos) {
        response = GenerateServerProtocolResponse("FLUSH_DISPLAY", "OK");
        
        // Sincronizar con Phase4GUISyscalls
        SendSyscallToGuest(HaikuMessage(MessageType::FLUSH_DISPLAY, 0, {}));
        
        printf("[HAIKU_SERVER] Display sincronizado\n");
    } else {
        response = GenerateServerProtocolResponse("UNKNOWN", "ERROR");
    }
    }
    
    if (!response.empty()) {
        send(client->socket_fd, response.c_str(), response.length(), 0);
    }
    
    return B_OK;
}

status_t HaikuWindowServer::SendSyscallToGuest(const HaikuMessage& msg) {
    // Enviar syscall al sistema de la VM para que la aplicación la procese
    // Esto debería enrutar a través del dispatcher existente
    // Por ahora, solo logging
    printf("[HAIKU_SERVER] Enviando syscall a VM: tipo=%u\n", msg.type);
    printf("[HAIKU_SERVER] Datos: [%u, %u, %u, %u]\n", 
           msg.data[0], msg.data[1], msg.data[2], msg.data[3]);
    
    return B_OK;
}

std::string HaikuWindowServer::GenerateDesktopLinkResponse(const std::string& command, const std::string& status) {
    return "Content-Type: text/plain\r\n\r\n" + command + ": " + status + "\r\n";
}

std::string HaikuWindowServer::GenerateServerProtocolResponse(const std::string& command, const std::string& status) {
    return command + " " + status + "\r\n";
}

uint32_t HaikuWindowServer::CreateWindow(const std::string& title, uint32_t width, uint32_t height) {
    printf("[HAIKU_SERVER] Creando ventana: '%s' (%ux%u)\n", title.c_str(), width, height);
    
    // Crear ventana nativa
    HaikuNativeWindow* window = new HaikuNativeWindow(next_window_id++, title);
    
    if (!window) {
        printf("[HAIKU_SERVER] ERROR: No se pudo crear ventana\n");
        return 0;
    }
    
    // Inicializar ventana
    if (window->Show() != B_OK) {
        printf("[HAIKU_SERVER] ERROR: No se pudo mostrar ventana\n");
        return 0;
    }
    
    windows[window->GetID()] = window;
    
    printf("[HAIKU_SERVER] Ventana nativa creada: %u, '%s'\n", 
           window->GetID(), title.c_str());
    
    return window->GetID();
}

status_t HaikuWindowServer::DestroyWindow(uint32_t window_id) {
    printf("[HAIKU_SERVER] Destruyendo ventana: %u\n", window_id);
    
    auto it = windows.find(window_id);
    if (it != windows.end()) {
        HaikuNativeWindow* window = it->second.get();
        if (window) {
            window->Hide();
            windows.erase(it);
            
            printf("[HAIKU_SERVER] Ventana nativa destruida: %u\n", window_id);
            return B_OK;
        }
    }
    
    printf("[HAIU_SERVER] ERROR: Ventana %u no encontrada\n", window_id);
    return B_ERROR;
}

HaikuNativeWindow* HaikuWindowServer::GetWindow(uint32_t window_id) {
    auto it = windows.find(window_id);
    if (it != windows.end()) {
        return it->second.get();
    }
    return nullptr;
}

void HaikuWindowServer::HandleMouseEvent(const HaikuMessage& msg) {
    printf("[HAIKU_SERVER] Evento de ratón: botón=%u, x=%u, y=%u\n", 
           msg.data[0], msg.data[1], msg.data[2]);
    
    // Enviar evento a todas las ventanas
    for (auto& pair : windows) {
        if (pair.second->IsFocused()) {
            // Enviar evento a la ventana enfocada
            HaikuMessage focus_msg(MessageType::MOUSE_CLICKED, 0, {
                pair.first, {msg.data[0], msg.data[1], msg.data[2], msg.data[3]}
            });
            
            SendSyscallToGuest(focus_msg);
        }
    }
}

void HaikuWindowServer::HandleKeyboardEvent(const HaikuMessage& msg) {
    printf("[HAIKU_SERVER] Evento de teclado: código=%u, mods=%u, presionado=%s\n", 
           msg.data[0], msg.data[1], msg.data[2]);
    
    // Enviar evento a todas las ventanas
    for (auto& pair : windows) {
        if (pair.second->IsFocused()) {
            HaikuMessage key_msg(MessageType::KEY_PRESSED, 0, {
                pair.first, {msg.data[0], msg.data[1], msg.data[2], msg.data[3]}
            });
            
            SendSyscallToGuest(key_msg);
        }
    }
}

void HaikuServer::HandleFocusEvent(const HaikuMessage& msg) {
    uint32_t window_id = msg.data[0];
    
    printf("[HAIKU_SERVER] Evento de foco: ventana=%u, ganado=%s\n", 
           window_id, msg.data[1] ? "SÍ" : "NO");
    
    auto it = windows.find(window_id);
    if (it != windows.end()) {
        HaikuNativeWindow* window = it->second.get();
        
        // Actualizar estado de la ventana
        bool was_focused = window->IsFocused();
        if (was_focused != (msg.data[1] == 1)) {
            window->Focus();
        } else {
            window->Unfocus();
        }
        
        windows[window_id] = window;
        
        printf("[HAIKU_SERVER] Foco de ventana %u: %s\n", 
               window_id, window->IsFocused() ? "ganado" : "perdido");
    }
    }
}

void HaikuWindowServer::PrintServerInfo() const {
    printf("\n=== SERVIDOR HAIKU SERVER ===\n");
    printf("Estado: %s\n", running ? "EN EJECUCIÓN" : "DETENIDO");
    printf("Display: %ux%u\n", config.display_width, config.display_height);
    printf("Puerto: %u\n", config.port);
    printf("Clientes: %zu\n", clients.size());
    printf("Ventanas: %zu\n", windows.size());
    printf("Modo Debug: %s\n", config.debug_mode ? "ACTIVADO" : "INACTIVO");
    printf("===============================\n");
}

bool HaikuWindowServer::IsValidRequest(const std::string& request) {
    // Verificar que no tenga contenido malicioso
    if (request.empty() || request.length() > 4096) {
        return false;
    }
    
    // Verificar caracteres básicos
    for (char c : request) {
        if (c < 32 && !isalnum(c) && !isspace(c)) {
            return false;
        }
    }
    
    return true;
}

bool HaikuWindowServer::IsValidDesktopLinkCommand(const std::string& command) {
    return (command == "d" || command == "info" || 
            command == "quit" || 
            command.substr(0, 11) == "background" ||
            command.substr(0, 4) == "window" ||
            command.substr(0, 7) == "server");
}

bool HaikuWindowServer::IsValidServerProtocolCommand(const std::string& command) {
    return (command.find("WINDOW_") != std::string::npos ||
            command.find("DRAW_") != std::string::npos ||
            command.find("FLUSH_") != std::namespace::npos ||
            command.find("GET_") != std::string::npos ||
            command.find("POST_") != std::string::npos ||
            command.find("EVENT_") != std::string::npos);
}

// Instancia global
HaikuWindowServer* g_haiku_server = nullptr;

// Función para inicializar el servidor de Haiku
status_t InitializeHaikuWindowServer() {
    printf("[MAIN] Inicializando sistema de ventanas Haiku...\n");
    
    ServerConfig config;
    config.display_width = 1024;
    config.display_height = 768;
    config.debug_mode = false;
    
    // Crear servidor
    g_haiku_server = new HaikuWindowServer(config);
    
    if (!g_haiku_server || g_haiku_server->Initialize() != B_OK) {
        printf("[MAIN] ERROR: No se pudo inicializar el servidor de ventanas\n");
        return B_ERROR;
    }
    
    printf("[MAIN] ✅ Servidor de ventanas inicializado\n");
    printf("[MAIN] Las aplicaciones invitadas verán interfaces nativas de Haiku\n");
    
    return B_OK;
}

// Función para obtener la instancia del servidor
HaikuWindowServer* GetHaikuWindowServer() {
    return g_haiku_server;
}

// Función para actualizar el estado del servidor
void UpdateHaikuWindowServer() {
    if (g_haiku_server && g_haiku_server->IsRunning()) {
        g_haiku_server->PrintServerInfo();
    }
}