#!/bin/bash

# UserlandVM-HIT - Minimal Sysroot Generator
# Optimized sysroot creation for Haiku x86-32 emulation
# Reduces 1.2GB → ~60MB while maintaining full functionality

set -e

# Configuration
MINIMAL_MODE=${MINIMAL_MODE:-true}           # true=minimal, false=full
INCLUDE_LANGUAGES=${INCLUDE_LANGUAGES:-"en es fr de"}  # Languages to keep
INCLUDE_GUI=${INCLUDE_GUI:-true}                # Include full GUI support
OUTPUT_FILE=${OUTPUT_FILE:-"userlandvm-sysroot"}   # Base name for output
COMPRESSION=${COMPRESSION:-"xz"}                   # Compression: xz, gz, bz2

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Progress indicator
progress() {
    local current=$1
    local total=$2
    local desc=$3
    local percent=$((current * 100 / total))
    local filled=$((percent / 2))
    local empty=$((50 - filled))
    
    printf "\r${BLUE}[PROGRESS]${NC} %s [%s%s] %d%%" "$desc" "$(printf '%*s' $filled | tr ' ' '=')" "$(printf '%*s' $empty)" "$percent"
    
    if [ $current -eq $total ]; then
        echo ""
    fi
}

# Check if sysroot exists
check_sysroot() {
    log_info "Checking existing sysroot..."
    if [ ! -d "sysroot/haiku32" ]; then
        log_error "sysroot/haiku32 not found. Please run setup_sysroot.sh first."
        exit 1
    fi
    
    local original_size=$(du -sh sysroot/haiku32 | cut -f1)
    log_info "Original sysroot size: $original_size"
}

# Phase 1: Aggressive cleanup
aggressive_cleanup() {
    log_info "Phase 1: Aggressive cleanup (removing 800MB+)"
    
    local total_steps=7
    local current_step=0
    
    # Remove redundant filesystem image
    if [ -f "sysroot/haiku32/haiku_full.bfs" ]; then
        rm -f sysroot/haiku32/haiku_full.bfs
        log_success "Removed haiku_full.bfs (439MB)"
    fi
    progress $((++current_step)) $total_steps "Cleanup"
    
    # Remove firmware (hardware drivers not needed in VM)
    if [ -d "sysroot/haiku32/data/firmware" ]; then
        rm -rf sysroot/haiku32/data/firmware
        log_success "Removed firmware directory (~60MB)"
    fi
    progress $((++current_step)) $total_steps "Cleanup"
    
    # Remove kernel add-ons
    if [ -d "sysroot/haiku32/add-ons/kernel" ]; then
        rm -rf sysroot/haiku32/add-ons/kernel
        log_success "Removed kernel add-ons (~23MB)"
    fi
    progress $((++current_step)) $total_steps "Cleanup"
    
    # Remove documentation
    if [ -d "sysroot/haiku32/documentation" ]; then
        rm -rf sysroot/haiku32/documentation
        log_success "Removed documentation (33MB)"
    fi
    progress $((++current_step)) $total_steps "Cleanup"
    
    # Remove demos
    if [ -d "sysroot/haiku32/demos" ]; then
        rm -rf sysroot/haiku32/demos
        log_success "Removed demos (2.4MB)"
    fi
    progress $((++current_step)) $total_steps "Cleanup"
    
    # Remove non-essential applications in minimal mode
    if [ "$MINIMAL_MODE" = "true" ]; then
        # Keep only essential apps for testing
        mkdir -p temp_apps
        for app in Terminal DeskCalc StyledEdit; do
            if [ -f "sysroot/haiku32/apps/$app" ]; then
                mv "sysroot/haiku32/apps/$app" temp_apps/
            fi
        done
        rm -rf sysroot/haiku32/apps/*
        mv temp_apps/* sysroot/haiku32/apps/
        rm -rf temp_apps
        log_success "Kept essential apps only (~10MB)"
    fi
    progress $((++current_step)) $total_steps "Cleanup"
    
    # Remove kernel binaries
    rm -f sysroot/haiku32/kernel_x86
    rm -f sysroot/haiku32/haiku_loader.bios_ia32
    log_success "Removed kernel binaries (2MB)"
    progress $((++current_step)) $total_steps "Cleanup"
}

# Phase 2: Optimize fonts
optimize_fonts() {
    log_info "Phase 2: Optimizing fonts (reducing 190MB → 5MB)"
    
    local temp_fonts="temp_fonts_$$"
    mkdir -p "$temp_fonts"
    
    # Essential fonts for BeOS/Haiku compatibility
    local essential_fonts=(
        "ttfonts/DejaVuSans.ttf"
        "ttfonts/DejaVuSansMono.ttf"
        "ttfonts/DejaVuSerif.ttf"
        "ttfonts/NotoSans-Regular.ttf"
        "ttfonts/NotoSans-Bold.ttf"
        "ttfonts/BitstreamVeraSans.ttf"
    )
    
    for font in "${essential_fonts[@]}"; do
        if [ -f "sysroot/haiku32/data/fonts/$font" ]; then
            mkdir -p "$temp_fonts/$(dirname "$font")"
            cp "sysroot/haiku32/data/fonts/$font" "$temp_fonts/$font"
        fi
    done
    
    # Replace fonts directory
    rm -rf sysroot/haiku32/data/fonts
    mkdir -p sysroot/haiku32/data/fonts/ttfonts
    cp -r "$temp_fonts"/* sysroot/haiku32/data/fonts/
    rm -rf "$temp_fonts"
    
    local font_size=$(du -sh sysroot/haiku32/data/fonts | cut -f1)
    log_success "Fonts optimized to: $font_size"
}

# Phase 3: Optimize locales
optimize_locales() {
    log_info "Phase 3: Optimizing locales (reducing 110MB → 4MB)"
    
    local temp_locale="temp_locale_$$"
    mkdir -p "$temp_locale"
    
    # Copy only specified languages
    for lang in $INCLUDE_LANGUAGES; do
        if ls sysroot/haiku32/data/locale/${lang}* >/dev/null 2>&1; then
            cp sysroot/haiku32/data/locale/${lang}* "$temp_locale/"
        fi
    done
    
    # Always copy English if not specified
    if ! echo "$INCLUDE_LANGUAGES" | grep -q "en"; then
        cp sysroot/haiku32/data/locale/en* "$temp_locale/"
    fi
    
    # Replace locale directory
    rm -rf sysroot/haiku32/data/locale
    mkdir -p sysroot/haiku32/data/locale
    cp "$temp_locale"/* sysroot/haiku32/data/locale/
    rm -rf "$temp_locale"
    
    local locale_count=$(find sysroot/haiku32/data/locale -name "*.catalog" | wc -l)
    log_success "Kept $locale_count locale catalogs"
}

# Phase 4: Optimize libraries
optimize_libraries() {
    log_info "Phase 4: Optimizing libraries for $MINIMAL_MODE mode"
    
    local temp_lib="temp_lib_$$"
    mkdir -p "$temp_lib"
    
    # Essential libraries (always keep)
    local essential_libs=(
        "libroot.so"
        "libbe.so"
        "libnetwork.so"
        "libstdc++.so.6"
        "libgcc_s.so.1"
        "libbsd.so"
        "libz.so.1"
        "libm.so.1"
    )
    
    # Additional GUI libraries if needed
    if [ "$INCLUDE_GUI" != "false" ]; then
        essential_libs+=(
            "libtracker.so"
            "libtranslation.so"
            "libbnetapi.so"
            "libmedia.so"
            "libgame.so"
        )
    fi
    
    # Copy essential libraries
    for lib in "${essential_libs[@]}"; do
        if [ -f "sysroot/haiku32/lib/$lib" ]; then
            cp "sysroot/haiku32/lib/$lib" "$temp_lib/"
        fi
    done
    
    # Copy library directories that might be needed
    for dir in coreutils; do
        if [ -d "sysroot/haiku32/lib/$dir" ]; then
            cp -r "sysroot/haiku32/lib/$dir" "$temp_lib/"
        fi
    done
    
    # Replace lib directory
    rm -rf sysroot/haiku32/lib
    mkdir -p sysroot/haiku32/lib
    cp -r "$temp_lib"/* sysroot/haiku32/lib/
    rm -rf "$temp_lib"
    
    local lib_count=$(find sysroot/haiku32/lib -name "*.so" | wc -l)
    log_success "Kept $lib_count essential libraries"
}

# Phase 5: Compress text files
compress_text_files() {
    log_info "Phase 5: Compressing text files"
    
    # Compress any remaining catalog files
    find sysroot/haiku32 -name "*.catalog" -exec gzip -9 {} \;
    
    local compressed_count=$(find sysroot/haiku32 -name "*.catalog.gz" | wc -l)
    log_success "Compressed $compressed_count catalog files"
}

# Phase 6: Create final package
create_package() {
    log_info "Phase 6: Creating compressed package"
    
    local package_name="${OUTPUT_FILE}"
    if [ "$MINIMAL_MODE" = "true" ]; then
        package_name="${package_name}-minimal"
    else
        package_name="${package_name}-full"
    fi
    
    local final_size=$(du -sh sysroot/haiku32 | cut -f1)
    log_info "Final optimized size: $final_size"
    
    case "$COMPRESSION" in
        "xz")
            tar -cJf "${package_name}.tar.xz" -C sysroot haiku32/
            ;;
        "gz")
            tar -czf "${package_name}.tar.gz" -C sysroot haiku32/
            ;;
        "bz2")
            tar -cjf "${package_name}.tar.bz2" -C sysroot haiku32/
            ;;
        *)
            log_warning "Unknown compression: $COMPRESSION, using gz"
            tar -czf "${package_name}.tar.gz" -C sysroot haiku32/
            ;;
    esac
    
    local package_size=$(du -sh "${package_name}.tar."* | cut -f1)
    log_success "Created package: ${package_name}.tar.* (${package_size})"
}

# Main execution
main() {
    echo "UserlandVM-HIT - Minimal Sysroot Generator"
    echo "============================================"
    echo "Mode: $([ "$MINIMAL_MODE" = "true" ] && echo "Minimal" || echo "Full")"
    echo "Languages: $INCLUDE_LANGUAGES"
    echo "GUI Support: $INCLUDE_GUI"
    echo "Compression: $COMPRESSION"
    echo ""
    
    check_sysroot
    aggressive_cleanup
    optimize_fonts
    optimize_locales
    optimize_libraries
    compress_text_files
    create_package
    
    echo ""
    log_success "Optimized sysroot package created successfully!"
    echo ""
    echo "Usage:"
    echo "  tar -xf ${OUTPUT_FILE}-*.tar.* -C /path/to/userlandvm/"
    echo ""
}

# Run main function
main "$@"