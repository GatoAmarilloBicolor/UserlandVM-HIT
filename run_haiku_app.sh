#!/bin/bash
# Run Haiku 32-bit applications in the emulator

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
VM_BIN="$SCRIPT_DIR/builddir/UserlandVM"
SYSROOT="$SCRIPT_DIR/sysroot/haiku32"
ENGINE="${ENGINE:-librvvm.so}"

# Ensure librvvm2.so symlink exists
if [ ! -f "$SCRIPT_DIR/builddir/librvvm2.so" ]; then
    ln -s "$SCRIPT_DIR/builddir/$ENGINE" "$SCRIPT_DIR/builddir/librvvm2.so"
fi

# Check if binary exists
if [ $# -eq 0 ]; then
    echo "Usage: $0 [BINARY] [ARGS...]"
    echo "Example: $0 sysroot/haiku32/bin/bash -c 'echo hello'"
    exit 1
fi

BINARY="$1"
shift

# Make binary path absolute if it's in sysroot
if [[ "$BINARY" != /* ]]; then
    BINARY="$SCRIPT_DIR/$BINARY"
fi

if [ ! -f "$BINARY" ]; then
    echo "Error: Binary not found: $BINARY"
    exit 1
fi

echo "[*] Running: $BINARY $@"
echo "[*] Engine: $ENGINE"
echo "[*] Sysroot: $SYSROOT"
echo ""

# Run in emulator
"$VM_BIN" "$BINARY" "$@"
