#!/bin/bash
# setup_sysroot_from_hpkg_v2.sh - Download Haiku packages from EU repository
# 
# This script downloads Haiku system packages from the official EU repository
# and extracts essential libraries needed for UserlandVM's sysroot.
#
# The repository structure:
# eu.hpkg.haiku-os.org/haiku/master/x86_gcc2/current -> redirects to actual path
# EU mirror: https://eu.hpkg.haiku-os.org/haikuports/
# CDN: https://cdn.haiku-os.org/haiku-repositories/

set -e

# ============================================================================
# CONFIGURATION
# ============================================================================

SYSROOT_DIR="${1:-.}/sysroot/haiku32"
INCLUDE_PORTS="${2:-}"
REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Haiku R1 Beta 5 base repositories
# These will automatically redirect to the correct mirror
HAIKU_REPO_BASE="https://eu.hpkg.haiku-os.org/haiku/r1beta5"
HAIKUPORTS_REPO_BASE="https://eu.hpkg.haiku-os.org/haikuports/r1beta5"

# Alternative CDN mirror (may be faster)
CDN_HAIKU_REPO="https://cdn.haiku-os.org/haiku-repositories/r1beta5"

# Temporary directory
TEMP_DIR="${REPO_DIR}/.hpkg_temp"
PKG_DIR="${TEMP_DIR}/packages"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# ============================================================================
# FUNCTIONS
# ============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

log_error() {
    echo -e "${RED}[✗]${NC} $1"
}

command_exists() {
    command -v "$1" &> /dev/null
    return $?
}

# Discover actual package repository URL after redirect
discover_repo_url() {
    local base_url=$1
    local arch=$2
    
    log_info "Discovering actual repository URL..."
    
    # Try to follow redirects and find the final URL
    local final_url=$(curl -s -L -I "${base_url}/${arch}/current/packages/" 2>/dev/null | grep -i "^location:" | tail -1 | awk '{print $2}' | tr -d '\r')
    
    if [ -z "$final_url" ]; then
        # Fallback: try direct access
        log_warning "Could not discover redirect, using direct URL"
        echo "${base_url}/r1beta5/${arch}/current/packages"
    else
        echo "$final_url"
    fi
}

# List packages in repository (using directory listing)
list_repo_packages() {
    local repo_url=$1
    
    log_info "Fetching package list from repository..."
    
    # Try to get directory listing - repositories may support directory listing
    curl -s "$repo_url/" 2>/dev/null | grep -o 'haiku[^"]*\.hpkg' | head -20
}

# Download a single package file
download_package() {
    local pkg_name=$1
    local repo_url=$2
    local arch=$3
    
    # Try different naming patterns for the package
    local urls=(
        "${repo_url}/${pkg_name}-${arch}.hpkg"
        "${repo_url}/${pkg_name}_${arch}.hpkg"
        "${repo_url}/${pkg_name}.hpkg"
    )
    
    local dest_file="${PKG_DIR}/${pkg_name}.hpkg"
    
    if [ -f "$dest_file" ]; then
        log_success "Already cached: $pkg_name"
        return 0
    fi
    
    log_info "Attempting to download: $pkg_name"
    
    for url in "${urls[@]}"; do
        if command_exists curl; then
            if curl -f -L -o "$dest_file" --progress-bar "$url" 2>/dev/null; then
                log_success "Downloaded: $pkg_name from $url"
                return 0
            fi
        elif command_exists wget; then
            if wget -q --show-progress -O "$dest_file" "$url" 2>/dev/null; then
                log_success "Downloaded: $pkg_name from $url"
                return 0
            fi
        fi
    done
    
    log_warning "Could not download: $pkg_name"
    return 1
}

# Extract libraries from HPKG file
extract_libraries() {
    local hpkg_file=$1
    local dest_dir=$2
    
    if [ ! -f "$hpkg_file" ]; then
        return 1
    fi
    
    # HPKG files are ZIP-compatible archives
    if command_exists unzip; then
        local temp_extract="${TEMP_DIR}/extract_$$_$RANDOM"
        mkdir -p "$temp_extract"
        
        if unzip -q "$hpkg_file" -d "$temp_extract" 2>/dev/null; then
            # Extract .so and .so.* files
            find "$temp_extract" -name "*.so*" -type f 2>/dev/null | while read lib_file; do
                cp "$lib_file" "$dest_dir/" 2>/dev/null && log_success "Extracted: $(basename "$lib_file")"
            done
            
            rm -rf "$temp_extract"
            return 0
        fi
    elif command_exists 7z; then
        local temp_extract="${TEMP_DIR}/extract_$$_$RANDOM"
        mkdir -p "$temp_extract"
        
        if 7z x -o"$temp_extract" "$hpkg_file" >/dev/null 2>&1; then
            find "$temp_extract" -name "*.so*" -type f 2>/dev/null | while read lib_file; do
                cp "$lib_file" "$dest_dir/" 2>/dev/null && log_success "Extracted: $(basename "$lib_file")"
            done
            
            rm -rf "$temp_extract"
            return 0
        fi
    fi
    
    log_warning "Could not extract $hpkg_file (unzip/7z not available)"
    return 1
}

# ============================================================================
# MAIN
# ============================================================================

echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  UserlandVM Sysroot Setup v2 - Haiku Package Manager       ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

log_info "Repository Base: $HAIKU_REPO_BASE"
log_info "Sysroot Directory: $SYSROOT_DIR"
echo ""

# ============================================================================
# VERIFY PREREQUISITES
# ============================================================================

log_info "Checking prerequisites..."

if ! command_exists curl && ! command_exists wget; then
    log_error "Neither curl nor wget is installed"
    exit 1
fi

log_success "Download tools available"

if ! command_exists unzip && ! command_exists 7z; then
    log_error "Neither unzip nor 7z found - cannot extract packages"
    exit 1
fi

log_success "Extraction tools available"

# Check network connectivity
log_info "Checking network connectivity..."
if ! ping -c 1 eu.hpkg.haiku-os.org >/dev/null 2>&1; then
    log_error "Cannot reach eu.hpkg.haiku-os.org"
    exit 1
fi

log_success "Network OK"
echo ""

# ============================================================================
# CREATE DIRECTORIES
# ============================================================================

log_info "Creating directory structure..."

mkdir -p "$SYSROOT_DIR/system/lib"
mkdir -p "$SYSROOT_DIR/system/develop/headers"
mkdir -p "$SYSROOT_DIR/boot/system/lib"
mkdir -p "$PKG_DIR"

log_success "Directories created"
echo ""

# ============================================================================
# DOWNLOAD PACKAGES
# ============================================================================

log_info "Phase 1: Discovering repository structure..."
echo ""

# For x86_gcc2 (32-bit hybrid)
ARCH="x86_gcc2h"

# Common Haiku package names
# Note: Exact names depend on the repository structure
declare -a PACKAGES=(
    "haiku"
    "haiku_devel"
)

SUCCESS=0
FAILED=0

# Try to download packages from repository
for pkg in "${PACKAGES[@]}"; do
    # Format: haiku-r1beta5-x86_gcc2h.hpkg
    pkg_file="${pkg}-r1beta5-${ARCH}"
    
    if download_package "$pkg_file" "$HAIKU_REPO_BASE/packages" "$ARCH"; then
        ((SUCCESS++))
    else
        ((FAILED++))
    fi
done

echo ""
log_info "Phase 2: Extracting libraries from packages..."
echo ""

# Extract from all downloaded packages
for hpkg_file in "$PKG_DIR"/*.hpkg; do
    if [ -f "$hpkg_file" ]; then
        pkg_name=$(basename "$hpkg_file" .hpkg)
        log_info "Processing: $pkg_name"
        extract_libraries "$hpkg_file" "$SYSROOT_DIR/system/lib"
    fi
done

echo ""

# ============================================================================
# VERIFY RESULTS
# ============================================================================

log_info "Verifying sysroot contents..."
echo ""

LIB_COUNT=$(find "$SYSROOT_DIR/system/lib" -type f -name "*.so*" 2>/dev/null | wc -l)

log_success "Libraries found: $LIB_COUNT"

if [ $LIB_COUNT -gt 0 ]; then
    log_success "Essential libraries:"
    ls -1 "$SYSROOT_DIR/system/lib"/*.so* 2>/dev/null | sed 's/^/  • /'
else
    log_warning "No libraries found in sysroot"
fi

echo ""

# ============================================================================
# CLEANUP
# ============================================================================

log_info "Cleaning up temporary files..."
rm -rf "$TEMP_DIR"
log_success "Cleanup complete"

echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Sysroot Setup Summary:${NC}"
echo "  Location:      $SYSROOT_DIR"
echo "  Libraries:     $LIB_COUNT"
echo "  Downloaded:    $SUCCESS"
echo "  Failed:        $FAILED"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo ""

if [ $LIB_COUNT -gt 0 ]; then
    log_success "✓ Sysroot setup completed successfully"
    exit 0
else
    log_error "✗ Sysroot setup incomplete - no libraries found"
    log_info "Troubleshooting tips:"
    log_info "1. Check network connectivity to eu.hpkg.haiku-os.org"
    log_info "2. Try using a VPN if the server is geographically blocked"
    log_info "3. You can manually download packages from:"
    log_info "   https://eu.hpkg.haiku-os.org/haiku/r1beta5/x86_gcc2h/current/packages/"
    exit 1
fi
