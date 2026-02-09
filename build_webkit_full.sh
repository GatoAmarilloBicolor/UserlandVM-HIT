#!/bin/bash
set -e

echo "=========================================="
echo "  UserlandVM Full WebPositive Build"
echo "=========================================="
echo ""

# Compile GUI Interceptor with Be API
echo "[BUILD] Compiling GUI Interceptor..."
g++ -std=c++17 -O2 -c GUIInterceptor.cpp -o GUIInterceptor.o -lbe
echo "✓ GUI Interceptor compiled"

# Compile Dynamic Linker Stub
echo "[BUILD] Compiling Dynamic Linker..."
g++ -std=c++17 -O2 -c DynamicLinkerStub.cpp -o DynamicLinkerStub.o
echo "✓ Dynamic Linker compiled"

# Compile BeAPI Wrapper
echo "[BUILD] Compiling Be API Wrapper..."
g++ -std=c++17 -O2 -c BeAPIWrapper.cpp -o BeAPIWrapper.o -lbe
echo "✓ Be API Wrapper compiled"

# Compile Master VM
echo "[BUILD] Compiling Master VM..."
g++ -std=c++17 -O2 -c userlandvm_haiku32_master.cpp -o userlandvm_haiku32_master.o
echo "✓ Master VM compiled"

# Link everything
echo "[BUILD] Linking final binary..."
g++ -std=c++17 -O2 \
    userlandvm_haiku32_master.o \
    GUIInterceptor.o \
    DynamicLinkerStub.o \
    BeAPIWrapper.o \
    -o userlandvm_full \
    -lbe -lstdc++

if [ ! -f userlandvm_full ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "✓ Final binary linked"
echo ""
echo "=========================================="
echo "✓ BUILD COMPLETE"
echo "=========================================="
echo "Binary: userlandvm_full"
ls -lh userlandvm_full
echo ""
