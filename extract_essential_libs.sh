#!/bin/bash
# Extract essential libraries and headers for 32-bit dynamic linking
# Focuses on libc, libm, and core system libraries

PACKAGES_DIR="/boot/home/src/HaikuSoftware/userlandvm_repo/sysroot/haiku32/system/packages"
SYSROOT="/boot/home/src/HaikuSoftware/userlandvm_repo/sysroot/haiku32"

# Most important packages - minimal but complete
ESSENTIAL_PACKAGES=(
    # Compiler runtime libraries
    "gcc_x86_syslibs-13.3.0_2023_08_10-1-x86_gcc2.hpkg"
    
    # Core Haiku system (includes libc, libm, headers)
    "haiku_x86-r1~beta5_hrev59078-1-x86_gcc2.hpkg"
    
    # Standard libraries
    "zlib_x86-1.3.1-3-x86_gcc2.hpkg"
    "libiconv_x86-1.17-4-x86_gcc2.hpkg"
)

echo "📚 Essential Libraries Extractor for 32-bit"
echo "==========================================="
echo ""

mkdir -p "$SYSROOT/system/lib"
mkdir -p "$SYSROOT/system/include"

extract_package() {
    local pkg="$1"
    local pkg_path="$PACKAGES_DIR/$pkg"
    
    if [ ! -f "$pkg_path" ]; then
        echo "  ⚠️  Not found: $pkg"
        return 1
    fi
    
    echo -n "  $pkg ... "
    if package extract -C "$SYSROOT" "$pkg_path" >/dev/null 2>&1; then
        echo "✅"
        return 0
    else
        echo "❌"
        return 1
    fi
}

echo "Extracting essential packages:"
count=0
success=0

for pkg in "${ESSENTIAL_PACKAGES[@]}"; do
    ((count++))
    extract_package "$pkg" && ((success++))
done

echo ""
echo "Results: $success/$count packages"
echo ""

# Show what we got
echo "Libraries in sysroot:"
find "$SYSROOT/system/lib" -type f -name "*.so*" -o -name "*.a" 2>/dev/null | sort

echo ""
echo "Headers available:"
if [ -d "$SYSROOT/system/include" ]; then
    find "$SYSROOT/system/include" -maxdepth 1 -type d | wc -l
    echo "include directories found"
else
    echo "(headers directory not created)"
fi

echo ""
echo "✅ Essential libraries extracted to: $SYSROOT"
