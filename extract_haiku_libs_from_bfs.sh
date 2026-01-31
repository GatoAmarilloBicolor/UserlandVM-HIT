#!/bin/bash
# extract_haiku_libs_from_bfs.sh - Extract libraries from existing Haiku BFS image
#
# Si ya tienes un archivo haiku_full.bfs, este script extrae las librerías
# esenciales sin necesidad de descargar nada nuevo

set -e

REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BFS_FILE="${REPO_DIR}/sysroot/haiku32/haiku_full.bfs"
SYSROOT_DIR="${REPO_DIR}/sysroot/haiku32"
LIB_DIR="${SYSROOT_DIR}/system/lib"
MOUNT_POINT="/mnt/haiku_extract"

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
echo -e "${GREEN}║  Extract Haiku Libraries from BFS Image                 ║${NC}"
echo -e "${GREEN}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================================================
# CHECK PREREQUISITES
# ============================================================================

if [ ! -f "$BFS_FILE" ]; then
    log_error "BFS file not found: $BFS_FILE"
    exit 1
fi

log_info "Found BFS image: $(du -h $BFS_FILE | cut -f1)"
echo ""

# ============================================================================
# METHOD 1: Try direct extraction with unzip (if HPKG)
# ============================================================================

log_info "Checking if BFS file is an HPKG package..."

if file "$BFS_FILE" | grep -q "Zip\|zip"; then
    log_success "File is a ZIP/HPKG package"
    log_info "Extracting libraries..."
    
    mkdir -p "$LIB_DIR"
    cd "$REPO_DIR"
    unzip -j "$BFS_FILE" "system/lib/*.so*" -d "$LIB_DIR" 2>/dev/null || true
    
    EXTRACTED=$(find "$LIB_DIR" -name "*.so*" -type f 2>/dev/null | wc -l)
    
    if [ $EXTRACTED -gt 0 ]; then
        log_success "Extracted $EXTRACTED libraries from HPKG"
        
        echo ""
        echo "Libraries in sysroot:"
        ls -1 "$LIB_DIR" | sed 's/^/  • /' | head -20
        
        exit 0
    fi
fi

# ============================================================================
# METHOD 2: Try mounting with libfuse2-haiku (if available)
# ============================================================================

log_info "Checking for BFS mount tools..."

if command -v mount.bfs &>/dev/null || command -v fusefs &>/dev/null; then
    log_info "Attempting to mount BFS image..."
    
    sudo mkdir -p "$MOUNT_POINT"
    
    if sudo mount.bfs -o ro "$BFS_FILE" "$MOUNT_POINT" 2>/dev/null; then
        log_success "BFS mounted at $MOUNT_POINT"
        log_info "Copying libraries..."
        
        mkdir -p "$LIB_DIR"
        cp "$MOUNT_POINT/system/lib"/*.so* "$LIB_DIR/" 2>/dev/null || true
        cp "$MOUNT_POINT/boot/system/lib"/*.so* "$LIB_DIR/" 2>/dev/null || true
        
        sudo umount "$MOUNT_POINT"
        
        EXTRACTED=$(find "$LIB_DIR" -name "*.so*" -type f 2>/dev/null | wc -l)
        
        if [ $EXTRACTED -gt 0 ]; then
            log_success "Extracted $EXTRACTED libraries"
            exit 0
        fi
    fi
fi

# ============================================================================
# METHOD 3: Use strings/binwalk to search for library data
# ============================================================================

log_info "Using binary analysis to extract libraries..."

if command -v binwalk &>/dev/null; then
    log_info "Binwalk found, analyzing file..."
    
    # This won't directly extract but will show structure
    binwalk "$BFS_FILE" 2>/dev/null | head -20
fi

# ============================================================================
# METHOD 4: Manual fallback - use existing sysroot
# ============================================================================

log_warning "Could not auto-extract from BFS image"
echo ""
log_info "Alternative approach:"
echo "  1. Download pre-built sysroot packages directly"
echo "  2. Or extract using Haiku host system"
echo "  3. Or use pre-extracted binaries"
echo ""

# Check if sysroot already has some content
EXISTING=$(find "$LIB_DIR" -name "*.so*" -type f 2>/dev/null | wc -l)

if [ $EXISTING -gt 0 ]; then
    log_success "Found $EXISTING existing libraries in sysroot"
    ls -1 "$LIB_DIR" | sed 's/^/  • /' | head -20
    exit 0
fi

log_error "Unable to extract libraries from BFS"
log_info "Please run on a Haiku system or provide extracted libraries"
exit 1
