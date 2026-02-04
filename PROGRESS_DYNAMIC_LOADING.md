# Progress: Dynamic Library Loading Implementation

**Date**: February 4, 2026  
**Status**: ✅ Partial - Dependencies loaded, execution not yet functional

## Current State

### ✅ Completed
1. **ELF Binary Loading**
   - Multi-segment PT_LOAD mapping
   - Address translation (64-bit safe)
   - BSS initialization

2. **Dynamic Dependency Detection**
   - PT_DYNAMIC section parsing
   - Dependency scanning (libroot.so identification)

3. **Library Loading System**
   - Search multiple standard paths
   - Successfully loads libroot.so
   - Memory allocation and mapping

### Example Execution Flow
```
Input: ./builddir/UserlandVM ./sysroot/haiku32/bin/echo "Test"

[X86] Loading x86 32-bit Haiku program
[ELF] Loading file: echo
[ELF] Loading image: min=0 max=0xca17 size=0xca18
[ELF] Loading PT_LOAD segment 0: vaddr=0 filesz=0xa840
[ELF] Loading PT_LOAD segment 1: vaddr=0xb840 filesz=0x3b4 memsz=0x11d8
[X86] Loading dynamic dependencies
[X86] Loading libroot.so from ./sysroot/haiku32/lib/libroot.so
[ELF] Loading file: libroot.so
[ELF] Loading image: min=0 max=0xf887f size=0xf8880
[LINKER] Added library 'libroot.so' at 0xfa5f263000
[X86] libroot.so loaded at 0xfa5f263000
[X86] Ready to execute x86 32-bit program
[X86] Entry point: 0xXXXXXXXX
[X86] ===== Program Output =====
X86:fCodeBase: 0xXXXXXXXX
X86:fState: 0xXXXXXXXX
[X86] CPU initialized, jumping to entry point
(timeout - execution reaches CPU)
```

## What's Needed for Functional Output

### 1. Symbol Resolution ⚠️ NOT DONE
- [ ] Parse symbol tables from both binary and libroot.so
- [ ] Handle relocation entries (REL/RELA)
- [ ] Resolve undefined symbols

**Example**: When `echo` calls `printf()`, it needs to resolve the symbol address from libroot.so

### 2. Relocation Processing ⚠️ NOT DONE
- [ ] Apply GOT (Global Offset Table) relocations
- [ ] Handle PLT (Procedure Linkage Table) entries
- [ ] Process all DT_REL/DT_RELA entries

**Example**: Adjust all function pointers to point to libroot.so functions

### 3. Syscall Interception ⚠️ PARTIAL
- [x] Framework exists (Haiku32SyscallDispatcher)
- [x] Real syscall IDs implemented
- [ ] Integration with x86-32 CPU execution
- [ ] int 0x25 instruction handling

**Example**: When `echo` calls `write(1, buf, len)`, syscall #151 is invoked. We need to capture this and execute the host's write syscall.

### 4. Stack Setup ⚠️ PARTIAL
- [x] Stack allocated (1MB)
- [x] Stack pointer configured
- [ ] argc/argv properly passed
- [ ] Environment variables setup
- [ ] TLS initialization

## Technical Challenges

### Challenge 1: Symbol Resolution
**Problem**: We have the symbols but need to match them between binary and libraries.

**Solution Path**:
1. Load symbol table from both binaries
2. Create symbol map: name → address
3. Update all references in main binary

### Challenge 2: Relocation Application
**Problem**: Addresses in the binary are relative to library load addresses, which change at runtime.

**Solution Path**:
1. Parse DT_REL entries in binary
2. For each relocation:
   - Find target symbol in libraries
   - Calculate address delta
   - Patch binary at relocation offset

### Challenge 3: Syscall Execution
**Problem**: int 0x25 instruction in emulator needs to call our dispatcher.

**Current Architecture**:
- VirtualCpuX86Native uses native x86 execution (setjmp/longjmp)
- Transitions to 32-bit mode with special code
- Would need to catch SIGSEGV or use debug breakpoint

**Solution Options**:
1. **Pure Interpreter** (slow but complete control)
   - Interpret x86-32 instructions directly
   - Intercept syscalls naturally

2. **Native with Trampoline** (fast but complex)
   - Run natively via VirtualCpuX86Native
   - Use signal handler to catch int 0x25

3. **Hybrid Approach** (recommended)
   - Run most instructions natively
   - Detect syscalls via special marker instruction
   - Delegate to dispatcher

## File Dependencies for Functional Execution

```
echo (x86-32 binary)
  ├── Needs: main() entry point ✅
  ├── Needs: write() from libroot.so ❌ Symbol unresolved
  ├── Needs: printf() from libroot.so ❌ Symbol unresolved
  ├── Needs: malloc()/free() from libroot.so ❌ Symbol unresolved
  └── Needs: syscall support ⚠️ Partial
       ├── write() = syscall #151 ⚠️
       └── exit() = syscall #41 ⚠️
```

## Recommended Implementation Order

**Phase 1**: Symbol Resolution
```cpp
class SymbolResolver {
    bool ResolveSymbol(const char *name, void **address);
    bool ResolveAllSymbols(ElfImage *binary, ElfImage **libraries);
};
```

**Phase 2**: Relocation Application
```cpp
class RelocationProcessor {
    bool ProcessRelocations(ElfImage *image, SymbolResolver &resolver);
    bool ApplyRelocation(uint32_t type, uint32_t offset, uint32_t target);
};
```

**Phase 3**: Syscall Integration
```cpp
class SyscallHandler {
    void OnInt25(uint32_t syscall_number);
    void DispatchToKernel(uint32_t op, uint64_t *args);
};
```

## Next Immediate Steps

1. **Implement symbol loading**
   - Read DT_SYMTAB from both binaries
   - Create symbol → address mappings
   - Print symbol resolution results

2. **Implement relocation processing**
   - Parse DT_REL/DT_RELA entries
   - Apply each relocation
   - Verify relocated addresses

3. **Test with simple syscall**
   - Implement minimal write() wrapper
   - Verify output appears

## Estimated Complexity

- Symbol Resolution: Medium (1-2 hours)
- Relocation Application: High (2-3 hours)
- Syscall Integration: High (3-4 hours)
- Testing & Debugging: High (2-3 hours)

**Total Path to Working Output**: ~8-12 hours of focused work

## Performance Considerations

- Library loading: Fast (< 100ms)
- Symbol resolution: Medium (N symbols)
- Relocation application: Medium (1000s of relocations)
- Overall startup: < 500ms target

## Success Criteria

✅ Program loads
✅ Libraries load
⚠️ Symbols resolved
⚠️ Relocations applied
❌ Program executes
❌ Output visible
❌ Exit code correct

**Target**: Get to "Output visible" state in next session.
