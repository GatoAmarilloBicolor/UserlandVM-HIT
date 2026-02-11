/*
 * HaikuWindowServer.h - Subsistema de Ventanas Haiku
 * 
 * Implementa el app_server de Haiku OS para visualización nativa
 * de aplicaciones Haiku invitadas en UserlandVM
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>

// Haiku status codes and basic types (only if not already defined)
#ifndef B_OK
#define B_OK                    0
#endif
#ifndef B_ERROR
#define B_ERROR                 (-1)
#endif
#ifndef B_BAD_VALUE
#define B_BAD_VALUE             (-2147483647)
#endif
#ifndef B_NO_MEMORY
#define B_NO_MEMORY             (-2147483646)
#endif
#ifndef B_NO_INIT
#define B_NO_INIT               (-2147483645)
#endif

typedef int32_t status_t;
typedef int32_t haiku_status_t;

// Forward declarations
namespace B {
    typedef int32_t BWindow;
    typedef int32_t BView;
    typedef int32_t BApplication;
    typedef int32_t BMessenger;
    typedef int32_t BMessage;
}

namespace HaikuWindowServer {

/**
 * Mensajes del app_server de Haiku (simplificados)
 */
enum class MessageType {
    WINDOW_CREATED = 100,
    WINDOW_DESTROYED,
    WINDOW_ACTIVATED,
    WINDOW_DEACTIVATED,
    WINDOW_MOVED,
    WINDOW_RESIZED,
    MOUSE_MOVED,
    MOUSE_CLICKED,
    KEY_PRESSED,
    KEY_RELEASED,
    FOCUS_LOST,
    FOCUS_GAINED,
    DRAW_RECT,
    DRAW_STRING,
    FLUSH_DISPLAY,
    GET_WINDOW_INFO
};

/**
 * Estructura de mensaje simplificado
 */
struct HaikuMessage {
    MessageType type;
    uint32_t target_id;  // window_id, 0 para global
    uint32_t data[4];    // parámetros específicos
    
    HaikuMessage(MessageType t = MessageType::WINDOW_CREATED, uint32_t id = 0) 
        : type(t), target_id(id) {
        data[0] = data[1] = data[2] = data[3] = 0;
    }
};

/**
 * Cliente del app_server de Haiku
 */
class HaikuAppClient {
public:
    HaikuAppClient(int32_t client_id, int socket_fd);
    ~HaikuAppClient();
    
    status_t Connect();
    void Disconnect();
    
    bool IsConnected() const { return connected; }
    uint32_t GetClientID() const { return client_id; }
    
    // Manejo de mensajes
    haiku_status_t SendMessage(const HaikuMessage& msg);
    haiku_status_t ReceiveMessage(HaikuMessage& msg, int timeout_ms = 1000);
    
private:
    int32_t client_id;
    int socket_fd;
    bool connected;
    std::vector<HaikuMessage> message_queue;
    mutable std::mutex client_mutex;
};

/**
 * Ventana nativa de Haiku
 */
class HaikuNativeWindow {
public:
    HaikuNativeWindow(uint32_t window_id, const std::string& title);
    ~HaikuNativeWindow();
    
    // Operaciones de ventana
    status_t Show();
    status_t Hide();
    status_t Move(int32_t x, int32_t y);
    status_t Resize(uint32_t width, uint32_t height);
    status_t Focus();
    status_t Unfocus();
    
    // Operaciones de dibujo
    haiku_status_t DrawRect(int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t color);
    haiku_status_t DrawString(int32_t x, int32_t y, const std::string& text, uint32_t color);
    haiku_status_t Clear(uint32_t color);
    haiku_status_t Flush();
    
    // Estado
    bool IsVisible() const { return visible; }
    bool IsFocused() const { return focused; }
    uint32_t GetID() const { return window_id; }
    const std::string& GetTitle() const { return title; }
    
    // Información
    struct WindowInfo {
        int32_t x, y;
        uint32_t width, height;
        bool visible, focused;
        std::string title;
    } info;
    
    WindowInfo GetInfo() const;
    
private:
    uint32_t window_id;
    std::string title;
    int32_t pos_x, pos_y;
    uint32_t width, height;
    bool visible;
    bool focused;
    uint32_t bg_color, fg_color;
    std::vector<uint32_t> pixel_buffer;  // framebuffer software
    
    void Invalidate();
    void Redraw();
};

/**
 * Servidor de ventanas principal
 */
class HaikuWindowServer {
public:
    HaikuWindowServer();
    ~HaikuWindowServer();
    
    status_t Initialize();
    void Shutdown();
    
    // Gestión de clientes
    status_t RegisterClient(const std::string& app_name, HaikuAppClient*& client);
    void UnregisterClient(int32_t client_id);
    
    // Gestión de ventanas
    uint32_t CreateWindow(const std::string& title, uint32_t width, uint32_t height);
    status_t DestroyWindow(uint32_t window_id);
    HaikuNativeWindow* GetWindow(uint32_t window_id);
    
    // Eventos de entrada
    void HandleMouseEvent(int32_t x, int32_t y, int32_t button, bool pressed);
    void HandleKeyboardEvent(int32_t key_code, uint32_t modifiers, bool pressed);
    
    // Bucle principal
    void Run();
    void Stop();
    
    // Estado del servidor
    bool IsRunning() const { return running; }
    size_t GetWindowCount() const { return windows.size(); }
    size_t GetClientCount() const { return clients.size(); }
    
private:
    // Comunicación con el VM
    status_t SendToGuest(const HaikuMessage& msg);
    void BroadcastToClients(const HaikuMessage& msg);
    
    // Gestión interna
    void ProcessClientMessage(HaikuAppClient* client, const HaikuMessage& msg);
    void HandleWindowRequest(HaikuAppClient* client, const HaikuMessage& msg);
    void HandleInputEvent(const HaikuMessage& msg);
    void UpdateZOrder();
    
    std::map<int32_t, std::unique_ptr<HaikuAppClient>> clients;
    std::map<uint32_t, std::unique_ptr<HaikuNativeWindow>> windows;
    std::map<uint32_t, int32_t> window_focus_stack;  // Z-order
    uint32_t next_window_id;
    uint32_t next_client_id;
    uint32_t focused_window_id;
    
    bool running;
    int server_socket;
    std::thread server_thread;
    mutable std::mutex server_mutex;
    
    // Configuración
    uint32_t display_width = 1024;
    uint32_t display_height = 768;
    bool debug_mode = false;
};

// Instancia global del servidor
extern HaikuWindowServer* g_haiku_server;

} // namespace HaikuWindowServer