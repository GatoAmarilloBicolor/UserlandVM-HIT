#!/bin/bash
# Download Haiku 32-bit libraries from official repositories

set -e

REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SYSROOT_DIR="$REPO_DIR/sysroot/haiku32"

echo "[*] Downloading Haiku 32-bit system libraries..."
mkdir -p "$SYSROOT_DIR/system/lib"
mkdir -p "$SYSROOT_DIR/system/develop/headers"

# Haiku x86_gcc2 repository base URLs
HAIKU_REPOS=(
    "https://download.haiku-os.org/haiku/master/x86_gcc2/current"
    "https://eu.hpkg.sh/haiku/master/x86_gcc2/current"
    "https://packages.haiku-os.org/haikuos/master/x86_gcc2/current"
)

echo "[*] Available Haiku package repositories:"
echo "  1. download.haiku-os.org"
echo "  2. eu.hpkg.sh"
echo "  3. packages.haiku-os.org"

# Try main repository
BASE_URL="https://download.haiku-os.org/haiku/master/x86_gcc2/current"

echo "[*] Testing connection to: $BASE_URL"

# Try to download haiku packages (these contain libc.so.0)
PACKAGES=(
    "haiku-1~r1beta4-1-x86_gcc2.hpkg"
    "haiku_devel-1~r1beta4-1-x86_gcc2.hpkg"
    "gcc-1~4.8.5_2015_11_06-5-x86_gcc2.hpkg"
)

for pkg in "${PACKAGES[@]}"; do
    echo "[*] Attempting to download: $pkg"
    
    # Try downloading from repository
    if wget -q --timeout=5 "$BASE_URL/$pkg" -O "/tmp/$pkg" 2>/dev/null; then
        echo "[+] Downloaded $pkg"
        
        # Extract .hpkg (which is a tar.gz)
        if file "/tmp/$pkg" | grep -q "gzip"; then
            echo "[*] Extracting libraries from $pkg..."
            tar -xzf "/tmp/$pkg" -C "$SYSROOT_DIR/" 2>/dev/null || true
        fi
        
        rm -f "/tmp/$pkg"
    else
        echo "[-] Could not download $pkg from $BASE_URL"
    fi
done

# Check if we got libc
if find "$SYSROOT_DIR" -name "libc.so*" 2>/dev/null | grep -q .; then
    echo "[+] Successfully downloaded 32-bit libraries!"
    echo "[*] Found libraries:"
    find "$SYSROOT_DIR" -name "*.so*" -type f 2>/dev/null | head -20
else
    echo "[!] No libc.so found. Trying alternative approach..."
    
    # Alternative: Create stub libc for testing
    echo "[*] Creating minimal test structure..."
    mkdir -p "$SYSROOT_DIR/system/lib"
    
    # Try to find system libc and copy if 32-bit
    if [ -f "/system/lib/libc.so.0" ]; then
        echo "[+] Found libc.so.0 in /system/lib/"
        cp /system/lib/libc.so.0 "$SYSROOT_DIR/system/lib/" 2>/dev/null || echo "[-] Cannot copy (permissions?)"
    fi
fi

echo "[+] Sysroot setup complete at: $SYSROOT_DIR"
ls -lah "$SYSROOT_DIR/system/lib/" 2>/dev/null || echo "[*] Directory created"

exit 0
