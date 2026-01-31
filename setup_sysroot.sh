#!/bin/bash
# Setup sysroot with Haiku 32-bit libraries
# This script copies necessary 32-bit libraries from the host system

set -e

REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SYSROOT_DIR="$REPO_DIR/sysroot/haiku32"

echo "[*] Setting up Haiku 32-bit sysroot..."
echo "[*] Target: $SYSROOT_DIR"

# Create directory structure
mkdir -p "$SYSROOT_DIR/system/lib"
mkdir -p "$SYSROOT_DIR/system/develop/headers"
mkdir -p "$SYSROOT_DIR/boot/system/lib"

echo "[*] Checking for 32-bit libc..."

# Try to find 32-bit libc on the host system
LIBC_PATHS=(
    "/system/lib/libc.so.0"
    "/boot/system/lib/libc.so.0"
    "/lib/libc.so.6"
    "/lib32/libc.so.6"
)

FOUND_LIBC=0
for path in "${LIBC_PATHS[@]}"; do
    if [ -f "$path" ]; then
        # Check if it's 32-bit
        if file "$path" | grep -q "32-bit"; then
            echo "[+] Found 32-bit libc at: $path"
            cp "$path" "$SYSROOT_DIR/system/lib/"
            FOUND_LIBC=1
            break
        fi
    fi
done

if [ $FOUND_LIBC -eq 0 ]; then
    echo "[!] Warning: Could not find 32-bit libc"
    echo "[*] This is expected on non-Haiku systems"
    echo "[*] You may need to manually extract libc from Haiku ISO"
    
    # Create a minimal fallback
    echo "[*] Creating minimal sysroot structure..."
    touch "$SYSROOT_DIR/system/lib/.placeholder"
fi

# Copy headers if available
if [ -d "/boot/system/develop/headers" ]; then
    echo "[*] Copying development headers..."
    cp -r /boot/system/develop/headers/* "$SYSROOT_DIR/system/develop/headers/" 2>/dev/null || true
fi

echo "[+] Sysroot setup complete!"
echo "[*] Location: $SYSROOT_DIR"
ls -la "$SYSROOT_DIR/system/lib/" 2>/dev/null || echo "[*] lib/ directory created but empty"

exit 0
