// UserlandVM Haiku - REAL WINDOWS VERSION
// Este programa crea VENTANAS REALES usando APIs nativas de HaikuOS
// SIN simulación - ventanas visibles e interactivas

#include <iostream>
#include <unistd.h>

// Detectar si estamos en HaikuOS
#ifdef __HAIKU__
    // Estamos corriendo en HaikuOS real - usar APIs nativas
    #include <os/app/Application.h>
    #include <os/interface/Window.h>
    #include <os/interface/View.h>
    #include <os/interface/Rect.h>
    #include <os/interface/Point.h>
    #include <os/interface/Screen.h>
    
    using namespace BPrivate;
    
    static BApplication* app = nullptr;
    static BWindow* window = nullptr;
    static BView* view = nullptr;
    
    class HaikuWindow : public BWindow {
    public:
        HaikuWindow(BRect frame, const char* title) 
            : BWindow(frame, title, B_TITLED_WINDOW, 0) {}
        
        virtual bool QuitRequested() {
            printf("[HaikuWindow] ❌ Ventana cerrada por usuario\n");
            be_app_messenger.SendMessage(B_QUIT_REQUESTED);
            return true;
        }
        
        virtual void MessageReceived(BMessage* message) {
            printf("[HaikuWindow] 📨 Mensaje recibido: 0x%08x\n", message->what);
            BWindow::MessageReceived(message);
        }
    };
    
    class HaikuView : public BView {
    public:
        HaikuView(BRect frame) : BView(frame, "main_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW) {}
        
        virtual void Draw(BRect updateRect) {
            printf("[HaikuView] 🎨 Dibujando ventana en: %.0f,%.0f - %.0f,%.0f\n", 
                   updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);
            
            // Fondo azul Haiku
            SetHighColor(0, 150, 255, 255);
            FillRect(updateRect);
            
            // Texto
            SetHighColor(255, 255, 255, 255);
            MoveTo(20, 30);
            DrawString("¡UserlandVM Haiku - VENTANA REAL!");
            MoveTo(20, 60);
            DrawString("Esta es una ventana REAL de HaikuOS");
            MoveTo(20, 90);
            DrawString("Interactúa con ella - es 100% real");
        }
        
        virtual void MouseDown(BPoint point) {
            printf("[HaikuView] 🖱️ Click en: %.0f,%.0f\n", point.x, point.y);
            BView::MouseDown(point);
        }
    };
#endif

int main(int argc, char* argv[]) {
    printf("🚀 UserlandVM Haiku - REAL WINDOW SYSTEM\n");
    
#ifdef __HAIKU__
    printf("✅ Detectado HaikuOS nativo - creando ventanas REALES\n");
    
    // Crear aplicación Haiku
    app = new BApplication("application/x-vnd.userlandvm.real");
    
    // Obtener tamaño de pantalla
    BScreen screen(B_MAIN_SCREEN_ID);
    BRect screen_frame = screen.Frame();
    
    // Crear ventana de 800x600 centrada
    BRect window_frame(
        (screen_frame.Width() - 800) / 2,
        (screen_frame.Height() - 600) / 2,
        (screen_frame.Width() + 800) / 2,
        (screen_frame.Height() + 600) / 2
    );
    
    window = new HaikuWindow(window_frame, "UserlandVM - Ventana REAL");
    view = new HaikuView(window_frame);
    
    // Agregar la vista a la ventana
    window->AddChild(view);
    
    // Mostrar la ventana - ESTO ES LO QUE LA HACE REAL
    window->Show();
    
    printf("✅ Ventana REAL creada y mostrada\n");
    printf("✅ Puedes interactuar con la ventana - es 100% real\n");
    printf("✅ La ventana aparece en tu escritorio HaikuOS\n");
    
    // Ejecutar la aplicación - esto inicia el loop de eventos REAL
    app->Run();
    
    // Cleanup (nunca se llega aquí con Run())
    delete view;
    delete window;
    delete app;
    
    printf("✅ Aplicación Haiku terminada\n");
#else
    printf("❌ NO ESTÁS EN HAIKU OS\n");
    printf("Este programa solo funciona en HaikuOS nativo\n");
#endif
    
    return 0;
}