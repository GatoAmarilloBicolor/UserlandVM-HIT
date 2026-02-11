/*
 * HaikuWindowInitializer.h - Declaraciones para inicialización del Backend Nativo Haiku
 * 
 * Proporciona la interfaz entre el sistema UserlandVM principal
 * y nuestro HaikuNativeBEBackend
 */

#pragma once

#include <cstdint>

// Haiku status codes (solo si no están definidos)
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

// Forward declaration
class HaikuNativeBEBackend;

// Variable global del backend (definida en HaikuNativeBEBackend.h)
// extern HaikuNativeBEBackend* g_haiku_native_backend;

// Funciones de inicialización y control del backend nativo Haiku
#ifdef __cplusplus
extern "C" {
#endif

status_t InitializeHaikuNativeBackend();
HaikuNativeBEBackend* GetHaikuNativeBackend();
bool IsHaikuNativeBackendInitialized();
void ShutdownHaikuNativeBackend();

// Funciones para creación y gestión de ventanas
uint32_t CreateHaikuWindow(const char* title, uint32_t width, uint32_t height, uint32_t x, uint32_t y);
status_t ShowHaikuWindow(uint32_t window_id);
status_t HideHaikuWindow(uint32_t window_id);
status_t GetHaikuWindowFramebuffer(uint32_t window_id, void** framebuffer, uint32_t* width, uint32_t* height);
void DestroyHaikuWindow(uint32_t window_id);

// Función de demostración
void DemoHaikuWindows();

#ifdef __cplusplus
}
#endif