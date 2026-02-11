# UserlandVM-HIT: Next Phase - Symbol Implementation

## Current Status

**Harmonization Complete ✓**
- All subsystems compiled and linked
- Build: 120KB, zero errors
- Components initialized in correct order
- Symbol table: 35 entries (libc + libbe)

**Currently Working ✓**
- ELF loading and memory mapping
- Static x86-32 program execution
- AppServerBridge window registry
- Dynamic symbol resolution infrastructure

**Currently Not Working ✗**
- Symbol stub execution (stubs are placeholder addresses)
- Dynamic linking and relocation
- Graphics rendering (calls reach renderer but drawing fails)
- WebPositive execution (requires above)

## Why WebPositive Doesn't Work Yet

WebPositive is a **dynamic** Haiku application that:
1. Calls `libc` functions (printf, malloc, free, etc.) - **STUBS NOT IMPLEMENTED**
2. Calls `libbe` functions (BApplication, BWindow, BView) - **STUBS NOT IMPLEMENTED**
3. Requires relocation processing - **NOT IMPLEMENTED**
4. Needs proper stack/memory management - **PARTIAL**

When WebPositive calls `malloc` at 0x10001000, it hits a **stub address with no implementation**, causing segfault or incorrect behavior.

## Phase 1: Implement Symbol Stubs

### Goal
Replace placeholder addresses with minimal working implementations of key functions.

### Required Libc Stubs

```c
void* malloc(size_t size)
    - Allocate from guest heap
    - Return pointer to guest memory
    - Maintain allocation list

void free(void* ptr)
    - Deallocate memory
    - Update allocation list

int printf(const char* format, ...)
    - Format string and arguments
    - Output to host stdout
    - Return bytes written

int puts(const char* str)
    - Write string and newline
    - Output to host stdout

void* memcpy(void* dst, const void* src, size_t n)
void* memset(void* ptr, int val, size_t n)
size_t strlen(const char* str)
int strcmp(const char* a, const char* b)
char* strcpy(char* dst, const char* src)
char* strncpy(char* dst, const char* src, size_t n)

void exit(int status)
    - Halt execution with status

void abort()
    - Terminate immediately

int __libc_start_main(int (*main)(int, char**, char**), int argc, char** argv, ...)
    - Set up stack
    - Call main
    - Handle exit
```

### Required Libbe Stubs (Minimal)

```cpp
BApplication::BApplication(const char* signature)
    - Create app instance
    - Register with app_server (via AppServerBridge)

BApplication::Run()
    - Event loop
    - Process events from AppServerBridge

BApplication::Quit()
    - Shutdown cleanly

BWindow::BWindow(BRect frame, const char* title, window_type type, int flags)
    - Create window
    - Register with app_server

BWindow::Show()
BWindow::Hide()
BWindow::Quit()
BWindow::AddChild(BView* view)
BWindow::Bounds()

BView::BView(BRect frame, const char* name, int resizeMask, int flags)
BView::SetViewColor(rgb_color color)
BView::SetHighColor(rgb_color color)
BView::FillRect(BRect rect)
BView::StrokeLine(BPoint from, BPoint to)
BView::DrawString(const char* text, BPoint where)
BView::Invalidate()
```

### Implementation Strategy

1. **Memory Layout**
   ```
   Guest Memory (64MB)
   ├── Heap (dynamic): 0x00000000 - 0x10000000
   ├── libc stubs:     0x10000000 - 0x20000000
   ├── libbe stubs:    0x20000000 - 0x30000000
   └── Program code:   0x30000000 - 0x40000000
   ```

2. **Stub Generation**
   - Create stub function objects in guest memory
   - Each stub: metadata + minimal code
   - Entry points at base addresses (0x1000xxxx, 0x2000xxxx)

3. **Stub Execution**
   - Interpreter recognizes stub entry points
   - Executes native host code directly (not guest x86)
   - Returns to guest code with result in EAX

4. **Alternative: Opcode Replacement**
   - Place special opcodes (e.g., 0xFF) at entry points
   - Interpreter catches these opcodes
   - Routes to native implementations

## Phase 2: ELF Relocation Processing

### Current Gap
ELF relocation sections (.rel.dyn, .rel.plt) are **not processed**.
This means:
- GOT entries are not filled with symbol addresses
- PLT trampolines don't know symbol targets
- Indirect calls fail

### Implementation

```c
void process_relocations(ELF* elf, Memory* mem, SymbolTable* syms) {
    // Parse .rel.dyn (absolute relocations)
    for each relocation in .rel.dyn:
        symbol = resolve_symbol(relocation.sym)
        if relocation.type == R_386_32:
            mem[relocation.offset] = symbol.address
        elif relocation.type == R_386_RELATIVE:
            mem[relocation.offset] += base_address
    
    // Parse .rel.plt (PLT relocations)
    for each relocation in .rel.plt:
        symbol = resolve_symbol(relocation.sym)
        if relocation.type == R_386_JUMP_SLOT:
            mem[relocation.offset] = symbol.address
}
```

## Phase 3: Memory Management

### Heap Allocator
Implement simple heap for `malloc`/`free`:

```cpp
class GuestHeap {
private:
    uint32_t heap_base = 0x1000000;
    uint32_t heap_offset = 0;
    std::map<uint32_t, size_t> allocations;
    
public:
    uint32_t allocate(size_t size) {
        uint32_t ptr = heap_base + heap_offset;
        allocations[ptr] = size;
        heap_offset += (size + 15) & ~15;  // Align to 16 bytes
        return ptr;
    }
    
    void deallocate(uint32_t ptr) {
        allocations.erase(ptr);
    }
};
```

## Implementation Priority

### High Priority (blocks WebPositive)
1. malloc/free - needed for all dynamic allocation
2. printf - debugging output
3. memcpy/memset/strlen/strcmp - string operations
4. BApplication/BWindow/BView constructors

### Medium Priority (improves functionality)
5. strcpy/strncpy - string manipulation
6. exit/__libc_start_main - proper program flow
7. Other Be API methods

### Low Priority (nice-to-have)
8. abort - error handling
9. puts - simpler output

## File Changes Needed

### New Files
- **SymbolStubs.cpp/h** - Stub implementations
- **GuestHeap.cpp/h** - Memory allocator
- **RelocationProcessor.cpp/h** - ELF relocation handler

### Modified Files
- **userlandvm_haiku32_master.cpp** - Add stub initialization
- **CompleteELFDynamicLinker.cpp** - Link to stubs after loading
- **InterpreterX86_32.cpp** - Recognize stub entry points

## Testing Strategy

1. **Test malloc/free**
   ```bash
   gcc -c test_malloc.c -m32 -march=i686
   ./userlandvm_harmonized test_malloc
   ```

2. **Test printf**
   ```bash
   gcc -c test_printf.c -m32 -march=i686
   ./userlandvm_harmonized test_printf
   # Should print: "Hello from WebPositive"
   ```

3. **Test string functions**
   ```bash
   ./userlandvm_harmonized test_strings
   ```

4. **Test WebPositive**
   ```bash
   ./userlandvm_harmonized sysroot/haiku32/bin/webpositive
   # Should show window with rendering
   ```

## Success Criteria

- [ ] malloc/free working (allocation/deallocation)
- [ ] printf working (output to console)
- [ ] String functions working (strlen, strcmp, strcpy, etc.)
- [ ] Be API constructors called successfully
- [ ] WebPositive starts without segfault
- [ ] WebPositive window renders content
- [ ] WebPositive responds to user events

## Estimated Effort

- **Stub implementations**: 2-3 hours
- **ELF relocation**: 1-2 hours
- **Memory allocator**: 1 hour
- **Testing & debugging**: 2-3 hours
- **Total**: ~6-9 hours to WebPositive baseline

## Current Symbol Table (For Reference)

```
 0: _ZN12BApplication3RunEv                  @ 0x20001200
 1: _ZN12BApplication4QuitEv                 @ 0x20001300
 2: _ZN12BApplicationC1EPKc                  @ 0x20001000
 3: _ZN12BApplicationD1Ev                    @ 0x20001100
 4: _ZN5BView10DrawStringEPKcNS_6BPointE     @ 0x20002200
 5: _ZN5BView10InvalidateEv                  @ 0x20002300
 6: _ZN5BView12SetViewColorE7rgb_color       @ 0x20001e00
 7: _ZN5BView13SetHighColorE7rgb_color       @ 0x20001f00
 8: _ZN5BView4DrawEh                         @ 0x20001d00
 9: _ZN5BView8StrokeLineENS_6BPointES0_      @ 0x20002100
10: _ZN5BView9FillRectENS_5BRectE            @ 0x20002000
11: _ZN5BView9FindViewEPKc                   @ 0x20002400
12: _ZN5BViewC1EN5BRectS0_PKcjj              @ 0x20001b00
13: _ZN5BViewD1Ev                            @ 0x20001c00
14: _ZN7BWindow4HideEv                       @ 0x20001700
15: _ZN7BWindow4QuitEv                       @ 0x20001800
16: _ZN7BWindow4ShowEv                       @ 0x20001600
17: _ZN7BWindow8AddChildEP5BView             @ 0x20001900
18: _ZN7BWindow8BoundsEv                     @ 0x20001a00
19: _ZN7BWindowC1EN5BRectS0_PKcjj            @ 0x20001400
20: _ZN7BWindowD1Ev                          @ 0x20001500
21: __libc_start_main                        @ 0x10007000
22: abort                                    @ 0x10006100
23: exit                                     @ 0x10006000
24: free                                     @ 0x10002000
25: malloc                                   @ 0x10001000
26: memcpy                                   @ 0x10005000
27: memset                                   @ 0x10005100
28: printf                                   @ 0x10003000
29: puts                                     @ 0x10003100
30: rgb_color                                @ 0x20002500
31: strcmp                                   @ 0x10004100
32: strcpy                                   @ 0x10004200
33: strlen                                   @ 0x10004000
34: strncpy                                  @ 0x10004300
```

Start with #25 (malloc), #28 (printf), #33 (strlen), then Be API constructors.
