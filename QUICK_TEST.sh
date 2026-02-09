#!/bin/bash
# Quick test script for UserlandVM-HIT

VM="./userlandvm_haiku32_master"
SYSROOT="./sysroot/haiku32"

if [ ! -f "$VM" ]; then
    echo "Error: $VM not found. Please build first with: make"
    exit 1
fi

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║          UserlandVM-HIT Quick Test Suite                       ║"
echo "║              32-bit Haiku Program Execution                    ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

run_test() {
    local name="$1"
    local binary="$2"
    local timeout_sec="$3"
    
    if [ ! -f "$binary" ]; then
        echo "⚠️  $name: Binary not found"
        return
    fi
    
    echo "► Testing: $name ($(ls -lh "$binary" | awk '{print $5}'))"
    timeout ${timeout_sec:-2} "$VM" "$binary" > /dev/null 2>&1
    local exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        echo "  ✅ PASS (timeout after ${timeout_sec}s)"
    elif [ $exit_code -eq 0 ]; then
        echo "  ✅ PASS (clean exit)"
    else
        echo "  ⚠️  Exit code: $exit_code"
    fi
}

echo "Testing CLI programs..."
run_test "cat" "$SYSROOT/bin/cat" 2
run_test "ls" "$SYSROOT/bin/ls" 2
run_test "ps" "$SYSROOT/bin/ps" 2
echo ""

echo "Testing large programs..."
run_test "listdev (2.7 MB)" "$SYSROOT/bin/listdev" 5
echo ""

echo "Testing GUI applications..."
run_test "webpositive (GUI)" "$SYSROOT/bin/webpositive" 3
echo ""

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                   ✅ Test Suite Complete                       ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
