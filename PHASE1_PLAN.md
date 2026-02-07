# Fase 1: PT_INTERP Handler Implementation Plan

## Objetivo
Detectar e implementar el manejador PT_INTERP (dynamic linking) para preparar ejecución.

## Componentes Requeridos

### 1. PT_INTERP Detector (en Loader.cpp)
- Leer ruta del intérprete desde PT_INTERP
- Típicamente: `/system/runtime_loader`
- Almacenar en ElfImage

### 2. Dynamic Linker (Nuevo archivo)
- Cargar el runtime_loader
- Resolver símbolos básicos (11)
- Vincular dependencias

### 3. Symbol Resolver (Extensión)
Símbolos Haiku core:
```
1. _dyld_call_init_routine
2. __cxa_atexit
3. __cxa_finalize
4. malloc
5. free
6. strlen
7. strcpy
8. memcpy
9. exit
10. printf
11. memset
```

### 4. Extended Main (Main.cpp)
- Detectar si programa es dinámico
- Si tiene PT_INTERP → usar dynamic linker
- Preparar para ejecución (no ejecutar aún)

## Etapas

### Etapa 1: Agregar GetInterpreter() a Loader
- Extender Loader.h con método virtual
- Implementar en ElfImageImpl
- Leer string de PT_INTERP

### Etapa 2: Crear DynamicLinker class
- Cargar runtime_loader
- Mantener tabla de símbolos

### Etapa 3: Integrar en Main
- Detectar programa dinámico
- Crear DynamicLinker si necesario
- Reportar información de linking

### Etapa 4: Testing
- Verificar detección PT_INTERP
- Validar carga runtime_loader
- Confirmar resolución de símbolos

## Archivos a Modificar/Crear
- Loader.h (agregar método)
- Loader.cpp (implementar lectura PT_INTERP)
- DynamicLinker.h (nueva clase)
- DynamicLinker.cpp (nueva implementación)
- Main.cpp (integración)

## Métricas de Éxito
✅ Detecta PT_INTERP en binarios dinámicos
✅ Lee ruta del intérprete correctamente
✅ Resuelve 11 símbolos básicos
✅ HaikuDepot muestra información de linking
✅ Código compila sin errores

## Tiempo Estimado
4-6 commits, ~500 líneas de código nuevo
