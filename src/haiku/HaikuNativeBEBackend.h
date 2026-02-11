/*
 * HaikuNativeBEBackend.h - Backend Nativo de Haiku usando Haiku API BE
 * 
 * Este backend usa las APIs nativas de Haiku (BApplication, BWindow, BView, etc.)
 * en lugar de SDL2/X11, para máxima compatibilidad y rendimiento
 */

#pragma once

// Backend nativo de Haiku usando APIs reales de Haiku
#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>
#include <thread>
#include <stdexcept>

// Constantes de Haiku
#define B_OK                    0
#define B_ERROR                 (-1)
#define B_BAD_VALUE             (-2147483647)
#define B_NO_MEMORY             (-2147483646)
#define B_NO_INIT               (-2147483645)

// Definiciones condicionales para compilación
#ifdef __HAIKU__
#include <AppKit.h>
#include <InterfaceKit.h>
#include <View.h>
#include <Window.h>
#include <Bitmap.h>
#include <GraphicsDefs.h>
#include <Screen.h>
#include <Message.h>
#include <String.h>
#include <Region.h>
#include <Looper.h>
#include <Handler.h>
#include <Messenger.h>
#else
// Stubs para compilación no-Haiku
#include <stdint.h>

// Tipos básicos de Haiku (stubs)
typedef struct rgb_color {
    uint8_t red, green, blue, alpha;
} rgb_color;

typedef struct BRect {
    float left, top, right, bottom;
    BRect() : left(0), top(0), right(0), bottom(0) {}
    BRect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
    float Width() const { return right - left; }
    float Height() const { return bottom - top; }
} BRect;

typedef struct BPoint {
    float x, y;
    BPoint() : x(0), y(0) {}
    BPoint(float x_, float y_) : x(x_), y(y_) {}
} BPoint;

// Status type definition (using int32_t instead of enum to avoid conflicts)
typedef int32_t status_t;

// Clases base de Haiku (stubs)
class BLooper {
public:
    BLooper() {}
    virtual ~BLooper() {}
};

class BHandler {
public:
    BHandler() {}
    virtual ~BHandler() {}
};

class BMessage {
public:
    BMessage() {}
    virtual ~BMessage() {}
};

class BMessenger {
public:
    BMessenger() {}
    BMessenger(BLooper* looper) {}
};

class BString {
public:
    BString() {}
    BString(const char* str) {}
};

// Clases de la interfaz (stubs)
class BView : public BHandler {
public:
    BView(BRect frame) {}
    virtual ~BView() {}
    virtual void Draw(BRect updateRect) {}
    virtual void SetHighColor(rgb_color color) {}
    virtual void SetLowColor(rgb_color color) {}
    virtual void SetViewColor(rgb_color color) {}
    virtual void FillRect(BRect rect, rgb_color color) {}
    virtual void StrokeRect(BRect rect, rgb_color color) {}
    virtual void DrawString(const char* string, BPoint point) {}
    virtual void MoveTo(BPoint point) {}
    virtual void ResizeTo(float width, float height) {}
    virtual void GetPreferredSize(float* width, float* height) {}
    virtual void FrameMoved(BPoint newLocation) {}
    virtual void DrawAfterChildren(BRect updateRect) {}
    virtual void AllAttached() {}
    virtual void AllDetached() {}
};

class BWindow : public BLooper {
public:
    BWindow(BRect frame, const char* title, uint32_t type = 0, uint32_t flags = 0) {}
    virtual ~BWindow() {}
    virtual bool Show() { return true; }
    virtual void Hide() {}
    virtual void Minimize(bool minimize) {}
    virtual bool IsMinimized() const { return false; }
    virtual void Activate(bool active = true) {}
    virtual bool IsActive() const { return false; }
    virtual void SetFlags(uint32_t flags) {}
    virtual uint32_t Flags() const { return 0; }
    virtual void MessageReceived(BMessage* message) {}
    virtual void DispatchMessage(BMessage* message) {}
};

class BApplication : public BLooper {
public:
    BApplication(const char* signature) {}
    virtual ~BApplication() {}
    virtual void ReadyToRun() {}
    virtual void Pulse() {}
    virtual void Quit() {}
    virtual bool IsRunning() const { return true; }
};

// Tipos adicionales
typedef uint32_t escapement_escape;
typedef struct BFont BFont;

// Colores de Haiku
typedef struct BColor {
    uint8_t red, green, blue, alpha;
    BColor() : red(0), green(0), blue(0), alpha(255) {}
    BColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : red(r), green(g), blue(b), alpha(a) {}
} BColor;

// Mensajes de Haiku
struct HaikuMessage {
    uint32_t what;
    std::string data;
    HaikuMessage() : what(0) {}
};

#endif

// Componentes del backend
class HaikuNativeView;
class HaikuNativeWindow;
class HaikuNativeApplication;

// Variables globales del backend
static HaikuNativeView* g_root_view = nullptr;
static HaikuNativeApplication* g_be_application = nullptr;
static BWindow* g_root_window = nullptr;

// Configuración del backend
struct HaikuNativeBEConfig {
    uint32_t width = 1024;
    uint32_t height = 768;
    uint32_t bpp = 32;
    bool fullscreen = false;
    bool hardware_accel = true;
    bool vsync = true;
    bool debug_mode = false;
    std::string window_title = "UserlandVM-Haiku";
};

// Forward declarations
class HaikuNativeBEBackend;

// Variables globales del backend
static HaikuNativeBEBackend* g_haiku_native_backend = nullptr;

class HaikuNativeView : public BView {
public:
    HaikuNativeView(BRect frame);
    virtual ~HaikuNativeView();
    
    // Métodos de BView
    virtual void Draw(BRect updateRect);
    virtual void SetHighColor(rgb_color color);
    virtual void SetLowColor(rgb_color color);
    virtual void SetViewColor(rgb_color color);
    virtual void FillRect(BRect rect, rgb_color color);
    virtual void StrokeRect(BRect rect, rgb_color color);
    virtual void DrawString(const char* string, BPoint point, escapement_escape escape);
    virtual void DrawString(const char* string, BPoint point, const BFont* font, rgb_color color);
    virtual void MoveTo(BPoint point);
    virtual void ResizeTo(float width, float height);
    virtual void GetPreferredSize(float* width, float* height);
    virtual void FrameMoved(BPoint newLocation);
    virtual void DrawAfterChildren(BRect updateRect);
    virtual void AllAttached();
    virtual void AllDetached();
    
private:
    // Framebuffer para el contenido de la vista
    uint8_t* framebuffer;
    uint32_t* pixels;
    size_t framebuffer_size;
    BRect frame_rect;
    BColor current_fg_color;
    BColor current_bg_color;
    
    // Configuración global (referencia estática)
    static HaikuNativeBEConfig* config;
    
    // Sincronización
    BLooper* looper;
    BMessenger messenger;
    
    // Métodos privados
    void UpdateFramebuffer(BRect update_rect);
    void UpdateFrame(BRect new_frame);
    void ClearFramebuffer();
    void DrawPixel(int32_t x, int32_t y, uint32_t color);
    void DrawHLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void DrawVLine(int32_t x, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void DrawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    void DrawFilledRect(BRect rect, uint32_t color);
    void UpdateFramebuffer(const void* data, size_t size, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void InvalidateRect(BRect rect);
    void InvalidateRange(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
    uint32_t ColorToRGB(const BColor& color);
    uint32_t ColorToRGB(uint32_t rgba);
    void CopyFramebufferTo(void* destination, size_t size);
    BRect FrameRect() const;
    bool Intersects(const BRect& rect) const;
    bool Contains(const BPoint& point) const;
    void PrintFramebufferInfo() const;
};

// Componente de ventana Haiku nativa
class HaikuNativeWindow : public BWindow {
public:
    HaikuNativeWindow(BRect frame, const char* title, uint32_t type = 0, uint32_t flags = 0);
    virtual ~HaikuNativeWindow();
    
    // Métodos de BWindow
    virtual bool Show();
    virtual void Hide();
    virtual void Minimize(bool minimize);
    virtual bool IsMinimized() const;
    virtual void Activate(bool active = true);
    virtual bool IsActive() const;
    virtual void SetFlags(uint32_t flags);
    virtual uint32_t Flags() const;
    
    // Métodos de BHandler
    virtual void MessageReceived(BMessage* message);
    virtual void DispatchMessage(BMessage* message);
    
    // Framebuffer y contenido
    void* GetFramebuffer() const { return pixels; }
    uint32_t GetFramebufferWidth() const { return frame_rect.Width(); }
    uint32_t GetFramebufferHeight() const { return frame_rect.Height(); }
    void UpdateFramebufferContent(const void* data, size_t size);
    
private:
    // Estado
    bool is_visible;
    bool is_minimized;
    bool is_active;
    bool focused;
    uint32_t window_flags;
    
    // Framebuffer
    uint32_t* pixels;
    size_t framebuffer_size;
    BRect frame_rect;
    
    // Vista hija
    HaikuNativeView* main_view;
    std::vector<HaikuNativeView*> child_views;
    
    // Propiedades de la ventana
    char title[256];
    uint32_t look;
    uint32_t feel;
    uint32_t type;
    
    // Integración con el servidor Haiku
    uint32_t server_window_id;
    
    // Métodos privados
    void CreateChildViews();
    void UpdateWindowInfo();
    void HandleAppServerMessage(const HaikuMessage& msg);
    void SendWindowStateToServer();
    
public:
    // Métodos de acceso para el backend
    uint32_t GetWindowWidth() const { return frame_rect.Width(); }
    uint32_t GetWindowHeight() const { return frame_rect.Height(); }
    HaikuNativeView* GetMainView() const { return main_view; }
    
    // Método para obtener el window_id del servidor
    uint32_t GetServerWindowID() const { return server_window_id; }
};

// Aplicación Haiku nativa
class HaikuNativeApplication : public BApplication {
public:
    HaikuNativeApplication(const char* signature);
    virtual ~HaikuNativeApplication();
    
    // Métodos de BApplication
    virtual void ReadyToRun();
    virtual void Pulse();
    virtual void Quit();
    virtual bool IsRunning() const;
    
    // Gestión de ventanas
    HaikuNativeWindow* CreateWindow(const char* title, BRect frame, uint32_t type = 0, uint32_t flags = 0);
    void DestroyWindow(uint32_t window_id);
    HaikuNativeWindow* GetWindow(uint32_t window_id);
    
    // Integración con el servidor
    void RegisterWithServer(uint32_t window_id);
    void UnregisterFromServer(uint32_t window_id);
    
private:
    bool is_running;
    char app_signature[256];
    
    std::map<uint32_t, std::unique_ptr<HaikuNativeWindow>> windows;
    BApplication* be_application;
    
    // Integración con el servidor Haiku
    void HandleServerMessage(const HaikuMessage& msg);
    void HandleWindowServerRequest(const HaikuMessage& msg);
};

// Backend principal que integra todo el sistema
class HaikuNativeBEBackend {
public:
    HaikuNativeBEBackend();
    ~HaikuNativeBEBackend();
    
    // Inicialización
    bool Initialize();
    void Shutdown();
    
    // Gestión de la aplicación Haiku
    status_t CreateApplication(const char* signature);
    void QuitApplication();
    
    // Gestión de ventanas
    uint32_t CreateWindow(const char* title, uint32_t width, uint32_t height, uint32_t x = 0, uint32_t y = 0, uint32_t type = 0, uint32_t flags = 0);
    void DestroyWindow(uint32_t window_id);
    status_t ShowWindow(uint32_t window_id);
    status_t HideWindow(uint32_t window_id);
    
    // Conexión con el servidor emulado
    status_t ConnectToHaikuServer(const std::string& server_host, uint32_t port);
    void DisconnectFromHaikuServer();
    
    // Framebuffer
    status_t GetWindowFramebuffer(uint32_t window_id, void** framebuffer, uint32_t* width, uint32_t* height);
    
    // Estado
    bool IsInitialized() const { return is_initialized; }
    bool IsConnected() const { return server_connected; }
    
private:
    bool is_initialized;
    bool server_connected;
    
    // Aplicación Haiku nativa
    std::unique_ptr<HaikuNativeApplication> be_application;
    
    // Gestión de ventanas
    std::map<uint32_t, std::unique_ptr<HaikuNativeWindow>> windows;
    uint32_t next_window_id;
    
    // Conexión con el servidor emulado
    int server_socket;
    std::string server_host;
    uint32_t server_port;
    std::thread server_thread;
    
    // Componentes de Haiku
    std::unique_ptr<BLooper> app_looper;
    std::unique_ptr<BMessenger> app_messenger;
    
    // Métodos privados
    void InitializeBEComponents();
    void RunBEApplication();
    void HandleBEMessage(BMessage* message);
    void ProcessWindowEvents();
    
    // Comunicación con servidor emulado
    void SendToServer(const HaikuMessage& msg);
    void HandleServerResponse(const std::string& response);
    void ProcessServerResponses();
    
    status_t CreateHaikuNativeWindow(const char* title, uint32_t width, uint32_t height, uint32_t type = 0, uint32_t flags = 0);
    void RegisterWindowWithServer(HaikuNativeWindow* window);
};

// Variable global del backend
extern HaikuNativeBEBackend* g_haiku_native_backend;