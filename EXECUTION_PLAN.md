# Plan de Ejecución de Software Haiku 32-bit

## Estado Actual

✓ **Compilación**: Proyecto compila sin errores
✓ **Test CPU X86**: VirtualCpuX86Native funciona correctamente  
✓ **Sysroot**: Completo con 175MB de software Haiku 32-bit
✓ **Binarios disponibles**: bash, echo, apps GUI (AboutSystem, DeskCalc, etc.)

## Problemas Actuales

### 1. Carga de ELF Dinámico
El cargador ELF actual:
- ✓ Carga cabeceras ELF básicas
- ✓ Mapea segmentos PT_LOAD
- ✗ No maneja bibliotecas compartidas (.so)
- ✗ No resuelve dependencias dinámicas
- ✗ No implementa dynamic linker

**Solución necesaria:**
- Implementar soporte para PT_DYNAMIC
- Cargar dependencias de `LD.SO` 
- Resolver símbolos en libc.so, libroot.so, etc.

### 2. Syscalls Incompletas
Muchas syscalls de Haiku no están implementadas:
- Threading/semáforos
- I/O de archivos completo
- Entrada/salida estándar
- GUI (app_server)

### 3. Runtime Loader
Haiku requiere `/system/runtime_loader` para:
- Setup inicial de proceso
- TLS (thread local storage)
- Inicialización de libroot

## Pasos Hacia Adelante

### Fase 1: Binarios Estáticos (CORTO PLAZO)
**Objetivo**: Ejecutar un binario Hello World estático x86 32-bit

```bash
# Crear binario estático mínimo
gcc -static -m32 -o hello_static hello.c

# Ejecutar en emulador
./builddir/UserlandVM hello_static
```

**Beneficio**: Verifica núcleo de ejecución sin dependencias

### Fase 2: Soporte de Bibliotecas (MEDIANO PLAZO)
**Objetivo**: Cargar y ejecutar bash dinámico

Necesita:
1. Implementar `ElfImage::LoadSegments()` para PT_DYNAMIC
2. Implementar `DynamicLinker` para resolver símbolos
3. Cargar libc.so.0 de Haiku 32-bit
4. Verificar relaciones de símbolos

### Fase 3: Syscalls de Consola (MEDIANO PLAZO)
**Objetivo**: Ejecutar comandos simple como `echo` y `ls`

Necesita:
1. Implementar syscalls de I/O:
   - write() / read()
   - open() / close()
   - stat() / fstat()
2. Implementar syscalls de proceso:
   - exit()
   - fork() (opcional)
3. Manejo de stdin/stdout/stderr

### Fase 4: GUI Básica (LARGO PLAZO)
**Objetivo**: Ejecutar aplicaciones GUI simple (DeskCalc, AboutSystem)

Necesita:
1. Implementar app_server (mensaje passing)
2. Soporte para BWindow, BView
3. Event loop y message dispatch
4. Renderizado a framebuffer

## Herramientas Disponibles

- **GCC multilib**: Para compilar binarios x86 32-bit
- **Haiku Sysroot**: 175MB completo con todas las librerías
- **VirtualCpuX86Native**: CPU emulator funcional
- **Syscall Dispatcher**: Framework básico existente

## Cambios de Código Necesarios

### 1. DynamicLinker.cpp
Completar implementación de:
```cpp
class DynamicLinker {
    bool LoadDependencies(ElfImage* image);
    void ResolveSymbols(ElfImage* image);
    void ApplyRelocations(ElfImage* image);
};
```

### 2. Syscalls.cpp
Expandir `DispatchSyscall()` con:
- read/write syscalls
- file operations
- process control
- memory management

### 3. Main.cpp
Mejorar `vm_load_image()` para:
- Cargar runtime_loader real
- Setup de commpage
- Inicializar TLS

## Próximos Pasos Recomendados

1. **Esta sesión**: Crear y compilar hello.c estático x86 32-bit
2. **Próxima sesión**: Implementar carga de PT_DYNAMIC
3. **Futuro**: Expandir syscalls según necesidad

## Recursos

- Haiku API: `/boot/home/src/UserlandVM-HIT/sysroot/haiku32/system/develop/`
- Ejemplos: `/boot/home/src/UserlandVM-HIT/sysroot/haiku32/apps/`
- Librerías: `/boot/home/src/UserlandVM-HIT/sysroot/haiku32/lib/`
