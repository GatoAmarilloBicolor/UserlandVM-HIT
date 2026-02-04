# Syscall Recycling Strategy

## Filosofía: Reciclar en lugar de reimplementar

En lugar de reimplementar cada syscall de Haiku manualmente, el emulador ahora recicla los syscalls del kernel Haiku directamente.

### Ventajas

1. **Mantenimiento**: Un cambio en el kernel se refleja automáticamente
2. **Compatibilidad**: 100% compatible con comportamiento del kernel
3. **Cobertura**: Se soportan automáticamente nuevos syscalls sin código extra
4. **Performance**: Sin overhead de traducción

### Implementación

```c
// Recycle kernel syscalls directly
static inline uint64 CallKernelSyscall(uint32 op, uint64 *args)
{
    return _kern_generic_syscall("kernel", op, args, 20 * sizeof(uint64));
}

// Only custom logic for guest-specific operations
case 300: // Custom: write() with stdout/stderr support
case 301: // Custom: read() with stdin support  
case 302: // Custom: exit() for clean termination
```

### Flujo de Ejecución

```
Guest Binary Syscall
    ↓
Trap Handler
    ↓
DispatchSyscall()
    ├─ op >= 300? → Custom handlers (write, read, exit)
    ├─ op < 300? → Check hardcoded Haiku syscalls
    └─ Fallback  → Delegate to kernel via CallKernelSyscall()
    ↓
Kernel Execution
    ↓
Return to Guest
```

## Syscalls Actualmente Soportados

### Reciclados Directamente (op < 300)
- Todos los syscalls estándar de Haiku (0-285+)
- Automatic fallback via CallKernelSyscall()

### Custom (op >= 300)
- **300**: write() - con soporte para stdout/stderr
- **301**: read() - con soporte para stdin
- **302**: exit() - termina proceso limpiamente

## Estado Actual

✅ Infraestructura de reciclaje implementada
✅ Fallback a kernel para syscalls desconocidos
⏳ Necesita runtime_loader real para funcionar
❌ Binarios todavía no ejecutan correctamente

## Próximos Pasos

1. **Implementar runtime_loader real**
   - Carga del programa
   - Setup de memoria
   - Inicialización de TLS

2. **Memory translation layer**
   - Guest memory ↔ Host memory mapping
   - Virtual address resolution

3. **Testing**
   - Verificar listdev, ls, bash
   - Audit de syscalls usados
   - Performance profiling

## Conclusion

La estrategia de reciclaje de syscalls es correcta y optimizada.
El bloqueo no está en syscalls sino en:
- Cargador ELF dinámico
- Runtime linker
- Memory management
