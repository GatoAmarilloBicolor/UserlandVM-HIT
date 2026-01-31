#!/bin/bash

# Extract essential HPKG packages to sysroot
# Extracts binaries and libraries from key packages

SYSROOT="./sysroot/haiku32"
TEMP_DIR="$SYSROOT/.hpkg_extract_temp"
ESSENTIAL_PACKAGES=(
    "bash"
    "coreutils_x86"
    "diffutils_x86"
    "findutils_x86"
    "grep_x86"
    "sed"
    "gzip"
    "tar_x86"
    "unzip"
    "zip"
    "git_x86"
    "curl_x86"
    "wget_x86"
    "which"
    "less"
    "nano_x86"
    "perl_x86"
    "mawk"
)

echo "═════════════════════════════════════════════════════════"
echo "  HPKG Essential Package Extractor"
echo "═════════════════════════════════════════════════════════"
echo ""

if [ ! -d "$SYSROOT" ]; then
    echo "Error: Sysroot not found at $SYSROOT"
    exit 1
fi

mkdir -p "$TEMP_DIR"

echo "Extracting essential packages..."
echo ""

extracted_count=0
for pkg_name in "${ESSENTIAL_PACKAGES[@]}"; do
    # Find the matching hpkg file (handling multiple versions)
    pkg_file=$(ls "$SYSROOT/system/packages/${pkg_name}"*.hpkg 2>/dev/null | head -1)
    
    if [ -z "$pkg_file" ]; then
        echo "[SKIP] $pkg_name - not found"
        continue
    fi
    
    pkg_basename=$(basename "$pkg_file")
    echo -n "[EXTRACT] $pkg_basename ... "
    
    extract_dir="$TEMP_DIR/$pkg_name"
    mkdir -p "$extract_dir"
    
    # Extract the HPKG
    if /boot/system/bin/package extract "$pkg_file" -C "$extract_dir" 2>/dev/null; then
        # Copy binaries
        if [ -d "$extract_dir/bin" ]; then
            cp -r "$extract_dir/bin"/* "$SYSROOT/bin/" 2>/dev/null && echo "✓ bin"
        elif [ -d "$extract_dir/apps" ]; then
            cp -r "$extract_dir/apps"/* "$SYSROOT/bin/" 2>/dev/null && echo "✓ apps"
        else
            echo "no bin/apps"
        fi
        
        # Copy libraries
        if [ -d "$extract_dir/lib" ]; then
            mkdir -p "$SYSROOT/lib"
            cp -r "$extract_dir/lib"/* "$SYSROOT/lib/" 2>/dev/null
        fi
        
        # Copy data if present
        if [ -d "$extract_dir/data" ]; then
            cp -r "$extract_dir/data"/* "$SYSROOT/data/" 2>/dev/null
        fi
        
        ((extracted_count++))
    else
        echo "✗ failed"
    fi
done

echo ""
echo "═════════════════════════════════════════════════════════"
echo "  Extracted $extracted_count packages"
echo "═════════════════════════════════════════════════════════"
echo ""

# List what we have now
echo "Available binaries in sysroot/haiku32/bin:"
echo ""
ls -1 "$SYSROOT/bin" | head -50

echo ""
echo "Total binaries: $(ls -1 "$SYSROOT/bin" | wc -l)"
echo ""

# Cleanup
echo "Cleaning up temporary files..."
rm -rf "$TEMP_DIR"

echo "✓ Done"
