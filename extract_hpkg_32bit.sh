#!/bin/bash
# Extract Haiku 32-bit packages to sysroot
# Usage: bash extract_hpkg_32bit.sh [--extract-all]
# Haiku .hpkg files are in a custom format, handled by 'package' command

PACKAGES_DIR="/boot/home/src/HaikuSoftware/userlandvm_repo/sysroot/haiku32/system/packages"
SYSROOT="/boot/home/src/HaikuSoftware/userlandvm_repo/sysroot/haiku32"
EXTRACT_ALL="${1:---critical}"

# Essential packages for 32-bit development
CRITICAL_PACKAGES=(
    "gcc_x86_syslibs-13.3.0_2023_08_10-1-x86_gcc2.hpkg"
    "haiku_x86-r1~beta5_hrev59078-1-x86_gcc2.hpkg"
)

# Standard libraries
LIB_PACKAGES=(
    "libiconv_x86-1.17-4-x86_gcc2.hpkg"
    "zlib_x86-1.3.1-3-x86_gcc2.hpkg"
    "ncurses6_x86-6.4_20230520-1-x86_gcc2.hpkg"
)

# All other utility packages
ALL_PACKAGES=()

echo "📦 Haiku 32-bit Package Extractor"
echo "=================================="
echo "Sysroot: $SYSROOT"
echo "Packages dir: $PACKAGES_DIR"
echo ""

# Create necessary directories
mkdir -p "$SYSROOT/system/lib"
mkdir -p "$SYSROOT/system/bin"
mkdir -p "$SYSROOT/system/include"

# Function to extract a package
extract_package() {
    local pkg="$1"
    local pkg_path="$PACKAGES_DIR/$pkg"
    
    if [ ! -f "$pkg_path" ]; then
        return 1
    fi
    
    echo -n "  $pkg ... "
    if package extract -C "$SYSROOT" "$pkg_path" >/dev/null 2>&1; then
        echo "✅"
        return 0
    else
        echo "⚠️"
        return 1
    fi
}

# Collect all packages if --extract-all specified
if [ "$EXTRACT_ALL" = "--extract-all" ]; then
    echo "Mode: Extract ALL packages"
    echo ""
    mapfile -t ALL_PACKAGES < <(ls "$PACKAGES_DIR"/*.hpkg 2>/dev/null | xargs -n1 basename | sort)
else
    echo "Mode: Extract CRITICAL packages only"
    echo ""
fi

echo "=== EXTRACTING PACKAGES ==="
count=0
success=0

if [ "$EXTRACT_ALL" = "--extract-all" ]; then
    for pkg in "${ALL_PACKAGES[@]}"; do
        ((count++))
        extract_package "$pkg" && ((success++))
    done
else
    for pkg in "${CRITICAL_PACKAGES[@]}" "${LIB_PACKAGES[@]}"; do
        ((count++))
        extract_package "$pkg" && ((success++))
    done
fi

echo ""
echo "=== EXTRACTION COMPLETE ==="
echo "Packages attempted: $count"
echo "Packages extracted: $success"
echo ""

# Summary
if [ -d "$SYSROOT/system/lib" ]; then
    lib_count=$(find "$SYSROOT/system/lib" -type f 2>/dev/null | wc -l)
    echo "Files in system/lib: $lib_count"
fi

if [ -d "$SYSROOT/system/bin" ]; then
    bin_count=$(find "$SYSROOT/system/bin" -type f 2>/dev/null | wc -l)
    echo "Files in system/bin: $bin_count"
fi

total_files=$(find "$SYSROOT" -type f 2>/dev/null | wc -l)
echo "Total files in sysroot: $total_files"
echo ""
echo "✅ Done! Use '--extract-all' to extract all packages"
