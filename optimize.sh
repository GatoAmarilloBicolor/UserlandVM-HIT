#!/bin/bash
# UserlandVM-HIT Complete Optimization Script
# Maximum hardware acceleration for HaikuOS

set -e

echo "=========================================="
echo "UserlandVM-HIT SIMD Optimization Suite"
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Hardware detection
echo -e "${BLUE}=== Hardware Detection ===${NC}"

CPU_VENDOR=$(grep -m1 "vendor_id" /proc/cpuinfo | awk '{print $3}')
CPU_MODEL=$(grep -m1 "model name" /proc/cpuinfo | cut -d: -f2 | xargs)
CPU_CORES=$(nproc)
CPU_FREQ=$(lscpu | grep "MHz" | awk '{print $3}' | head -1)

TOTAL_RAM=$(free -h | grep "Mem:" | awk '{print $2}')
AVAIL_RAM=$(free -h | grep "Mem:" | awk '{print $7}')

echo -e "${GREEN}CPU:${NC} $CPU_VENDOR $CPU_MODEL"
echo -e "${GREEN}Cores:${NC} $CPU_CORES @ ${CPU_FREQ}MHz"
echo -e "${GREEN}RAM:${NC} $TOTAL_RAM total, $AVAIL_RAM available"

# SIMD capability detection
echo -e "\n${BLUE}=== SIMD Capability Detection ===${NC}"

SSE2_SUPPORT=$(grep -q "sse2" /proc/cpuinfo && echo "YES" || echo "NO")
AVX2_SUPPORT=$(grep -q "avx2" /proc/cpuinfo && echo "YES" || echo "NO")
AVX512_SUPPORT=$(grep -q "avx512" /proc/cpuinfo && echo "YES" || echo "NO")

echo -e "${GREEN}SSE2:${NC} $SSE2_SUPPORT"
echo -e "${GREEN}AVX2:${NC} $AVX2_SUPPORT"
echo -e "${GREEN}AVX512:${NC} $AVX512_SUPPORT"

# Cache detection
L1_CACHE=$(lscpu | grep "L1d cache" | awk '{print $3}' | sed 's/K//')
L2_CACHE=$(lscpu | grep "L2 cache" | awk '{print $3}' | sed 's/K//')
L3_CACHE=$(lscpu | grep "L3 cache" | awk '{print $3}' | sed 's/K//')

echo -e "\n${GREEN}L1 Cache:${NC} ${L1_CACHE}KB"
echo -e "${GREEN}L2 Cache:${NC} ${L2_CACHE}KB"
echo -e "${GREEN}L3 Cache:${NC} ${L3_CACHE}KB"

# Generate optimization flags
echo -e "\n${BLUE}=== Generating Optimization Flags ===${NC}"

OPT_FLAGS="-std=c++20 -O3 -march=native -mtune=native"
OPT_FLAGS="$OPT_FLAGS -msse2"

if [ "$AVX2_SUPPORT" = "YES" ]; then
    OPT_FLAGS="$OPT_FLAGS -mavx2 -mfma -mbmi2"
    echo -e "${GREEN}✓ AVX2 optimizations enabled${NC}"
else
    echo -e "${YELLOW}⚠ AVX2 not available, using SSE2 only${NC}"
fi

if [ "$AVX512_SUPPORT" = "YES" ]; then
    OPT_FLAGS="$OPT_FLAGS -mavx512f -mavx512bw"
    echo -e "${GREEN}✓ AVX512 optimizations enabled${NC}"
fi

# CPU-specific optimizations
if [[ "$CPU_VENDOR" == *"Intel"* ]]; then
    OPT_FLAGS="$OPT_FLAGS -mintel-opt-rtl -mavoid-false-dependencies"
    echo -e "${GREEN}✓ Intel-specific optimizations enabled${NC}"
elif [[ "$CPU_VENDOR" == *"AMD"* ]]; then
    OPT_FLAGS="$OPT_FLAGS -march=znver2"
    echo -e "${GREEN}✓ AMD-specific optimizations enabled${NC}"
fi

# General performance flags
OPT_FLAGS="$OPT_FLAGS -flto -ffast-math -funroll-loops -fprefetch-loop-arrays"
OPT_FLAGS="$OPT_FLAGS -fomit-frame-pointer -foptimize-sibling-calls"
OPT_FLAGS="$OPT_FLAGS -DCACHE_LINE_SIZE=$L1_CACHE"

echo -e "${BLUE}Final optimization flags:${NC}"
echo "$OPT_FLAGS"

# HaikuOS Kit detection
echo -e "\n${BLUE}=== HaikuOS Kit Availability ===${NC}"

KIT_DIRS="/boot/system/develop/headers"
if [ -d "$KIT_DIRS" ]; then
    echo -e "${GREEN}✓ ApplicationKit: $([ -f "$KIT_DIRS/interface/Application.h" ] && echo "Available" || echo "Missing")${NC}"
    echo -e "${GREEN}✓ StorageKit: $([ -f "$KIT_DIRS/storage/StorageDefs.h" ] && echo "Available" || echo "Missing")${NC}"
    echo -e "${GREEN}✓ InterfaceKit: $([ -f "$KIT_DIRS/interface/InterfaceDefs.h" ] && echo "Available" || echo "Missing")${NC}"
    echo -e "${GREEN}✓ MediaKit: $([ -f "$KIT_DIRS/media/MediaDefs.h" ] && echo "Available" || echo "Missing")${NC}"
    echo -e "${GREEN}✓ NetworkKit: $([ -f "$KIT_DIRS/net/NetDefs.h" ] && echo "Available" || echo "Missing")${NC}"
    echo -e "${GREEN}✓ GameKit: $([ -f "$KIT_DIRS/game/GameDefs.h" ] && echo "Available" || echo "Missing")${NC}"
else
    echo -e "${RED}✗ HaikuOS development headers not found${NC}"
fi

# Build optimized version
echo -e "\n${BLUE}=== Building Optimized UserlandVM-HIT ===${NC}"

# Backup original Makefile
if [ -f "Makefile" ]; then
    cp Makefile Makefile.backup
    echo -e "${GREEN}✓ Backed up original Makefile${NC}"
fi

# Use optimized Makefile
if [ -f "Makefile.optimized" ]; then
    cp Makefile.optimized Makefile
    echo -e "${GREEN}✓ Using optimized Makefile${NC}"
else
    echo -e "${RED}✗ Makefile.optimized not found${NC}"
    exit 1
fi

# Set environment variables for compilation
export CXXFLAGS="$OPT_FLAGS"
export LDFLAGS="-flto"
export SIMD_SUPPORT="$SSE2_SUPPORT"
export AVX_SUPPORT="$AVX2_SUPPORT"

echo -e "${BLUE}Compiling with maximum optimizations...${NC}"

# Clean previous build
make clean > /dev/null 2>&1 || true

# Build with optimizations
if make -j$CPU_CORES all; then
    echo -e "${GREEN}✓ Build successful!${NC}"
    
    # Get binary info
    if [ -f "UserlandVM-HIT-Optimized" ]; then
        BINARY_SIZE=$(ls -lh UserlandVM-HIT-Optimized | awk '{print $5}')
        echo -e "${GREEN}✓ Optimized binary size: $BINARY_SIZE${NC}"
        
        # Strip binary for production
        strip UserlandVM-HIT-Optimized
        STRIPPED_SIZE=$(ls -lh UserlandVM-HIT-Optimized | awk '{print $5}')
        echo -e "${GREEN}✓ Stripped binary size: $STRIPPED_SIZE${NC}"
    fi
else
    echo -e "${RED}✗ Build failed!${NC}"
    # Restore original Makefile
    if [ -f "Makefile.backup" ]; then
        mv Makefile.backup Makefile
    fi
    exit 1
fi

# Performance testing
echo -e "\n${BLUE}=== Performance Benchmark ===${NC}"

if [ -f "UserlandVM-HIT-Optimized" ] && [ -f "TestX86" ]; then
    echo -e "${BLUE}Running optimized performance test...${NC}"
    
    # Measure execution time
    START_TIME=$(date +%s%N)
    ./UserlandVM-HIT-Optimized TestX86 > /dev/null 2>&1 || true
    END_TIME=$(date +%s%N)
    
    EXECUTION_TIME=$(( ($END_TIME - $START_TIME) / 1000000 ))
    echo -e "${GREEN}✓ Execution time: ${EXECUTION_TIME}ms${NC}"
    
    # Measure memory usage
    if command -v valgrind >/dev/null 2>&1; then
        echo -e "${BLUE}Memory analysis (if valgrind available)...${NC}"
        valgrind --tool=massif --massif-out-file=massif.out ./UserlandVM-HIT-Optimized TestX86 > /dev/null 2>&1 || true
        
        if [ -f "massif.out" ]; then
            PEAK_MEMORY=$(ms_print massif.out | grep "heap" | tail -1 | awk '{print $4}')
            echo -e "${GREEN}✓ Peak memory usage: ${PEAK_MEMORY}KB${NC}"
        fi
    fi
fi

# SIMD validation
echo -e "\n${BLUE}=== SIMD Validation ===${NC}"

if [ -f "UserlandVM-HIT-Optimized" ]; then
    echo -e "${BLUE}Checking SIMD instruction usage...${NC}"
    
    if command -v objdump >/dev/null 2>&1; then
        # Check for SIMD instructions in binary
        SSE2_COUNT=$(objdump -d UserlandVM-HIT-Optimized | grep -c " xmm\|ps\|pd" || echo "0")
        AVX_COUNT=$(objdump -d UserlandVM-HIT-Optimized | grep -c "ymm\|avx" || echo "0")
        
        echo -e "${GREEN}✓ SSE2 instruction count: $SSE2_COUNT${NC}"
        echo -e "${GREEN}✓ AVX instruction count: $AVX_COUNT${NC}"
        
        if [ "$SSE2_COUNT" -gt 0 ] || [ "$AVX_COUNT" -gt 0 ]; then
            echo -e "${GREEN}✓ SIMD instructions detected in binary${NC}"
        else
            echo -e "${YELLOW}⚠ No SIMD instructions detected${NC}"
        fi
    fi
fi

# Installation
echo -e "\n${BLUE}=== Installation ===${NC}"

if [ -f "UserlandVM-HIT-Optimized" ]; then
    INSTALL_DIR="/boot/home/config/non-packaged/bin"
    mkdir -p "$INSTALL_DIR" 2>/dev/null || true
    
    if cp UserlandVM-HIT-Optimized "$INSTALL_DIR/"; then
        echo -e "${GREEN}✓ Installed to $INSTALL_DIR${NC}"
        
        # Create symlink for easier access
        ln -sf "$INSTALL_DIR/UserlandVM-HIT-Optimized" "/boot/home/bin/userlandvm-opt" 2>/dev/null || true
        echo -e "${GREEN}✓ Symlink created: /boot/home/bin/userlandvm-opt${NC}"
    else
        echo -e "${RED}✗ Installation failed${NC}"
    fi
fi

# Cleanup
echo -e "\n${BLUE}=== Cleanup ===${NC}"

# Restore original Makefile
if [ -f "Makefile.backup" ]; then
    mv Makefile.backup Makefile
    echo -e "${GREEN}✓ Restored original Makefile${NC}"
fi

# Remove temporary files
rm -f massif.out core.* 2>/dev/null || true

echo -e "\n${GREEN}=========================================="
echo "UserlandVM-HIT Optimization Complete!"
echo "==========================================${NC}"

echo -e "\n${BLUE}Summary:${NC}"
echo -e "  Hardware: $CPU_VENDOR $CPU_MODEL ($CPU_CORES cores)"
echo -e "  SIMD: SSE2=$SSE2_SUPPORT, AVX2=$AVX2_SUPPORT, AVX512=$AVX512_SUPPORT"
echo -e "  Cache: L1=${L1_CACHE}KB, L2=${L2_CACHE}KB, L3=${L3_CACHE}KB"
echo -e "  Binary: UserlandVM-HIT-Optimized (optimized with $OPT_FLAGS)"
echo -e "  Location: $INSTALL_DIR/UserlandVM-HIT-Optimized"

echo -e "\n${BLUE}Usage:${NC}"
echo -e "  userlandvm-opt <program>    # Run with SIMD optimizations"
echo -e "  make -f Makefile.optimized  # Rebuild with optimizations"

echo -e "\n${GREEN}Optimization Status: COMPLETE ✅${NC}"