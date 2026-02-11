/*
 * libroot_stub.cpp - Implementación del stub de libroot.so
 * 
 * Este archivo implementa las funciones que emiten INT 0x63 para ser
 * interceptadas por el UserlandVM
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

static inline B::status_t emit_haiku_syscall(uint32_t syscall_num, uint32_t* args, uint32_t arg_count)
{
    if (!g_haiku_handler) {
        printf("[libroot_stub] ERROR: No hay manejador de syscalls registrado\n");
        return B_ERROR;
    }
    
    printf("[libroot_stub] Emitiendo syscall Haiku 0x%04X con %u argumentos\n", syscall_num, arg_count);
    
    // Emitir INT 0x63 con el número de syscall y argumentos
    // Esto será interceptado por el UserlandVM
    uint32_t result = g_haiku_handler(syscall_num, args, arg_count);
    
    return (B::status_t)result;
}

///////////////////////////////////////////////////////////////////////////
// Implementaciones adicionales para compatibilidad
///////////////////////////////////////////////////////////////////////////

namespace B {

// Implementaciones globales que podrían ser llamadas directamente
status_t GlobalShowWindow(BWindow* window) {
    return emit_haiku_syscall(0x6309, (uint32_t*)&window, 1);
}

status_t GlobalHideWindow(BWindow* window) {
    return emit_haiku_syscall(0x630A, (uint32_t*)&window, 1);
}

status_t GlobalRunApplication(BApplication* app) {
    return emit_haiku_syscall(0x6310, (uint32_t*)&app, 1);
}

status_t GlobalQuitApplication(BApplication* app) {
    return emit_haiku_syscall(0x6311, (uint32_t*)&app, 1);
}

// Additional Be API stubs that might be needed
status_t GlobalShowView(BView* view) {
    printf("[libroot_stub] BView::Show(%p)\n", view);
    return emit_haiku_syscall(0x6312, (uint32_t*)&view, 1);
}

status_t GlobalHideView(BView* view) {
    printf("[libroot_stub] BView::Hide(%p)\n", view);
    return emit_haiku_syscall(0x6313, (uint32_t*)&view, 1);
}

status_t GlobalDrawString(BView* view, const char* string, BPoint location) {
    printf("[libroot_stub] BView::DrawString(%p, '%s', %.0f,%.0f)\n", view, string, location.x, location.y);
    uint32_t args[4] = {(uint32_t)view, (uint32_t)string, *(uint32_t*)&location.x, *(uint32_t*)&location.y};
    return emit_haiku_syscall(0x6314, args, 4);
}

status_t GlobalInvalidateRect(BView* view, BRect rect) {
    printf("[libroot_stub] BView::InvalidateRect(%p, %.0f,%.0f,%.0f,%.0f)\n", 
           view, rect.left, rect.top, rect.right, rect.bottom);
    uint32_t args[5] = {(uint32_t)view, *(uint32_t*)&rect.left, *(uint32_t*)&rect.top, *(uint32_t*)&rect.right, *(uint32_t*)&rect.bottom};
    return emit_haiku_syscall(0x6315, args, 5);
}

// BLooper stubs (message handling)
status_t GlobalPostMessage(BMessage* message, BMessenger* target = nullptr) {
    printf("[libroot_stub] BLooper::PostMessage(%p, %p)\n", message, target);
    uint32_t args[2] = {(uint32_t)message, (uint32_t)target};
    return emit_haiku_syscall(0x6320, args, 2);
}

status_t GlobalRunLooper(BLooper* looper) {
    printf("[libroot_stub] BLooper::Run(%p)\n", looper);
    return emit_haiku_syscall(0x6321, (uint32_t*)&looper, 1);
}

// BBitmap stubs
status_t GlobalCreateBitmap(uint32_t width, uint32_t height, uint32_t color_space, uint32_t flags, void** bitmap) {
    printf("[libroot_stub] BBitmap::CreateBitmap(%ux%u, %u, %u, %u)\n", width, height, color_space, flags);
    uint32_t args[5] = {width, height, color_space, flags, (uint32_t)bitmap};
    return emit_haiku_syscall(0x6322, args, 5);
}

status_t GlobalGetBitmapBits(void* bitmap, void** bits) {
    printf("[libroot_stub] BBitmap::GetBits(%p)\n", bitmap);
    uint32_t args[2] = {(uint32_t)bitmap, (uint32_t)bits};
    return emit_haiku_syscall(0x6323, args, 2);
}

// BControl stubs
status_t GlobalCreateControl(BRect frame, const char* name, uint32_t resize_mask, uint32_t flags, void** control) {
    printf("[libroot_stub] BControl::CreateControl(%.0f,%.0f,%.0f,%.0f, '%s', %u, %u, %p)\n", 
           frame.left, frame.top, frame.right, frame.bottom, name, resize_mask, flags, control);
    uint32_t args[6] = {*(uint32_t*)&frame.left, *(uint32_t*)&frame.top, *(uint32_t)&frame.right, *(uint32_t)&frame.bottom, 
                       (uint32_t)name, resize_mask, flags, (uint32_t)control};
    return emit_haiku_syscall(0x6324, args, 6);
}

// BButton stubs  
status_t GlobalCreateButton(BRect frame, const char* name, uint32_t resize_mask, uint32_t flags, void** button) {
    printf("[libroot_stub] BButton::CreateButton(%.0f,%.0f,%.0f,%.0f, '%s', %u, %u, %p)\n", 
           frame.left, frame.top, frame.right, frame.bottom, name, resize_mask, flags, button);
    uint32_t args[6] = {*(uint32_t*)&frame.left, *(uint32_t*)&frame.top, *(uint32_t)&frame.right, *(uint32_t)&frame.bottom, 
                       (uint32_t)name, resize_mask, flags, (uint32_t)button};
    return emit_haiku_syscall(0x6325, args, 6);
}

// BRaster stubs (drawing acceleration)
status_t GlobalAcquireBitmap(void* bitmap) {
    printf("[libroot_stub] BRaster::AcquireBitmap(%p)\n", bitmap);
    return emit_haiku_syscall(0x6326, (uint32_t*)&bitmap, 1);
}

status_t GlobalReleaseBitmap(void* bitmap) {
    printf("[libroot_stub] BRaster::ReleaseBitmap(%p)\n", bitmap);
    return emit_haiku_syscall(0x6327, (uint32_t*)&bitmap, 1);
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

} // namespace B

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