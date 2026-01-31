#!/bin/bash
# download_haiku32_packages.sh - Simple direct download of Haiku 32-bit packages
#
# This script downloads haiku_x86_gcc2 packages directly from the EU mirror
# The repository structure redirects /current to the actual version directory
#
# Usage:
#   bash download_haiku32_packages.sh [sysroot_dir]

set -e

SYSROOT_DIR="${1:-.}/sysroot/haiku32"
REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[✓]${NC} $1"; }
log_error() { echo -e "${RED}[✗]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[!]${NC} $1"; }

echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  Haiku 32-bit Package Downloader                           ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================================================
# STEP 1: DETERMINE ACTUAL REPOSITORY URL
# ============================================================================

log_info "Step 1: Discovering actual repository URL..."

# The /current link redirects to the actual version
# First, fetch the redirect target
REDIRECT_URL=$(curl -s -L "https://eu.hpkg.haiku-os.org/haiku/master/x86_gcc2/current/" | \
    grep -o '"path":"[^"]*"' | sed 's/"path":"\|"//g' | head -1)

if [ -n "$REDIRECT_URL" ]; then
    # REDIRECT_URL contains something like: master/x86_gcc2/r1~beta5_hrev59189
    BASE_REPO="https://eu.hpkg.haiku-os.org/haiku/${REDIRECT_URL}"
    log_success "Found actual repository: $BASE_REPO"
else
    # Fallback to known stable version
    log_warn "Could not determine redirect, using fallback repository"
    BASE_REPO="https://eu.hpkg.haiku-os.org/haiku/master/x86_gcc2/r1~beta5_hrev59189"
fi

PKG_REPO="${BASE_REPO}/packages"
log_info "Package repository: $PKG_REPO"
echo ""

# ============================================================================
# STEP 2: CREATE DIRECTORIES
# ============================================================================

log_info "Step 2: Creating sysroot structure..."
mkdir -p "$SYSROOT_DIR/system/lib"
mkdir -p "$SYSROOT_DIR/system/develop/headers"
mkdir -p "${REPO_DIR}/.temp_hpkg"
TEMP_DIR="${REPO_DIR}/.temp_hpkg"

log_success "Directories ready"
echo ""

# ============================================================================
# STEP 3: QUERY REPOSITORY FOR AVAILABLE PACKAGES
# ============================================================================

log_info "Step 3: Querying available packages..."

# Try to list packages - the repository might support directory listing
PACKAGES=$(curl -s "${PKG_REPO}/" 2>/dev/null | grep -o '[a-zA-Z_]*-[0-9~._a-zA-Z]*-x86_gcc2[^<]*\.hpkg' | sort -u | head -20)

if [ -z "$PACKAGES" ]; then
    log_warn "Could not get directory listing, using known package names..."
    # Use standard Haiku package names
    PACKAGES="haiku-r1~beta5_hrev59189-x86_gcc2.hpkg
haiku_devel-r1~beta5_hrev59189-x86_gcc2.hpkg"
fi

log_success "Found packages:"
echo "$PACKAGES" | sed 's/^/  • /'
echo ""

# ============================================================================
# STEP 4: DOWNLOAD PACKAGES
# ============================================================================

log_info "Step 4: Downloading packages..."
echo ""

SUCCESS=0
FAILED=0

echo "$PACKAGES" | while read pkg; do
    [ -z "$pkg" ] && continue
    
    PKG_URL="${PKG_REPO}/${pkg}"
    PKG_FILE="${TEMP_DIR}/${pkg}"
    
    # Skip if already cached
    if [ -f "$PKG_FILE" ]; then
        log_success "Already cached: $pkg"
        continue
    fi
    
    log_info "Downloading: $pkg"
    if curl -f -L -o "$PKG_FILE" --progress-bar "$PKG_URL" 2>/dev/null; then
        log_success "Downloaded: $pkg"
        ((SUCCESS++))
    else
        log_error "Failed to download: $pkg"
        ((FAILED++))
        rm -f "$PKG_FILE"
    fi
done

echo ""

# ============================================================================
# STEP 5: EXTRACT LIBRARIES
# ============================================================================

log_info "Step 5: Extracting libraries from packages..."
echo ""

EXTRACTED=0

for hpkg_file in "${TEMP_DIR}"/*.hpkg; do
    if [ ! -f "$hpkg_file" ]; then
        continue
    fi
    
    pkg_name=$(basename "$hpkg_file")
    log_info "Processing: $pkg_name"
    
    # Create temp extraction directory
    EXTRACT_DIR="${TEMP_DIR}/extract_$$"
    mkdir -p "$EXTRACT_DIR"
    
    # Extract with unzip (HPKG is ZIP-compatible)
    if unzip -q "$hpkg_file" -d "$EXTRACT_DIR" 2>/dev/null; then
        # Find and copy all .so* files
        find "$EXTRACT_DIR" -name "*.so*" -type f 2>/dev/null | while read lib; do
            cp "$lib" "$SYSROOT_DIR/system/lib/" 2>/dev/null
            log_success "Extracted: $(basename "$lib")"
            ((EXTRACTED++))
        done
    else
        log_error "Failed to extract: $pkg_name"
    fi
    
    rm -rf "$EXTRACT_DIR"
done

echo ""

# ============================================================================
# STEP 6: VERIFY RESULTS
# ============================================================================

log_info "Step 6: Verifying sysroot..."
echo ""

LIB_COUNT=$(find "$SYSROOT_DIR/system/lib" -name "*.so*" -type f 2>/dev/null | wc -l)

log_success "Total libraries in sysroot: $LIB_COUNT"

if [ $LIB_COUNT -gt 0 ]; then
    log_success "Libraries found:"
    ls -1 "$SYSROOT_DIR/system/lib" | sed 's/^/  • /'
fi

echo ""

# ============================================================================
# CLEANUP
# ============================================================================

log_info "Cleaning up..."
rm -rf "${TEMP_DIR}"
log_success "Done"

echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Download Summary:${NC}"
echo "  Sysroot:       $SYSROOT_DIR"
echo "  Libraries:     $LIB_COUNT"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo ""

if [ $LIB_COUNT -gt 0 ]; then
    log_success "✓ Haiku 32-bit sysroot ready!"
    exit 0
else
    log_error "✗ No libraries found - download may have failed"
    exit 1
fi
