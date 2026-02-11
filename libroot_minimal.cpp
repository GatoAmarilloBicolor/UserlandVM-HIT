/*
 * libroot_minimal.cpp - Implementación mínima del stub de libroot
 * 
 * Versión minimalista sin clases C++ para evitar errores
 */

#include <cstdio>
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////
// Tipos básicos
///////////////////////////////////////////////////////////////////////////

typedef uint32_t status_t;
typedef uintptr_t haiku_ptr_t;

// Tipo de función handler
typedef uint32_t (*haiku_syscall_handler_t)(uint32_t syscall_num, uint32_t* args, uint32_t arg_count);

// Constantes
#define B_OK                    0
#define B_ERROR                 ((status_t)-1)

///////////////////////////////////////////////////////////////////////////
// Variables globales
///////////////////////////////////////////////////////////////////////////

static haiku_syscall_handler_t g_haiku_handler = nullptr;

///////////////////////////////////////////////////////////////////////////
// Función de registro
///////////////////////////////////////////////////////////////////////////

extern "C" void register_haiku_syscall_handler(haiku_syscall_handler_t handler)
{
    printf("[libroot_minimal] Registrando manejador de syscalls Haiku en: %p\n", handler);
    g_haiku_handler = handler;
}

///////////////////////////////////////////////////////////////////////////
// Función helper para emitir INT 0x63
///////////////////////////////////////////////////////////////////////////

static inline status_t emit_haiku_syscall(uint32_t syscall_num, uint32_t* args, uint32_t arg_count)
{
    if (!g_haiku_handler) {
        printf("[libroot_minimal] ERROR: No hay manejador de syscalls registrado\n");
        return B_ERROR;
    }
    
    printf("[libroot_minimal] Emitiendo syscall Haiku 0x%04X con %u argumentos\n", syscall_num, arg_count);
    
    // Emitir INT 0x63 con el número de syscall y argumentos
    // Esto será interceptado por el UserlandVM
    uint32_t result = g_haiku_handler(syscall_num, args, arg_count);
    
    return (status_t)result;
}

///////////////////////////////////////////////////////////////////////////
// Funciones exportadas del stub
///////////////////////////////////////////////////////////////////////////

extern "C" {
    
    // BMessage functions
    status_t BMessage_AddInt32(const char* name, int32_t value) {
        printf("[libroot_minimal] BMessage::AddInt32('%s', %d)\n", name, value);
        
        uint32_t args[2];
        args[0] = (haiku_ptr_t)name;
        args[1] = (haiku_ptr_t)(uintptr_t)value;
        
        return emit_haiku_syscall(0x6301, args, 2);
    }
    
    status_t BMessage_AddString(const char* name, const char* string) {
        printf("[libroot_minimal] BMessage::AddString('%s', '%s')\n", name, string);
        
        uint32_t args[2];
        args[0] = (haiku_ptr_t)name;
        args[1] = (haiku_ptr_t)string;
        
        return emit_haiku_syscall(0x6302, args, 2);
    }
    
    status_t BMessage_AddPointer(const char* name, void* pointer) {
        printf("[libroot_minimal] BMessage::AddPointer('%s', %p)\n", name, pointer);
        
        uint32_t args[2];
        args[0] = (haiku_ptr_t)name;
        args[1] = (haiku_ptr_t)pointer;
        
        return emit_haiku_syscall(0x6303, args, 2);
    }
    
    status_t BMessage_FindInt32(const char* name, int32_t* value) {
        printf("[libroot_minimal] BMessage::FindInt32('%s')\n", name);
        
        uint32_t args[2];
        args[0] = (haiku_ptr_t)name;
        args[1] = (haiku_ptr_t)(uintptr_t)value;
        
        return emit_haiku_syscall(0x6304, args, 2);
    }
    
    status_t BMessage_FindString(const char* name, const char** string) {
        printf("[libroot_minimal] BMessage::FindString('%s')\n", name);
        
        uint32_t args[2];
        args[0] = (haiku_ptr_t)name;
        args[1] = (haiku_ptr_t)string;
        
        return emit_haiku_syscall(0x6305, args, 2);
    }
    
    // BWindow functions
    status_t BWindow_Show(const char* title) {
        printf("[libroot_minimal] BWindow::Show('%s')\n", title);
        
        uint32_t args[1];
        args[0] = (haiku_ptr_t)title;
        
        return emit_haiku_syscall(0x6309, args, 1);
    }
    
    status_t BWindow_Hide(const char* title) {
        printf("[libroot_minimal] BWindow::Hide('%s')\n", title);
        
        uint32_t args[1];
        args[0] = (haiku_ptr_t)title;
        
        return emit_haiku_syscall(0x630A, args, 1);
    }
    
    status_t BWindow_MoveTo(const char* title, float x, float y) {
        printf("[libroot_minimal] BWindow::MoveTo('%s', %.0f,%.0f)\n", title, x, y);
        
        uint32_t args[3];
        args[0] = (haiku_ptr_t)title;
        args[1] = (haiku_ptr_t)(uintptr_t)&x;
        args[2] = (haiku_ptr_t)(uintptr_t)&y;
        
        return emit_haiku_syscall(0x630B, args, 3);
    }
    
    status_t BWindow_ResizeTo(const char* title, float width, float height) {
        printf("[libroot_minimal] BWindow::ResizeTo('%s', %.0f,%.0f)\n", title, width, height);
        
        uint32_t args[3];
        args[0] = (haiku_ptr_t)title;
        args[1] = (haiku_ptr_t)(uintptr_t)&width;
        args[2] = (haiku_ptr_t)(uintptr_t)&height;
        
        return emit_haiku_syscall(0x630C, args, 3);
    }
    
    status_t BWindow_SetTitle(const char* old_title, const char* new_title) {
        printf("[libroot_minimal] BWindow::SetTitle('%s' -> '%s')\n", old_title, new_title);
        
        uint32_t args[2];
        args[0] = (haiku_ptr_t)old_title;
        args[1] = (haiku_ptr_t)new_title;
        
        return emit_haiku_syscall(0x630F, args, 2);
    }
    
    // BApplication functions
    status_t BApplication_Run(const char* signature) {
        printf("[libroot_minimal] BApplication::Run('%s')\n", signature);
        
        uint32_t args[1];
        args[0] = (haiku_ptr_t)signature;
        
        return emit_haiku_syscall(0x6310, args, 1);
    }
    
    status_t BApplication_Quit(const char* signature) {
        printf("[libroot_minimal] BApplication::Quit('%s')\n", signature);
        
        uint32_t args[1];
        args[0] = (haiku_ptr_t)signature;
        
        return emit_haiku_syscall(0x6311, args, 1);
    }
}

///////////////////////////////////////////////////////////////////////////
// Constructor y destructor del módulo
///////////////////////////////////////////////////////////////////////////

__attribute__((constructor))
void libroot_init() {
    printf("[libroot_minimal] libroot.so stub inicializado\n");
    printf("[libroot_minimal] Listo para emitir syscalls Haiku via INT 0x63\n");
}

__attribute__((destructor))
void libroot_fini() {
    printf("[libroot_minimal] libroot.so stub finalizado\n");
}