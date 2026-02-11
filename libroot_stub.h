/*
 * libroot.so - Haiku API Stub Library
 * 
 * Implementación stub de las clases básicas de Haiku (BWindow, BApplication, BMessage)
 * que emiten INT 0x63 para ser interceptadas por el UserlandVM
 */

#include <stdint.h>
#include <stddef.h>

// Punto de entrada principal para INT 0x63
extern "C" {
    typedef uint32_t (*haiku_syscall_handler_t)(uint32_t syscall_num, uint32_t* args, uint32_t arg_count);
    
    // Función para registrar el manejador de syscalls
    void register_haiku_syscall_handler(haiku_syscall_handler_t handler);
}

namespace B {

///////////////////////////////////////////////////////////////////////////
// Tipos básicos de Haiku
///////////////////////////////////////////////////////////////////////////

typedef uint32_t status_t;
typedef uint32_t bigtime_t;
typedef uint32_t color_space;

#define B_OK                    0
#define B_ERROR                 ((status_t)-1)
#define B_BAD_VALUE             ((status_t)-2147483647)
#define B_NO_MEMORY             ((status_t)-2147483646)
#define B_NO_INIT               ((status_t)-2147483645)

#define B_RGB32                0
#define B_RGBA32               1

// Mensajes de sistema
#define B_QUIT_REQUESTED        'quit'
#define B_WINDOW_ACTIVATED       'wact'
#define B_WINDOW_DEACTIVATED     'wdea'

///////////////////////////////////////////////////////////////////////////
// BRect - Rectángulo básico
///////////////////////////////////////////////////////////////////////////

class BRect {
public:
    float left, top, right, bottom;
    
    BRect() : left(0), top(0), right(0), bottom(0) {}
    BRect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
    
    float Width() const { return right - left; }
    float Height() const { return bottom - top; }
    
    bool IsValid() const { return right >= left && bottom >= top; }
};

///////////////////////////////////////////////////////////////////////////
// BPoint - Punto 2D básico
///////////////////////////////////////////////////////////////////////////

class BPoint {
public:
    float x, y;
    
    BPoint() : x(0), y(0) {}
    BPoint(float x_, float y_) : x(x_), y(y_) {}
};

///////////////////////////////////////////////////////////////////////////
// BMessage - Mensaje para comunicación entre componentes
///////////////////////////////////////////////////////////////////////////

class BMessage {
private:
    uint32_t what;
    
public:
    BMessage() : what(0) {}
    BMessage(uint32_t what) : what(what) {}
    virtual ~BMessage() {}
    
    uint32_t What() const { return what; }
    
    status_t AddInt32(const char* name, int32_t value) {
        // SYSINT 0x6301: BMessage::AddInt32
        uint32_t args[2];
        args[0] = (uint32_t)name;
        args[1] = (uint32_t)value;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x6301, args, 2);
        }
        return B_ERROR;
    }
    
    status_t AddString(const char* name, const char* string) {
        // SYSINT 0x6302: BMessage::AddString
        uint32_t args[2];
        args[0] = (uint32_t)name;
        args[1] = (uint32_t)string;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x6302, args, 2);
        }
        return B_ERROR;
    }
    
    status_t AddPointer(const char* name, void* pointer) {
        // SYSINT 0x6303: BMessage::AddPointer
        uint32_t args[2];
        args[0] = (uint32_t)name;
        args[1] = (uint32_t)pointer;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x6303, args, 2);
        }
        return B_ERROR;
    }
    
    status_t FindInt32(const char* name, int32_t* value) const {
        // SYSINT 0x6304: BMessage::FindInt32
        uint32_t args[2];
        args[0] = (uint32_t)name;
        args[1] = (uint32_t)value;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x6304, args, 2);
        }
        return B_ERROR;
    }
    
    status_t FindString(const char* name, const char** string) const {
        // SYSINT 0x6305: BMessage::FindString
        uint32_t args[2];
        args[0] = (uint32_t)name;
        args[1] = (uint32_t)string;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x6305, args, 2);
        }
        return B_ERROR;
    }
};

///////////////////////////////////////////////////////////////////////////
// BView - Vista básica
///////////////////////////////////////////////////////////////////////////

class BView {
private:
    BRect frame;
    BView* parent;
    
public:
    BView() : parent(nullptr) {}
    BView(BRect frame) : frame(frame), parent(nullptr) {}
    virtual ~BView() {}
    
    virtual void Draw(BRect updateRect) {
        // SYSINT 0x6306: BView::Draw
        uint32_t args[5];
        args[0] = (uint32_t)this;
        args[1] = (uint32_t)updateRect.left;
        args[2] = (uint32_t)updateRect.top;
        args[3] = (uint32_t)updateRect.right;
        args[4] = (uint32_t)updateRect.bottom;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            g_haiku_handler(0x6306, args, 5);
        }
    }

    virtual void MoveTo(float x, float y) {
        // SYSINT 0x6307: BView::MoveTo
        uint32_t args[3];
        args[0] = (uint32_t)this;
        args[1] = (uint32_t)*(uint32_t*)&x;
        args[2] = (uint32_t)*(uint32_t*)&y;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            g_haiku_handler(0x6307, args, 3);
        }
    }

    virtual void ResizeTo(float width, float height) {
        // SYSINT 0x6308: BView::ResizeTo
        uint32_t args[3];
        args[0] = (uint32_t)this;
        args[1] = (uint32_t)*(uint32_t*)&width;
        args[2] = (uint32_t)*(uint32_t*)&height;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            g_haiku_handler(0x6308, args, 3);
        }
    }
};

///////////////////////////////////////////////////////////////////////////
// BWindow - Ventana básica
///////////////////////////////////////////////////////////////////////////

class BWindow {
private:
    BRect frame;
    const char* title;
    BView* child;
    bool visible;
    bool focused;
    
public:
    BWindow() : child(nullptr), visible(false), focused(false) {}
    BWindow(BRect frame, const char* title) : frame(frame), title(title), child(nullptr), visible(false), focused(false) {}
    virtual ~BWindow() {}
    
    status_t Show() {
        visible = true;
        
        // SYSINT 0x6309: BWindow::Show
        uint32_t args[1];
        args[0] = (uint32_t)this;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x6309, args, 1);
        }
        return B_OK;
    }
    
    status_t Hide() {
        visible = false;
        
        // SYSINT 0x630A: BWindow::Hide
        uint32_t args[1];
        args[0] = (uint32_t)this;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x630A, args, 1);
        }
        return B_OK;
    }
    
    status_t MoveTo(float x, float y) {
        frame.left = x;
        frame.top = y;
        frame.right = x + frame.Width();
        frame.bottom = y + frame.Height();
        
        // SYSINT 0x630B: BWindow::MoveTo
        uint32_t args[3];
        args[0] = (uint32_t)this;
        args[1] = (uint32_t)*(uint32_t*)&x;
        args[2] = (uint32_t)*(uint32_t*)&y;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x630B, args, 3);
        }
        return B_OK;
    }
    
    status_t ResizeTo(float width, float height) {
        frame.right = frame.left + width;
        frame.bottom = frame.top + height;
        
        // SYSINT 0x630C: BWindow::ResizeTo
        uint32_t args[3];
        args[0] = (uint32_t)this;
        args[1] = (uint32_t)*(uint32_t*)&width;
        args[2] = (uint32_t)*(uint32_t*)&height;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x630C, args, 3);
        }
        return B_OK;
    }
    
    status_t AddChild(BView* child) {
        this->child = child;
        
        // SYSINT 0x630D: BWindow::AddChild
        uint32_t args[2];
        args[0] = (uint32_t)this;
        args[1] = (uint32_t)child;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x630D, args, 2);
        }
        return B_OK;
    }
    
    void Invalidate() {
        // SYSINT 0x630E: BWindow::Invalidate
        uint32_t args[1];
        args[0] = (uint32_t)this;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            g_haiku_handler(0x630E, args, 1);
        }
    }
    
    status_t SetTitle(const char* title) {
        this->title = title;
        
        // SYSINT 0x630F: BWindow::SetTitle
        uint32_t args[2];
        args[0] = (uint32_t)this;
        args[1] = (uint32_t)title;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x630F, args, 2);
        }
        return B_OK;
    }
    
    BRect Frame() const { return frame; }
    const char* Title() const { return title; }
    bool IsVisible() const { return visible; }
    bool IsFocused() const { return focused; }
};

///////////////////////////////////////////////////////////////////////////
// BApplication - Aplicación básica
///////////////////////////////////////////////////////////////////////////

class BApplication {
private:
    const char* signature;
    bool running;
    
public:
    BApplication() : signature(nullptr), running(false) {}
    BApplication(const char* signature) : signature(signature), running(false) {}
    virtual ~BApplication() {}
    
    status_t Run() {
        running = true;
        
        // SYSINT 0x6310: BApplication::Run
        uint32_t args[1];
        args[0] = (uint32_t)this;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x6310, args, 1);
        }
        return B_OK;
    }
    
    status_t Quit() {
        running = false;
        
        // SYSINT 0x6311: BApplication::Quit
        uint32_t args[1];
        args[0] = (uint32_t)this;
        
        extern haiku_syscall_handler_t g_haiku_handler;
        if (g_haiku_handler) {
            return g_haiku_handler(0x6311, args, 1);
        }
        return B_OK;
    }
    
    bool IsRunning() const { return running; }
    const char* Signature() const { return signature; }
};

///////////////////////////////////////////////////////////////////////////
// Clases auxiliares de Haiku
///////////////////////////////////////////////////////////////////////////

class BColor {
private:
    uint8_t red, green, blue, alpha;
    
public:
    BColor() : red(0), green(0), blue(0), alpha(255) {}
    BColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) 
        : red(r), green(g), blue(b), alpha(a) {}
    
    uint8_t Red() const { return red; }
    uint8_t Green() const { return green; }
    uint8_t Blue() const { return blue; }
    uint8_t Alpha() const { return alpha; }
    
    uint32_t ToRGB32() const {
        return (alpha << 24) | (red << 16) | (green << 8) | blue;
    }
};

///////////////////////////////////////////////////////////////////////////
// Funciones globales auxiliares
///////////////////////////////////////////////////////////////////////////

namespace BScreen {
    inline BColor DesktopColor() { return BColor(51, 102, 152); }
}

} // namespace B

///////////////////////////////////////////////////////////////////////////
// Exportación de símbolos para compatibilidad
///////////////////////////////////////////////////////////////////////////

extern "C" {
    // Declaración de variable global (definida en el archivo .cpp)
    extern haiku_syscall_handler_t g_haiku_handler;
    
    // Función de registro (implementada en el archivo .cpp)
    void register_haiku_syscall_handler(haiku_syscall_handler_t handler);
}