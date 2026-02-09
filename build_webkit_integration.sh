#!/bin/bash
# Build script for UserlandVM WebPositive integration
# Integrates the master haiku32 VM with real Be API window support

set -e

echo "=========================================="
echo "  UserlandVM WebKit Integration Builder"
echo "=========================================="
echo ""

# Configuration
OUTPUT_BINARY="userlandvm_webkit_integrated"
MASTER_SOURCE="userlandvm_haiku32_master.cpp"
BEAPI_SOURCE="BeAPIWrapper.cpp"
COMPILER="g++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra"
LDFLAGS="-lbe -lstdc++"

# Check dependencies
echo "[BUILD] Checking dependencies..."
if ! which g++ > /dev/null; then
    echo "ERROR: g++ not found"
    exit 1
fi

if [ ! -f "$MASTER_SOURCE" ]; then
    echo "ERROR: $MASTER_SOURCE not found"
    exit 1
fi

if [ ! -f "$BEAPI_SOURCE" ]; then
    echo "ERROR: $BEAPI_SOURCE not found"
    exit 1
fi

if [ ! -f "BeAPIWrapper.h" ]; then
    echo "ERROR: BeAPIWrapper.h not found"
    exit 1
fi

echo "✓ All dependencies found"
echo ""

# Step 1: Compile BeAPIWrapper
echo "[BUILD] Compiling Be API Wrapper..."
$COMPILER $CXXFLAGS -c "$BEAPI_SOURCE" -o BeAPIWrapper.o $LDFLAGS
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile BeAPIWrapper.cpp"
    exit 1
fi
echo "✓ BeAPIWrapper compiled"
echo ""

# Step 2: Compile Master Source
echo "[BUILD] Compiling Master VM..."
$COMPILER $CXXFLAGS -c "$MASTER_SOURCE" -o userlandvm_haiku32_master.o
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile $MASTER_SOURCE"
    exit 1
fi
echo "✓ Master VM compiled"
echo ""

# Step 3: Link everything together
echo "[BUILD] Linking final binary..."
$COMPILER $CXXFLAGS \
    userlandvm_haiku32_master.o \
    BeAPIWrapper.o \
    -o "$OUTPUT_BINARY" \
    $LDFLAGS

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to link final binary"
    exit 1
fi

echo "✓ Binary linked successfully"
echo ""

# Verify output
if [ ! -f "$OUTPUT_BINARY" ]; then
    echo "ERROR: Output binary not created"
    exit 1
fi

echo "=========================================="
echo "✓ BUILD SUCCESSFUL"
echo "=========================================="
echo ""
echo "Output binary: $OUTPUT_BINARY"
ls -lh "$OUTPUT_BINARY"
echo ""
echo "To run WebPositive:"
echo "  ./$OUTPUT_BINARY sysroot/haiku32/bin/webpositive"
echo ""
echo "To run with GUI window:"
echo "  ./$OUTPUT_BINARY sysroot/haiku32/bin/webpositive"
echo ""
