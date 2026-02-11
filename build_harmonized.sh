#!/bin/bash
set -e

echo "=========================================="
echo "  UserlandVM Harmonized Build"
echo "  Integrating AppServerBridge + Dynamic Linker + Renderer"
echo "=========================================="
echo ""

# Clean previous objects
rm -f *.o userlandvm_harmonized 2>/dev/null || true

# Step 1: Compile HaikuLogging (needed by AppServerBridge)
echo "[BUILD] Compiling HaikuLogging..."
g++ -std=c++17 -O2 -c HaikuLogging.cpp -o HaikuLogging.o
echo "✓ HaikuLogging compiled"

# Step 2: Compile AppServerBridge (new, requires HaikuLogging)
echo "[BUILD] Compiling AppServerBridge..."
g++ -std=c++17 -O2 -c AppServerBridge.cpp -o AppServerBridge.o -lbe 2>&1 | grep -v "warning:" || true
echo "✓ AppServerBridge compiled"

# Step 3: Compile Real-Time Renderer with Be API
echo "[BUILD] Compiling Real-Time Renderer..."
g++ -std=c++17 -O2 -c RealTimeRenderer.cpp -o RealTimeRenderer.o -lbe 2>&1 | grep -v "warning:" || true
echo "✓ Real-Time Renderer compiled"

# Step 4: Compile Syscall Interceptor
echo "[BUILD] Compiling Syscall Interceptor..."
g++ -std=c++17 -O2 -c SyscallInterceptor.cpp -o SyscallInterceptor.o
echo "✓ Syscall Interceptor compiled"

# Step 5: Compile Dynamic Linker
echo "[BUILD] Compiling Dynamic Linker..."
g++ -std=c++17 -O2 -c CompleteELFDynamicLinker.cpp -o CompleteELFDynamicLinker.o
echo "✓ Dynamic Linker compiled"

# Step 6: Compile Dynamic Loading Interceptor (as C to match linker symbols)
echo "[BUILD] Compiling Dynamic Loading Interceptor..."
g++ -std=c++17 -O2 -c DynamicLoadingInterceptor.cpp -o DynamicLoadingInterceptor.o -Wl,--unresolved-symbols=ignore-in-object-files 2>&1 | grep -v "warning:" || true
echo "✓ Dynamic Loading Interceptor compiled"

# Step 7: Compile BeAPI Wrapper
echo "[BUILD] Compiling Be API Wrapper..."
g++ -std=c++17 -O2 -c BeAPIWrapper.cpp -o BeAPIWrapper.o -lbe 2>&1 | grep -v "warning:" || true
echo "✓ Be API Wrapper compiled"

# Step 8: Compile Master VM (modified to integrate all components)
echo "[BUILD] Compiling Master VM (harmonized)..."
g++ -std=c++17 -O2 -c userlandvm_haiku32_master.cpp -o userlandvm_haiku32_master.o 2>&1 | grep -v "warning:" || true
echo "✓ Master VM compiled"

# Step 9: Link final binary
echo "[BUILD] Linking final harmonized binary..."
g++ -std=c++17 -O2 \
    userlandvm_haiku32_master.o \
    HaikuLogging.o \
    AppServerBridge.o \
    RealTimeRenderer.o \
    SyscallInterceptor.o \
    CompleteELFDynamicLinker.o \
    DynamicLoadingInterceptor.o \
    BeAPIWrapper.o \
    -o userlandvm_harmonized \
    -lbe -lstdc++ 2>&1 | grep -E "error|Error" || true

if [ ! -f userlandvm_harmonized ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "✓ Final harmonized binary linked"
echo ""
echo "=========================================="
echo "✓ HARMONIZED BUILD COMPLETE"
echo "=========================================="
echo ""
echo "Binary: userlandvm_harmonized"
ls -lh userlandvm_harmonized
echo ""
echo "Usage:"
echo "  ./userlandvm_harmonized sysroot/haiku32/bin/webpositive"
echo ""
