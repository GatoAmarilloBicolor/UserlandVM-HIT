#!/bin/bash
# download_sysroot.sh - Download Haiku 32-bit system libraries from HaikuDepot
# 
# This script downloads essential 32-bit libraries for the UserlandVM sysroot
# It can be run in the background while the VM is executing

set -e  # Exit on error

SYSROOT_DIR="${1:-.}/sysroot/haiku32"
LIB_CACHE_DIR="${SYSROOT_DIR}/system/lib"
HEADERS_DIR="${SYSROOT_DIR}/system/develop/headers"

# HaikuDepot base URL
HAIKUDEPOT_BASE="https://depot.haiku-os.org/__api/v1/packages"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== UserlandVM Sysroot Download Manager ===${NC}"
echo "Sysroot directory: $SYSROOT_DIR"
echo ""

# Create directories
echo -e "${YELLOW}Creating sysroot directories...${NC}"
mkdir -p "$LIB_CACHE_DIR"
mkdir -p "$HEADERS_DIR"

# Essential libraries to download
declare -a LIBS=(
    "libc.so.0:haiku_x86_32_gcc2:r1beta4"
    "libm.so.0:haiku_x86_32_gcc2:r1beta4"
    "libroot.so:haiku_x86_32_gcc2:r1beta4"
    "libbe.so:haiku_x86_32_gcc2:r1beta4"
    "libappkit.so:haiku_x86_32_gcc2:r1beta4"
)

# Check which libraries are missing
echo -e "${YELLOW}Checking for missing libraries...${NC}"
MISSING=0
for lib_spec in "${LIBS[@]}"; do
    IFS=':' read -r lib_name package version <<< "$lib_spec"
    
    if [ -f "$LIB_CACHE_DIR/$lib_name" ]; then
        echo -e "${GREEN}✓${NC} $lib_name (cached)"
    else
        echo -e "${YELLOW}✗${NC} $lib_name (missing)"
        ((MISSING++))
    fi
done

echo ""

if [ $MISSING -eq 0 ]; then
    echo -e "${GREEN}All libraries are already cached. Nothing to download.${NC}"
    exit 0
fi

echo -e "${YELLOW}$MISSING libraries need to be downloaded${NC}"
echo ""

# Check for network connectivity
if ! ping -c 1 depot.haiku-os.org >/dev/null 2>&1; then
    echo -e "${RED}ERROR: Cannot reach depot.haiku-os.org${NC}"
    echo "Please check your network connection."
    exit 1
fi

echo -e "${GREEN}Network connection OK${NC}"
echo ""

# Download function
download_package() {
    local package_name=$1
    local version=$2
    local lib_name=$3
    local dest_dir=$4
    
    local url="${HAIKUDEPOT_BASE}/${package_name}/versions/${version}/download"
    local dest_file="${dest_dir}/${lib_name}"
    
    if [ -f "$dest_file" ]; then
        echo -e "${GREEN}✓${NC} $lib_name already exists, skipping..."
        return 0
    fi
    
    echo -e "${YELLOW}Downloading $lib_name...${NC}"
    echo "  From: $url"
    echo "  To: $dest_file"
    
    # Use curl with progress bar
    if command -v curl &> /dev/null; then
        if curl -L -f -o "$dest_file" --progress-bar "$url" 2>/dev/null; then
            echo -e "${GREEN}✓${NC} Downloaded $lib_name"
            return 0
        else
            echo -e "${RED}✗${NC} Failed to download $lib_name"
            # Don't exit, try next
            return 1
        fi
    else
        echo -e "${RED}ERROR: curl not found${NC}"
        echo "Please install curl: pkgman install cmd:curl"
        return 1
    fi
}

# Main download loop
SUCCESS=0
FAILED=0

for lib_spec in "${LIBS[@]}"; do
    IFS=':' read -r lib_name package version <<< "$lib_spec"
    
    if download_package "$package" "$version" "$lib_name" "$LIB_CACHE_DIR"; then
        ((SUCCESS++))
    else
        ((FAILED++))
    fi
done

echo ""
echo "=== Download Summary ==="
echo -e "${GREEN}Successful: $SUCCESS${NC}"
echo -e "${RED}Failed: $FAILED${NC}"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All libraries downloaded successfully!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some downloads failed${NC}"
    exit 1
fi
