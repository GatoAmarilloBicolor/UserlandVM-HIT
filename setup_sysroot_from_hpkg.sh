#!/bin/bash
# setup_sysroot_from_hpkg.sh - Configure sysroot from Haiku package repository
# 
# Descarga paquetes .hpkg desde https://eu.hpkg.haiku-os.org/haiku/master/x86_gcc2/current
# Extrae las bibliotecas necesarias para el sysroot del UserlandVM
#
# Uso:
#   ./setup_sysroot_from_hpkg.sh [sysroot_dir] [--with-ports]
#
# Ejemplos:
#   ./setup_sysroot_from_hpkg.sh                           # Usa sysroot/haiku32
#   ./setup_sysroot_from_hpkg.sh ./sysroot/haiku32         # Personalizado
#   ./setup_sysroot_from_hpkg.sh ./sysroot/haiku32 --with-ports # Incluye haikuports

set -e

# ============================================================================
# CONFIGURATION
# ============================================================================

SYSROOT_DIR="${1:-.}/sysroot/haiku32"
INCLUDE_PORTS="${2:-}"
REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Repository URLs
HPKG_BASE="https://eu.hpkg.haiku-os.org/haiku/master/x86_gcc2/current"
HAIKUPORTS_BASE="https://eu.hpkg.haiku-os.org/haikuports/master/x86_gcc2/current"

# Temporary directory for downloads and extraction
TEMP_DIR="${REPO_DIR}/.hpkg_temp"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Check if command exists
command_exists() {
    command -v "$1" &> /dev/null
    return $?
}

# Download a package
download_hpkg() {
    local pkg_name=$1
    local repo=$2
    local dest_dir=$3
    
    local url="${repo}/${pkg_name}.hpkg"
    local dest_file="${dest_dir}/${pkg_name}.hpkg"
    
    if [ -f "$dest_file" ]; then
        log_success "Package already downloaded: $pkg_name"
        return 0
    fi
    
    log_info "Downloading: $pkg_name"
    
    if command_exists curl; then
        if curl -L -f -o "$dest_file" --progress-bar "$url" 2>/dev/null; then
            log_success "Downloaded: $pkg_name"
            return 0
        else
            log_error "Failed to download: $pkg_name"
            return 1
        fi
    elif command_exists wget; then
        if wget -q --show-progress -O "$dest_file" "$url" 2>/dev/null; then
            log_success "Downloaded: $pkg_name"
            return 0
        else
            log_error "Failed to download: $pkg_name"
            return 1
        fi
    else
        log_error "Neither curl nor wget found. Install one and try again."
        return 1
    fi
}

# Extract library from HPKG
extract_lib_from_hpkg() {
    local hpkg_file=$1
    local lib_pattern=$2
    local dest_dir=$3
    
    if [ ! -f "$hpkg_file" ]; then
        log_warning "HPKG file not found: $hpkg_file"
        return 1
    fi
    
    # HPKG files are basically zip files, can be extracted with unzip or 7z
    if command_exists unzip; then
        # Extract to temporary directory
        local temp_extract="${TEMP_DIR}/extract_$$"
        mkdir -p "$temp_extract"
        
        if unzip -q "$hpkg_file" -d "$temp_extract" 2>/dev/null; then
            # Find and copy libraries
            find "$temp_extract" -name "$lib_pattern" -type f | while read lib_file; do
                cp "$lib_file" "$dest_dir/"
                log_success "Extracted: $(basename $lib_file)"
            done
            
            rm -rf "$temp_extract"
            return 0
        fi
    fi
    
    log_warning "Could not extract $hpkg_file (unzip not available)"
    return 1
}

# ============================================================================
# MAIN
# ============================================================================

echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  UserlandVM Sysroot Setup from Haiku Packages (.hpkg)     ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

log_info "Repository: $HPKG_BASE"
log_info "Sysroot: $SYSROOT_DIR"

if [ ! -z "$INCLUDE_PORTS" ] && [ "$INCLUDE_PORTS" = "--with-ports" ]; then
    log_info "Including HaikuPorts: $HAIKUPORTS_BASE"
fi

echo ""

# ============================================================================
# VERIFY PREREQUISITES
# ============================================================================

log_info "Checking prerequisites..."

if ! command_exists curl && ! command_exists wget; then
    log_error "Neither curl nor wget is installed"
    echo "On Haiku, install with: pkgman install curl"
    exit 1
fi

log_success "Download tools available"

if ! command_exists unzip && ! command_exists 7z; then
    log_warning "Neither unzip nor 7z found - cannot extract packages"
    log_warning "Install with: pkgman install unzip"
fi

# Check network
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
mkdir -p "$SYSROOT_DIR/boot/system/develop/headers"
mkdir -p "$TEMP_DIR"

log_success "Directories created"
echo ""

# ============================================================================
# DOWNLOAD AND EXTRACT PACKAGES
# ============================================================================

# Essential base packages for x86_gcc2 architecture
declare -a BASE_PACKAGES=(
    "haiku_x86_gcc2"
    "haiku_x86_gcc2_devel"
)

# Essential libraries
declare -a ESSENTIAL_LIBS=(
    "libc.so.0"
    "libm.so.0"
    "libroot.so"
    "libc.so"
    "libgcc_s.so.1"
)

# Optional but useful packages
declare -a OPTIONAL_PACKAGES=(
    "curl"
    "wget"
    "bash"
    "coreutils"
)

# GUI packages (for future GUI support)
declare -a GUI_PACKAGES=(
    "libbe"
    "libappkit"
)

log_info "Phase 1: Downloading base packages..."
echo ""

SUCCESS=0
FAILED=0
SKIPPED=0

# Download base packages
for pkg in "${BASE_PACKAGES[@]}"; do
    if download_hpkg "$pkg" "$HPKG_BASE" "$TEMP_DIR"; then
        ((SUCCESS++))
    else
        ((FAILED++))
    fi
done

echo ""
log_info "Phase 2: Extracting libraries..."
echo ""

# Extract libraries from downloaded packages
for hpkg_file in "$TEMP_DIR"/*.hpkg; do
    if [ -f "$hpkg_file" ]; then
        pkg_name=$(basename "$hpkg_file" .hpkg)
        log_info "Processing: $pkg_name"
        
        # Extract .so and .so.* libraries
        extract_lib_from_hpkg "$hpkg_file" "*.so*" "$SYSROOT_DIR/system/lib"
    fi
done

echo ""

# ============================================================================
# OPTIONAL PACKAGES
# ============================================================================

if [ ! -z "$INCLUDE_PORTS" ] && [ "$INCLUDE_PORTS" = "--with-ports" ]; then
    echo ""
    log_info "Phase 3: Downloading optional packages (with-ports)..."
    echo ""
    
    # Download some useful tools from haikuports
    for pkg in curl bash; do
        if download_hpkg "$pkg" "$HAIKUPORTS_BASE" "$TEMP_DIR"; then
            ((SUCCESS++))
        else
            log_warning "Optional package failed: $pkg (continuing)"
            ((SKIPPED++))
        fi
    done
fi

# ============================================================================
# VERIFY RESULTS
# ============================================================================

echo ""
log_info "Verifying sysroot contents..."
echo ""

LIB_COUNT=$(find "$SYSROOT_DIR/system/lib" -type f -name "*.so*" 2>/dev/null | wc -l)
HEADER_COUNT=$(find "$SYSROOT_DIR/system/develop/headers" -type f 2>/dev/null | wc -l)

log_success "Libraries found: $LIB_COUNT"
log_success "Headers found: $HEADER_COUNT"

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
echo "  Headers:       $HEADER_COUNT"
echo "  Downloaded:    $SUCCESS"
echo "  Failed:        $FAILED"
echo "  Skipped:       $SKIPPED"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Check if we have libc at least
if [ $LIB_COUNT -gt 0 ]; then
    log_success "✓ Sysroot is ready for use"
    exit 0
else
    log_error "✗ Sysroot setup incomplete - no libraries found"
    exit 1
fi
