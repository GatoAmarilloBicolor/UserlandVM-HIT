/*
 * libroot_stub.cpp - Implementación simplificada del stub de libroot
 * 
 * Este archivo implementa las clases básicas de Haiku que emiten INT 0x63
 * Versión simplificada sin C++ avanzado para evitar errores
 */

#include "libroot_stub.h"
#include <cstdio>

///////////////////////////////////////////////////////////////////////////
// Variable global del manejador de syscalls
///////////////////////////////////////////////////////////////////////////

haiku_syscall_handler_t g_haiku_handler = nullptr;

///////////////////////////////////////////////////////////////////////////
// Implementación de la función de registro
///////////////////////////////////////////////////////////////////////////

extern "C" void register_haiku_syscall_handler(haiku_syscall_handler_t handler)
{
    printf("[libroot_stub] Registrando manejador de syscalls Haiku en: %p\n", handler);
    g_haiku_handler = handler;
}

///////////////////////////////////////////////////////////////////////////
// Función helper para emitir INT 0x63
///////////////////////////////////////////////////////////////////////////

static inline status_t emit_haiku_syscall(uint32_t syscall_num, uint32_t* args, uint32_t arg_count)
{
    if (!g_haiku_handler) {
        printf("[libroot_stub] ERROR: No hay manejador de syscalls registrado\n");
        return B_ERROR;
    }
    
    printf("[libroot_stub] Emitiendo syscall Haiku 0x%04X con %u argumentos\n", syscall_num, arg_count);
    
    // Emitir INT 0x63 con el número de syscall y argumentos
    // Esto será interceptado por el UserlandVM
    uint32_t result = g_haiku_handler(syscall_num, args, arg_count);
    
    return (status_t)result;
}

///////////////////////////////////////////////////////////////////////////
// Implementaciones de las clases del stub (simplificadas)
///////////////////////////////////////////////////////////////////////////

// BMessage implementations
status_t BMessage_AddInt32(const char* name, int32_t value) {
    printf("[libroot_stub] BMessage::AddInt32('%s', %d)\n", name, value);
    
    uint32_t args[2];
    args[0] = (haiku_ptr_t)name;
    args[1] = (haiku_ptr_t)(uintptr_t)value;
    
    return emit_haiku_syscall(0x6301, args, 2);
}

status_t BMessage_AddString(const char* name, const char* string) {
    printf("[libroot_stub] BMessage::AddString('%s', '%s')\n", name, string);
    
    uint32_t args[2];
    args[0] = (haiku_ptr_t)name;
    args[1] = (haiku_ptr_t)string;
    
    return emit_haiku_syscall(0x6302, args, 2);
}

status_t BMessage_AddPointer(const char* name, void* pointer) {
    printf("[libroot_stub] BMessage::AddPointer('%s', %p)\n", name, pointer);
    
    uint32_t args[2];
    args[0] = (haiku_ptr_t)name;
    args[1] = (haiku_ptr_t)pointer;
    
    return emit_haiku_syscall(0x6303, args, 2);
}

status_t BMessage_FindInt32(const char* name, int32_t* value) {
    printf("[libroot_stub] BMessage::FindInt32('%s')\n", name);
    
    uint32_t args[2];
    args[0] = (haiku_ptr_t)name;
    args[1] = (haiku_ptr_t)(uintptr_t)value;
    
    return emit_haiku_syscall(0x6304, args, 2);
}

status_t BMessage_FindString(const char* name, const char** string) {
    printf("[libroot_stub] BMessage::FindString('%s')\n", name);
    
    uint32_t args[2];
    args[0] = (haiku_ptr_t)name;
    args[1] = (haiku_ptr_t)string;
    
    return emit_haiku_syscall(0x6305, args, 2);
}

// BView implementations
status_t BView_Draw(void* view, float left, float top, float right, float bottom) {
    printf("[libroot_stub] BView::Draw(rect: %.0f,%.0f,%.0f,%.0f)\n", left, top, right, bottom);
    
    uint32_t args[5];
    args[0] = (haiku_ptr_t)view;
    args[1] = (haiku_ptr_t)(uintptr_t)&left;
    args[2] = (haiku_ptr_t)(uintptr_t)&top;
    args[3] = (haiku_ptr_t)(uintptr_t)&right;
    args[4] = (haiku_ptr_t)(uintptr_t)&bottom;
    
    return emit_haiku_syscall(0x6306, args, 5);
}

status_t BView_MoveTo(void* view, float x, float y) {
    printf("[libroot_stub] BView::MoveTo(%.0f,%.0f)\n", x, y);
    
    uint32_t args[3];
    args[0] = (haiku_ptr_t)view;
    args[1] = (haiku_ptr_t)(uintptr_t)&x;
    args[2] = (haiku_ptr_t)(uintptr_t)&y;
    
    return emit_haiku_syscall(0x6307, args, 3);
}

status_t BView_ResizeTo(void* view, float width, float height) {
    printf("[libroot_stub] BView::ResizeTo(%.0f,%.0f)\n", width, height);
    
    uint32_t args[3];
    args[0] = (haiku_ptr_t)view;
    args[1] = (haiku_ptr_t)(uintptr_t)&width;
    args[2] = (haiku_ptr_t)(uintptr_t)&height;
    
    return emit_haiku_syscall(0x6308, args, 3);
}

// BWindow implementations
status_t BWindow_Show(void* window) {
    printf("[libroot_stub] BWindow::Show\n");
    
    uint32_t args[1];
    args[0] = (haiku_ptr_t)window;
    
    return emit_haiku_syscall(0x6309, args, 1);
}

status_t BWindow_Hide(void* window) {
    printf("[libroot_stub] BWindow::Hide\n");
    
    uint32_t args[1];
    args[0] = (haiku_ptr_t)window;
    
    return emit_haiku_syscall(0x630A, args, 1);
}

status_t BWindow_MoveTo(void* window, float x, float y) {
    printf("[libroot_stub] BWindow::MoveTo(%.0f,%.0f)\n", x, y);
    
    uint32_t args[3];
    args[0] = (haiku_ptr_t)window;
    args[1] = (haiku_ptr_t)(uintptr_t)&x;
    args[2] = (haiku_ptr_t)(uintptr_t)&y;
    
    return emit_haiku_syscall(0x630B, args, 3);
}

status_t BWindow_ResizeTo(void* window, float width, float height) {
    printf("[libroot_stub] BWindow::ResizeTo(%.0f,%.0f)\n", width, height);
    
    uint32_t args[3];
    args[0] = (haiku_ptr_t)window;
    args[1] = (haiku_ptr_t)(uintptr_t)&width;
    args[2] = (haiku_ptr_t)(uintptr_t)&height;
    
    return emit_haiku_syscall(0x630C, args, 3);
}

status_t BWindow_AddChild(void* window, void* child) {
    printf("[libroot_stub] BWindow::AddChild\n");
    
    uint32_t args[2];
    args[0] = (haiku_ptr_t)window;
    args[1] = (haiku_ptr_t)child;
    
    return emit_haiku_syscall(0x630D, args, 2);
}

void BWindow_Invalidate(void* window) {
    printf("[libroot_stub] BWindow::Invalidate\n");
    
    uint32_t args[1];
    args[0] = (haiku_ptr_t)window;
    
    emit_haiku_syscall(0x630E, args, 1);
}

status_t BWindow_SetTitle(void* window, const char* title) {
    printf("[libroot_stub] BWindow::SetTitle('%s')\n", title);
    
    uint32_t args[2];
    args[0] = (haiku_ptr_t)window;
    args[1] = (haiku_ptr_t)title;
    
    return emit_haiku_syscall(0x630F, args, 2);
}

// BApplication implementations
status_t BApplication_Run(void* app) {
    printf("[libroot_stub] BApplication::Run\n");
    
    uint32_t args[1];
    args[0] = (haiku_ptr_t)app;
    
    return emit_haiku_syscall(0x6310, args, 1);
}

status_t BApplication_Quit(void* app) {
    printf("[libroot_stub] BApplication::Quit\n");
    
    uint32_t args[1];
    args[0] = (haiku_ptr_t)app;
    
    return emit_haiku_syscall(0x6311, args, 1);
}

///////////////////////////////////////////////////////////////////////////
// Constructor del módulo (se llama cuando se carga la librería)
///////////////////////////////////////////////////////////////////////////

extern "C" __attribute__((constructor))
void libroot_init() {
    printf("[libroot_stub] libroot.so stub inicializado\n");
    printf("[libroot_stub] Listo para emitir syscalls Haiku via INT 0x63\n");
}

///////////////////////////////////////////////////////////////////////////
// Destructor del módulo (se llama cuando se descarga la librería)
///////////////////////////////////////////////////////////////////////////

extern "C" __attribute__((destructor))
void libroot_fini() {
    printf("[libroot_stub] libroot.so stub finalizado\n");
}