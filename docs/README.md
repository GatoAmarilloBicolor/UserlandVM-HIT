# UserlandVM Project: Estado Actual y Próximos Pasos (Pre-Alpha 0.1.x)

Este documento describe el estado actual del proyecto UserlandVM, un emulador de máquina virtual en espacio de usuario para Haiku OS, con un enfoque inicial en la emulación de binarios x86 de 32 bits.

**Estado:** Sprint 3 COMPLETADO (100%) | Última revisión: 2025-11-30  
**Arquitectura:** Evolutive Framework (7 Pilares Fundamentales)  
**Licencia:** MIT/BSD (100% compatible)  
**Hito Actual:** ✅ Hello World x86-32 ejecutándose correctamente

## 1. Visión General del Proyecto

UserlandVM busca permitir la ejecución de código de sistemas operativos invitados y aplicaciones dentro de un entorno de usuario en Haiku. Actualmente, el esfuerzo se concentra en establecer una base estable para la emulación de programas x86 de 32 bits.

### Filosofía Central
**Construir para el futuro, no solo para hoy.**

La arquitectura prioriza modularidad, extensibilidad y la creación de un framework que pueda evolucionar para soportar otras arquitecturas y personalidades de SO sin requerir una reingeniería del núcleo.

## 2. Arquitectura Clave: El `guest_loader_32bit`

Para superar la limitación de Haiku OS de no permitir que un proceso de 64 bits cambie directamente su modo de CPU a 32 bits, hemos introducido una pieza arquitectónica crucial: el `guest_loader_32bit`.

*   **Propósito:** Es un ejecutable compilado como un programa nativo de 32 bits para Haiku.
*   **Función:** Su rol principal es cargar y ejecutar el binario invitado real (el programa de 32 bits que se desea emular) utilizando la llamada al sistema `load_image()` de Haiku. Al ser un proceso de 32 bits, garantiza que el kernel cargue el binario invitado en un entorno de proceso de 32 bits adecuado.
*   **Claridad para el Usuario:** Cuando el `guest_loader_32bit` lanza un programa invitado, el kernel de Haiku crea un *nuevo proceso* para ese invitado, que aparecerá con su propio nombre (ej. `TestX86.exe`). Esto asegura que el usuario pueda identificar claramente los procesos en ejecución.

## 3. Estado Actual del Desarrollo (Pre-Alpha 0.1.x - Sprint 2 en Progreso)

Nos encontramos en la **Fase 1: Estabilización Fundamental (Enfoque: Núcleo x86 y Compilación)**, completando el **Sprint 2: "Hola, Mundo"**. El progreso hasta ahora incluye:

### ✅ Completado (Sprint 1 - "El Esqueleto")
*   **Definición Arquitectónica:** Se ha establecido la necesidad y el rol del `guest_loader_32bit` como intermediario para la ejecución de binarios de 32 bits.
*   **Interfaces Base (TCI):** Implementadas 4 interfaces abstractas:
    - `GuestContext.h` - Interfaz para el estado de la CPU
    - `AddressSpace.h` - Interfaz para gestión de memoria
    - `ExecutionEngine.h` - Interfaz para motor de ejecución
    - `SyscallDispatcher.h` - Interfaz para manejador de syscalls
*   **Implementaciones Concretas:**
    - `DirectAddressSpace` (x86-32) - Gestión de memoria con `create_area()`
    - `X86_32GuestContext` - Almacenamiento de registros x86 de 32 bits
*   **Estructura de Directorios `non-packaged`:** Se ha creado la estructura base en `/boot/home/config/non-packaged/UserlandVM32/` con subdirectorios `bin/` y `lib/`.
*   **Preparación del Entorno de Compilación Portable:** Se ha creado un entorno mínimo (`userlandvm_32bit_build_env/`) con los archivos fuente y un script `build.sh`.

### 🔄 En Progreso (Sprint 2 - "Hola, Mundo")
*   **Cargador ELF MIT/BSD Compatible:** Implementada `GuestElfLoader` para cargar binarios ELF32 de x86:
    - Parseo de headers ELF
    - Mapeo de segmentos PT_LOAD
    - Configuración inicial de registros y stack
    - 100% compatible con licencia MIT/BSD (sin GPLv3)
*   **Refactorización de Main.cpp:** Simplificado y refocalizado en x86-32:
    - Eliminación de IPC y RISC-V
    - Carga de ejecutables huéspedes
    - Visualización de estado inicial de CPU
*   **Sistema de Manejo de Errores:** Reemplazo de `abort()` con `status_t` en:
    - `Loader.cpp` / `Loader.h`
    - `VirtualCpuX86Test.cpp`
    - `Main.cpp`
*   **Build System (Meson):** Revisión y corrección de:
    - Compilación de asmjit como librería estática
    - Inclusión de headers de Haiku (AutoDeleter.h, SupportDefs.h)
    - Integración de subproyectos (zydis, zycore)

## 4. Próximos Pasos Clave (Fase 1 - Roadmap)

### Inmediatos (Sprint 2 - Finalización)
*   **Validación de Compilación:** Completar build con asmjit (en progreso)
*   **Tests Unitarios:** Ejecutar `VirtualCpuX86Test.cpp` para validar carga ELF
*   **Test de Integración:** "Hello World" estático en x86-32

### Sprint 3: "Herramientas Básicas"
*   **Syscalls Fundamentales:** Implementar soporte en `SyscallDispatcher`:
    - `write(fd, buf, count)` - Salida a stdout/stderr
    - `exit(code)` - Terminar proceso
    - `brk(addr)` - Gestión de heap
*   **Syscall Tracer:** Módulo `uvm_trace` utilizando API de instrumentación
*   **Soporte para Herramientas UNIX:** Hacedor que `ls` y `cat` estáticos funcionen

### Sprint 4: "Optimización y Seguridad"
*   **Motor de Ejecución Híbrido:** Implementar IHJP (Intérprete + JIT Promocional)
    - Intérprete puro (seguro)
    - Monitor de "hot spots"
    - Compilación JIT en background
    - Reemplazo atómico de código
*   **Políticas de Seguridad:**
    - W^X (Write XOR Execute) - Memoria nunca escribible y ejecutable simultáneamente
    - ASLR (Address Space Layout Randomization)
*   **Soporte TLS:** Thread-Local Storage para x86-32

### Post-Sprint 4: "Extensión"
*   **Asignación de Memoria Avanzada:** Gestión de mmap, shm
*   **Capa VFS:** Virtualización de sistema de archivos
*   **Soporte Dinámico:** Enlace dinámico y carga de librerías
*   **Otras Arquitecturas:** RISC-V, ARM (usando mismo framework)

## 5. Despliegue de Binarios de 32 bits (Estrategia `non-packaged`)

Para el despliegue de `guest_loader_32bit` y sus dependencias en un sistema Haiku de 64 bits, se sigue la siguiente estrategia:

1.  **Directorio Base:** `/boot/home/config/non-packaged/UserlandVM32/`.
2.  **Ejecutable:** El `guest_loader_32bit` compilado se colocará en `/boot/home/config/non-packaged/UserlandVM32/bin/`.
3.  **Bibliotecas de 32 bits:** Las bibliotecas del sistema Haiku de 32 bits (`libbe.so`, `libroot.so`, etc.) se copiarán desde una instalación de Haiku de 32 bits (o `/boot/system/lib/x86` en un sistema de 64 bits) a `/boot/home/config/non-packaged/UserlandVM32/lib/`.
4.  **`rpath` (Recomendado):** Se recomienda compilar el `guest_loader_32bit` con un `rpath` incrustado que apunte a `$ORIGIN/../lib` para que encuentre sus dependencias de forma autocontenida y robusta.

## 6. Entorno de Compilación Portable para `guest_loader_32bit`

Se ha preparado un entorno mínimo para compilar el `guest_loader_32bit` en una máquina Haiku de 32 bits. Este entorno incluye:

*   `userlandvm_32bit_build_env/` (directorio raíz)
    *   `build.sh` (script de compilación)
    *   `meson.build` (archivo de configuración de Meson)
    *   `guest_loader_32bit/main.cpp` (código fuente del cargador)

**Instrucciones de Uso del Entorno Portable:**

1.  Copiar la carpeta `userlandvm_32bit_build_env` completa a la máquina Haiku de 32 bits de destino.
2.  Asegurarse de que la máquina de destino tenga las herramientas de desarrollo de Haiku instaladas (`pkgman install haiku_devel`).
3.  Abrir una terminal en el directorio `userlandvm_32bit_build_env` y ejecutar el script: `./build.sh`.
4.  El ejecutable compilado `guest_loader_32bit` se encontrará en `./build/guest_loader_32bit`.

## 7. Licencia y Compliance

El proyecto UserlandVM se distribuye bajo la licencia **MIT/BSD**. Se ha realizado auditoría exhaustiva para garantizar:

- ✅ Eliminación de dependencias GPLv3 (rvvm compatibility removido)
- ✅ Uso exclusivo de librerías MIT/BSD-compatible:
  - zydis (MIT)
  - zycore (MIT)
  - asmjit (Zlib)
- ✅ Código original y derivados bajo MIT/BSD

Todas las contribuciones deben mantener esta compatibilidad de licencia.

## 8. Arquitectura del Framework (7 Pilares)

Para detalles completos de la arquitectura "Evolutive Framework", consultar `Proyecto.txt`:

1. **TCI (Guest Context Translator)** - Interfaz core
2. **Plugin System para Motores de Ejecución** - Extensibilidad
3. **MDGP (Direct Memory Mapping with Guard Pages)** - Gestión de memoria
4. **CELFC (ELF Compatible Loader)** - Carga de binarios
5. **Syscall Dispatcher by Personality** - Manejo de syscalls
6. **VFS Layer** - Virtualización de filesystem
7. **Instrumentation API** - Debugging y análisis

## 9. Documentación Generada

- `COMPILATION_STATUS_2025_11_28.md` - Análisis detallado de cambios y estado de compilación
- `HIT_PROGRESS_LOG.txt` - Log detallado de progreso del Haiku Imposible Team
- `Proyecto.txt` - Especificación completa del Evolutive Framework
- `README.md` (este archivo) - Introducción y roadmap en inglés
- `README.es.md` - Introducción y roadmap en español
- `README.ru.md`, `README.uk.md` - Documentación en ruso y ucraniano

## 10. Roadmap Completo: Hacia Aplicaciones Gráficas

Para una visión detallada del camino desde "Hello World" hasta aplicaciones gráficas completas, consultar:

**`ROADMAP_TO_GUI_2025_11_30.md`** - Incluye:
- Timeline estimado: Sprint 3-9 (~4-5 meses)
- Fases de desarrollo (Core → Syscalls → App Kit → GUI)
- Requisitos técnicos por fase
- Estrategias de aceleración
- Análisis de riesgos y mitigación

**Hito General:**
- **Sprint 4-5:** Dinámico linking & File I/O (5-6 semanas)
- **Sprint 6-7:** App Kit & Graphics (4-5 semanas)
- **Sprint 8-9:** Aplicaciones GUI (Paint, Calculator, Terminal) (3-4 semanas)
- **Objetivo:** Principios de 2026

## 11. Contribuciones

Las contribuciones son bienvenidas. Por favor:

1. Mantener compatibilidad de licencia MIT/BSD
2. Seguir el "Definition of Done" del proyecto:
   - Análisis estático obligatorio (CI)
   - Tests unitarios para nuevas funciones
   - Documentación actualizada
   - Cumplimiento de estilos de código C++17
3. Consultar `Proyecto.txt` para la filosofía arquitectónica

## 12. Documentación Completa

### Documentos Principales
- `README.md` - Esta guía (English/Spanish)
- `Proyecto.txt` - Especificación de arquitectura 7-pillar
- `ROADMAP_TO_GUI_2025_11_30.md` - Plan detallado hacia GUI ⭐ **NEW**

### Reportes de Sprint
- `SPRINT_3_FINAL_SUMMARY_2025_11_30.md` - Logros de Sprint 3
- `SPRINT_3_COMPLETION_2025_11_30.md` - Detalles de implementación
- `PROJECT_STATUS_2025_11_30_FINAL.md` - Estado actual del proyecto

## 13. Contacto y Soporte

Proyecto bajo desarrollo activo por el Haiku Imposible Team.  
Última actualización: 2025-11-30 (Sprint 3 ✅ Completado, Sprint 4 🚀 Iniciando)
