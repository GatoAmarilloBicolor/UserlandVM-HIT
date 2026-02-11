/*
 * HaikuWindowInitializer.cpp - Inicializador del Backend Nativo de Haiku
 * 
 * Este archivo conecta nuestro HaikuNativeBEBackend con el sistema principal UserlandVM
 * sin interferir con las implementaciones existentes
 */

#include "HaikuNativeBEBackend.h"
#include <cstdio>
#include <iostream>

// Variables globales del backend (definida en HaikuNativeBEBackend.h)
// HaikuNativeBEBackend* g_haiku_native_backend = nullptr;

// Función para inicializar el backend nativo de Haiku
extern "C" status_t InitializeHaikuNativeBackend() {
    printf("[HaikuNative] Inicializando backend nativo de Haiku...\n");
    
    // Crear el backend
    g_haiku_native_backend = new HaikuNativeBEBackend();
    
    if (!g_haiku_native_backend) {
        printf("[HaikuNative] ERROR: No se pudo crear el backend\n");
        return B_NO_MEMORY;
    }
    
    // Inicializar el backend
    if (!g_haiku_native_backend->Initialize()) {
        printf("[HaikuNative] ERROR: No se pudo inicializar el backend\n");
        delete g_haiku_native_backend;
        g_haiku_native_backend = nullptr;
        return B_NO_INIT;
    }
    
    // Crear aplicación Haiku
    status_t app_result = g_haiku_native_backend->CreateApplication("application/x-vnd.UserlandVM-Haiku");
    if (app_result != B_OK) {
        printf("[HaikuNative] ERROR: No se pudo crear aplicación Haiku\n");
        g_haiku_native_backend->Shutdown();
        delete g_haiku_native_backend;
        g_haiku_native_backend = nullptr;
        return app_result;
    }
    
    // Conectar al servidor emulado de Haiku (opcional)
    status_t conn_result = g_haiku_native_backend->ConnectToHaikuServer("localhost", 12345);
    if (conn_result != B_OK) {
        printf("[HaikuNative] WARNING: No se pudo conectar al servidor Haiku (usando modo local)\n");
    }
    
    printf("[HaikuNative] ✅ Backend nativo inicializado correctamente\n");
    printf("[HaikuNative] 🎯 APIs nativas de Haiku disponibles\n");
    printf("[HaikuNative] 💾 Backend: HaikuNativeBEBackend v1.0\n");
    
    return B_OK;
}

// Función para obtener la instancia del backend
extern "C" HaikuNativeBEBackend* GetHaikuNativeBackend() {
    return g_haiku_native_backend;
}

// Función para verificar si el backend está inicializado
extern "C" bool IsHaikuNativeBackendInitialized() {
    return g_haiku_native_backend && g_haiku_native_backend->IsInitialized();
}

// Función para apagar el backend
extern "C" void ShutdownHaikuNativeBackend() {
    if (g_haiku_native_backend) {
        printf("[HaikuNative] Apagando backend nativo...\n");
        g_haiku_native_backend->Shutdown();
        delete g_haiku_native_backend;
        g_haiku_native_backend = nullptr;
        printf("[HaikuNative] ✅ Backend apagado\n");
    }
}

// Función para crear ventanas desde otras partes del sistema
extern "C" uint32_t CreateHaikuWindow(const char* title, uint32_t width, uint32_t height, uint32_t x, uint32_t y) {
    if (!IsHaikuNativeBackendInitialized()) {
        printf("[HaikuNative] ERROR: Backend no inicializado\n");
        return 0;
    }
    
    return g_haiku_native_backend->CreateWindow(title, width, height, x, y);
}

// Función para mostrar ventanas
extern "C" status_t ShowHaikuWindow(uint32_t window_id) {
    if (!IsHaikuNativeBackendInitialized()) {
        return B_NO_INIT;
    }
    
    return g_haiku_native_backend->ShowWindow(window_id);
}

// Función para ocultar ventanas
extern "C" status_t HideHaikuWindow(uint32_t window_id) {
    if (!IsHaikuNativeBackendInitialized()) {
        return B_NO_INIT;
    }
    
    return g_haiku_native_backend->HideWindow(window_id);
}

// Función para obtener framebuffer de ventana
extern "C" status_t GetHaikuWindowFramebuffer(uint32_t window_id, void** framebuffer, uint32_t* width, uint32_t* height) {
    if (!IsHaikuNativeBackendInitialized()) {
        return B_NO_INIT;
    }
    
    return g_haiku_native_backend->GetWindowFramebuffer(window_id, framebuffer, width, height);
}

// Función para destruir ventanas
extern "C" void DestroyHaikuWindow(uint32_t window_id) {
    if (IsHaikuNativeBackendInitialized()) {
        g_haiku_native_backend->DestroyWindow(window_id);
    }
}

// Función para prueba/demostración
extern "C" void DemoHaikuWindows() {
    if (!IsHaikuNativeBackendInitialized()) {
        printf("[HaikuNative] ERROR: Backend no inicializado para demo\n");
        return;
    }
    
    printf("\n[HaikuNative] === Demostración de Ventanas Haiku ===\n");
    
    // Crear algunas ventanas de demostración
    uint32_t window1 = CreateHaikuWindow("Demo Tracker", 800, 600, 100, 100);
    uint32_t window2 = CreateHaikuWindow("Demo Terminal", 640, 480, 220, 180);
    uint32_t window3 = CreateHaikuWindow("Demo WebPositive", 1024, 768, 350, 50);
    
    if (window1 && window2 && window3) {
        printf("[HaikuNative] ✅ Ventanas creadas: %u, %u, %u\n", window1, window2, window3);
        
        // Mostrar todas las ventanas
        ShowHaikuWindow(window1);
        ShowHaikuWindow(window2);
        ShowHaikuWindow(window3);
        
        printf("[HaikuNative] ✅ Todas las ventanas mostradas\n");
        printf("[HaikuNative] 💡 Esto simularía la apariencia de aplicaciones Haiku reales\n");
        
        // Esperar un momento para visualización
        printf("[HaikuNative] ⏱️  Presione Enter para continuar con la demostración...\n");
        getchar();
        
        // Ocultar y destruir
        HideHaikuWindow(window2);
        DestroyHaikuWindow(window1);
        DestroyHaikuWindow(window2);
        DestroyHaikuWindow(window3);
        
        printf("[HaikuNative] ✅ Demostración completada\n");
    } else {
        printf("[HaikuNative] ❌ Error al crear ventanas de demostración\n");
    }
}