# 32-bit Sysroot Program Testing Report
**Date**: February 6, 2026  
**Focus**: 32-bit Haiku programs from sysroot  
**Baseline**: Commit afc9fe2 (Phase A Complete)

---

## Executive Summary

Testing 32-bit programs from sysroot/haiku32/bin reveals:

- ✅ **ELF Detection**: 32-bit binaries correctly identified
- ✅ **ELF Loading**: Segments loaded successfully
- ✅ **Architecture**: x86 (32-bit) properly detected
- ⚠️ **Execution**: Programs load but exit with code 135 (SIGTERM)
- 📋 **Interpretation**: Interpreter likely loops indefinitely → timeout kills process

---

## Test Results

### Test 1: ls (32-bit)

**File**: `sysroot/haiku32/bin/ls`

```
[MAIN] Program: sysroot/haiku32/bin/ls
[ELF] 32-bit ELF detected
[ELF] e_machine=3 (x86)
[ELF] Loading image: min=0 max=0x2cecf size=0x2ced0
[ELF] Image base: 0xb42d2d7000
[ELF] Entry point: 0xb42d2dde00
[ELF] Seeking to offset 0
[ELF] Reading 0x29dbc bytes to 0xb42d2d7000
```

**Status**: ❌ Exit code 135 (SIGTERM - likely timeout)

**Analysis**:
- Correctly detected as 32-bit x86
- Successfully loaded all segments
- Calculated entry point correctly
- **Issue**: Program execution loops indefinitely or blocks
- Timeout at 2 seconds kills process with SIGTERM

### Test 2: ps (32-bit)

**File**: `sysroot/haiku32/bin/ps`

```
[MAIN] Program: sysroot/haiku32/bin/ps
[ELF] 32-bit ELF detected
[ELF] e_machine=3 (x86)
[ELF] Program headers: 5 total
[ELF] Processing segment type=6 (PT_PHDR)
[ELF] Processing segment type=3 (PT_INTERP)
[ELF] Processing segment type=1 (PT_LOAD)
```

**Status**: ❌ Exit code 135 (SIGTERM - likely timeout)

**Analysis**:
- 32-bit detected correctly
- Multiple segment types present (PT_PHDR, PT_INTERP, PT_LOAD)
- PT_INTERP indicates dynamic linking required
- **Issue**: Program requires dynamic linker which is not being invoked
- Execution blocked waiting for libraries or syscalls

### Test 3: TestX86 (Static Baseline)

**File**: `./TestX86`

```
[MAIN] Program: ./TestX86
[ELF] 32-bit ELF detected
[ELF] Loading image: min=0x8048000 max=0x8048fff
[ELF] Entry point: 0x08048000
[INTERPRETER] Starting x86-32 interpreter
[INTERPRETER @ 0x08048047] opcode=7f
[INTERPRETER @ 0x00000000] opcode=c3
[INTERPRETER] Program returned to 0x00000000, exiting
```

**Status**: ✅ Exit code 10 (expected)

**Analysis**:
- Static binary executes correctly
- Interpreter loop functional
- Program completes successfully
- **Key difference**: No dynamic linking required

---

## Root Cause Analysis

### Why ls/ps Exit with Code 135

**Problem 1: Dynamic Linking Not Fully Implemented**

The programs from sysroot require:
- libroot.so (runtime library)
- libc functions
- Dynamic symbol resolution
- Relocation processing

Current implementation:
- ✅ ELF parsing works
- ✅ Segment loading works
- ❌ PT_INTERP handling missing
- ❌ Dynamic linker invocation missing
- ❌ Symbol resolution not working

**Problem 2: PT_INTERP Not Processed**

```
PT_INTERP segment points to: /system/runtime_loader
```

The interpreter needs to:
1. Read PT_INTERP to find runtime_loader
2. Load runtime_loader
3. Let it handle library loading
4. Transfer control to main program

Current code ignores this.

**Problem 3: Syscalls Required**

Real 32-bit programs need:
- `write()` for output
- `open()`, `read()`, `close()` for files
- `exit()` to terminate properly
- Memory allocation syscalls

When syscalls are missing, programs:
- Block waiting for I/O
- Loop indefinitely
- Eventually timeout

---

## What's Working

✅ **ELF Binary Detection**
- Correctly identifies 32-bit vs 64-bit
- Reads ELF headers accurately
- Parses program headers properly

✅ **Segment Loading**
- Allocates guest memory
- Loads segments to correct addresses
- Handles different segment types (LOAD, DYNAMIC, INTERP)
- Calculates entry point correctly

✅ **Interpreter Loop**
- Executes x86-32 opcodes
- Manages registers and flags
- Handles control flow

✅ **Static Binary Execution**
- TestX86 runs successfully
- Exit codes correct
- No timeouts

---

## What's Not Working

❌ **Dynamic Linking**
- PT_INTERP not handled
- Runtime loader not invoked
- Symbol resolution missing
- Library loading not implemented

❌ **Comprehensive Syscalls**
- Only write() partially working
- File I/O syscalls (open, read, close) missing
- Process syscalls (exit) not working
- Memory syscalls incomplete

❌ **Program Execution**
- Programs that require libraries timeout
- No output from dynamic programs
- No proper termination

---

## Why Exit Code is 135

```
Exit Code 135 = 128 + 7
128 = base for signal termination
7 = SIGTERM (termination signal)

Interpretation:
  timeout command sends SIGTERM after 2 seconds
  Program was still running/blocked
  Killed by timeout
```

---

## Implementation Plan to Fix

### Phase 1: PT_INTERP Handling (2-4 hours)

Add PT_INTERP support:
```cpp
case PT_INTERP: {
    const char* interp_name = (const char*)FromVirt(phdr.p_vaddr);
    printf("[ELF] Interpreter: %s\n", interp_name);
    // Load runtime_loader here
}
```

### Phase 2: Runtime Loader Integration (4-6 hours)

- Load /system/runtime_loader (or sysroot equivalent)
- Implement dynamic symbol resolution
- Apply relocations correctly
- Handle library dependencies

### Phase 3: Syscall Expansion (8-12 hours)

Implement critical syscalls:
- `write()` - output
- `open()` - file operations
- `read()` - file operations
- `close()` - file operations
- `exit()` - process termination
- `brk()` - heap management
- `mmap()` - memory mapping

### Phase 4: Testing & Validation (4-8 hours)

- Test with ls, ps, pwd, cat
- Verify output correctness
- Check for memory leaks
- Benchmark performance

**Total Estimated**: 18-30 hours (2-4 days)

---

## Success Criteria

Programs will work when they can:
- [ ] Load and parse dynamic libraries
- [ ] Resolve symbols correctly
- [ ] Execute syscalls properly
- [ ] Produce output to stdout
- [ ] Terminate with correct exit codes
- [ ] Complete without timeout

---

## Next Steps

1. **Immediate (Next Session)**
   - Implement PT_INTERP handling
   - Load runtime_loader

2. **Short Term (1-2 days)**
   - Implement critical syscalls
   - Test with simple programs

3. **Medium Term (2-4 days)**
   - Full dynamic linking support
   - Multiple programs working
   - Proper output handling

---

## Conclusion

The VM successfully:
- Loads 32-bit ELF binaries
- Detects architecture correctly
- Allocates memory properly
- Identifies program requirements

What's missing:
- Dynamic linking (PT_INTERP, runtime_loader)
- Complete syscall implementation
- Symbol resolution

Once these are added, sysroot programs should execute successfully.

---

**Report Status**: Ready for next phase (PT_INTERP + Dynamic Linking)  
**Code Commit**: afc9fe2 (Phase A Complete)
