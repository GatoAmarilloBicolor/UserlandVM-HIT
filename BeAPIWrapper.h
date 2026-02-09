#ifndef BE_API_WRAPPER_H
#define BE_API_WRAPPER_H

// Forward declarations para evitar conflictos de headers
class BApplication;
class BWindow;
class BView;

// Crear ventana Haiku sin incluir headers conflictivos
extern "C" {
    void CreateHaikuWindow(const char* title);
    void ShowHaikuWindow();
    void ProcessWindowEvents();
}

#endif
