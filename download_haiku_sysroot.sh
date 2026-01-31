#!/bin/bash
# download_haiku_sysroot.sh - Download and extract Haiku base packages
#
# Esta versión simplificada descarga el paquete base de Haiku y extrae
# las librerías esenciales al sysroot

set -e

REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SYSROOT_DIR="${REPO_DIR}/sysroot/haiku32"
LIB_DIR="${SYSROOT_DIR}/system/lib"
TEMP_DIR="${REPO_DIR}/.temp_hpkg"

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[i]${NC} $1"; }
log_success() { echo -e "${GREEN}[✓]${NC} $1"; }
log_error() { echo -e "${RED}[✗]${NC} $1"; }

echo -e "${GREEN}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║   Haiku Sysroot Downloader - Simple Version              ║${NC}"
echo -e "${GREEN}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================================================
# CHECK PREREQUISITES
# ============================================================================

log_info "Checking prerequisites..."

if ! command -v curl &>/dev/null && ! command -v wget &>/dev/null; then
    log_error "curl or wget not found"
    exit 1
fi

if ! command -v unzip &>/dev/null; then
    log_error "unzip not found. Install with: pkgman install unzip"
    exit 1
fi

log_success "Prerequisites OK"
echo ""

# ============================================================================
# CREATE DIRECTORIES
# ============================================================================

log_info "Creating sysroot directories..."
mkdir -p "$LIB_DIR"
mkdir -p "$TEMP_DIR"
log_success "Directories created"
echo ""

# ============================================================================
# DOWNLOAD HAIKU BASE PACKAGE
# ============================================================================

# Latest Haiku beta5 version
HAIKU_VERSION="r1~beta5_hrev59189"
PACKAGE_NAME="haiku-${HAIKU_VERSION}-x86-gcc2.hpkg"
DOWNLOAD_URL="https://eu.hpkg.haiku-os.org/haiku/master/x86_gcc2/${HAIKU_VERSION}/${PACKAGE_NAME}"

log_info "Downloading Haiku base package..."
log_info "Version: $HAIKU_VERSION"
log_info "Package: $PACKAGE_NAME"
echo ""

if command -v curl &>/dev/null; then
    if ! curl -L -f -o "$TEMP_DIR/$PACKAGE_NAME" --progress-bar "$DOWNLOAD_URL"; then
        log_error "Download failed"
        exit 1
    fi
elif command -v wget &>/dev/null; then
    if ! wget -q --show-progress -O "$TEMP_DIR/$PACKAGE_NAME" "$DOWNLOAD_URL"; then
        log_error "Download failed"
        exit 1
    fi
fi

log_success "Package downloaded ($(du -h $TEMP_DIR/$PACKAGE_NAME | cut -f1))"
echo ""

# ============================================================================
# EXTRACT LIBRARIES
# ============================================================================

log_info "Extracting libraries from package..."

cd "$TEMP_DIR"

if ! unzip -q "$PACKAGE_NAME" 2>/dev/null; then
    log_error "Failed to extract package"
    exit 1
fi

# Find and copy .so libraries
LIB_COUNT=0
find . -name "*.so*" -type f 2>/dev/null | while read lib_file; do
    cp "$lib_file" "$LIB_DIR/" 2>/dev/null || true
    echo "  $(basename $lib_file)"
    ((LIB_COUNT++))
done

cd "$REPO_DIR"

# Count extracted libraries
EXTRACTED=$(find "$LIB_DIR" -name "*.so*" -type f 2>/dev/null | wc -l)

if [ $EXTRACTED -eq 0 ]; then
    log_error "No libraries extracted"
    exit 1
fi

log_success "Extracted $EXTRACTED libraries"
echo ""

# ============================================================================
# CLEANUP
# ============================================================================

log_info "Cleaning up temporary files..."
rm -rf "$TEMP_DIR"
log_success "Cleanup complete"
echo ""

# ============================================================================
# VERIFY
# ============================================================================

log_info "Verifying sysroot contents..."
echo ""
echo "Libraries in sysroot:"
ls -1h "$LIB_DIR"/ | sed 's/^/  • /'

LIBC_CHECK=$(ls "$LIB_DIR"/libc.so* 2>/dev/null | wc -l)
LIBM_CHECK=$(ls "$LIB_DIR"/libm.so* 2>/dev/null | wc -l)
LIBROOT_CHECK=$(ls "$LIB_DIR"/libroot.so 2>/dev/null | wc -l)

echo ""
echo "Essential libraries:"
[ $LIBC_CHECK -gt 0 ] && echo -e "  ${GREEN}✓${NC} libc" || echo -e "  ${RED}✗${NC} libc"
[ $LIBM_CHECK -gt 0 ] && echo -e "  ${GREEN}✓${NC} libm" || echo -e "  ${RED}✗${NC} libm"  
[ $LIBROOT_CHECK -gt 0 ] && echo -e "  ${GREEN}✓${NC} libroot" || echo -e "  ${RED}✗${NC} libroot"

echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo "Sysroot setup complete!"
echo "Location: $SYSROOT_DIR"
echo "Libraries: $EXTRACTED"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"

if [ $EXTRACTED -gt 5 ]; then
    log_success "Sysroot ready for use"
    exit 0
else
    log_error "Sysroot may be incomplete"
    exit 1
fi
