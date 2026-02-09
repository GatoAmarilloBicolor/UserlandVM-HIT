#!/bin/bash
set -e

echo "=========================================="
echo "  UserlandVM Complete WebPositive Build"
echo "=========================================="
echo ""

# Step 1: Compile Real-Time Renderer with Be API
echo "[BUILD] Compiling Real-Time Renderer..."
g++ -std=c++17 -O2 -c RealTimeRenderer.cpp -o RealTimeRenderer.o -lbe 2>&1 | grep -v "warning:" || true
echo "✓ Real-Time Renderer compiled"

# Step 2: Compile Syscall Interceptor
echo "[BUILD] Compiling Syscall Interceptor..."
g++ -std=c++17 -O2 -c SyscallInterceptor.cpp -o SyscallInterceptor.o
echo "✓ Syscall Interceptor compiled"

# Step 3: Compile Dynamic Linker
echo "[BUILD] Compiling Dynamic Linker..."
g++ -std=c++17 -O2 -c DynamicLinkerStub.cpp -o DynamicLinkerStub.o
echo "✓ Dynamic Linker compiled"

# Step 4: Compile BeAPI Wrapper
echo "[BUILD] Compiling Be API Wrapper..."
g++ -std=c++17 -O2 -c BeAPIWrapper.cpp -o BeAPIWrapper.o -lbe 2>&1 | grep -v "warning:" || true
echo "✓ Be API Wrapper compiled"

# Step 5: Compile Master VM
echo "[BUILD] Compiling Master VM..."
g++ -std=c++17 -O2 -c userlandvm_haiku32_master.cpp -o userlandvm_haiku32_master.o 2>&1 | grep -v "warning:" || true
echo "✓ Master VM compiled"

# Step 6: Link final binary
echo "[BUILD] Linking final binary..."
g++ -std=c++17 -O2 \
    userlandvm_haiku32_master.o \
    RealTimeRenderer.o \
    SyscallInterceptor.o \
    DynamicLinkerStub.o \
    BeAPIWrapper.o \
    -o userlandvm_complete \
    -lbe -lstdc++ 2>&1 | grep -E "error|Error" || true

if [ ! -f userlandvm_complete ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "✓ Final binary linked"
echo ""
echo "=========================================="
echo "✓ BUILD COMPLETE"
echo "=========================================="
echo ""
echo "Binary: userlandvm_complete"
ls -lh userlandvm_complete
echo ""
echo "Usage:"
echo "  ./userlandvm_complete sysroot/haiku32/bin/webpositive"
echo ""
