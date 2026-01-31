#!/bin/bash
# quick_start.sh - Quick start guide for UserlandVM complex testing
#
# This script automates the entire setup process:
# 1. Downloads sysroot from Haiku HPKG repository
# 2. Builds test programs
# 3. Runs basic tests
# 4. Provides next steps

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Paths
REPO_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$REPO_DIR/build.x86_64"
SYSROOT_DIR="$REPO_DIR/sysroot/haiku32"
TEST_BINS="$REPO_DIR/test_programs/binaries"

log_header() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${NC}  $1"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

log_step() {
    echo -e "${BLUE}[Step]${NC} $1"
}

log_info() {
    echo -e "${BLUE}[Info]${NC} $1"
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

check_command() {
    if command -v "$1" &> /dev/null; then
        return 0
    else
        return 1
    fi
}

# ============================================================================
# MAIN FLOW
# ============================================================================

clear

log_header "UserlandVM Quick Start - Complex Testing Setup"

echo "This script will:"
echo "  1. Verify project structure"
echo "  2. Download Haiku sysroot packages"
echo "  3. Build test programs"
echo "  4. Run basic validation tests"
echo "  5. Show next steps"
echo ""

read -p "Continue? [y/N] " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    log_info "Cancelled by user"
    exit 0
fi

# ============================================================================
# STEP 1: VERIFY PROJECT
# ============================================================================

log_header "STEP 1: Verify Project Structure"

log_step "Checking directory structure..."

if [ ! -f "$REPO_DIR/Main.cpp" ]; then
    log_error "Main.cpp not found - are you in the correct directory?"
    exit 1
fi

log_success "✓ Main project files found"

if [ ! -f "$REPO_DIR/setup_sysroot_from_hpkg.sh" ]; then
    log_error "setup_sysroot_from_hpkg.sh not found"
    exit 1
fi

log_success "✓ Setup scripts found"

if [ ! -f "$REPO_DIR/build_test_programs.sh" ]; then
    log_error "build_test_programs.sh not found"
    exit 1
fi

log_success "✓ Test build script found"

# ============================================================================
# STEP 2: VERIFY DEPENDENCIES
# ============================================================================

log_header "STEP 2: Check Dependencies"

log_step "Checking required tools..."

MISSING_TOOLS=0

if ! check_command gcc; then
    log_warning "gcc not found (needed for test compilation)"
    ((MISSING_TOOLS++))
else
    log_success "gcc available"
fi

if ! check_command curl && ! check_command wget; then
    log_error "Neither curl nor wget found (required for downloads)"
    log_info "Install: pkgman install curl"
    exit 1
fi

if check_command curl; then
    log_success "curl available"
else
    log_success "wget available"
fi

if ! check_command unzip; then
    log_warning "unzip not found (needed for HPKG extraction)"
    log_info "Optional: pkgman install unzip"
fi

# ============================================================================
# STEP 3: BUILD PROJECT
# ============================================================================

log_header "STEP 3: Build UserlandVM"

if [ -f "$BUILD_DIR/UserlandVM" ]; then
    log_info "UserlandVM binary already built"
    log_step "Rebuild? [y/N] "
    read -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        log_step "Rebuilding..."
        cd "$REPO_DIR"
        meson setup build.x86_64 --wipe 2>/dev/null || true
        ninja -C build.x86_64 2>&1 | tail -20
        log_success "Build complete"
    fi
else
    log_step "Building UserlandVM..."
    cd "$REPO_DIR"
    meson setup build.x86_64 2>/dev/null || true
    ninja -C build.x86_64 2>&1 | tail -20
    
    if [ ! -f "$BUILD_DIR/UserlandVM" ]; then
        log_error "Build failed"
        exit 1
    fi
    
    log_success "Build complete"
fi

# ============================================================================
# STEP 4: SETUP SYSROOT
# ============================================================================

log_header "STEP 4: Setup Sysroot from Haiku Packages"

LIB_COUNT=$(find "$SYSROOT_DIR/system/lib" -name "*.so*" -type f 2>/dev/null | wc -l)

if [ $LIB_COUNT -gt 0 ]; then
    log_info "Sysroot already has $LIB_COUNT libraries"
    log_step "Re-download sysroot? [y/N] "
    read -n 1 -r
    echo
    
    if ! [[ $REPLY =~ ^[Yy]$ ]]; then
        log_success "Using existing sysroot"
        goto_build_tests=1
    fi
fi

if [ -z "$goto_build_tests" ]; then
    log_step "Downloading Haiku packages..."
    log_info "This may take a few minutes depending on connection speed..."
    echo ""
    
    if "$REPO_DIR/setup_sysroot_from_hpkg.sh" 2>&1 | tail -30; then
        log_success "Sysroot setup complete"
    else
        log_error "Sysroot setup failed"
        log_info "Try manually: ./setup_sysroot_from_hpkg.sh"
        exit 1
    fi
fi

# Verify sysroot
LIB_COUNT=$(find "$SYSROOT_DIR/system/lib" -name "*.so*" -type f 2>/dev/null | wc -l)

if [ $LIB_COUNT -lt 3 ]; then
    log_warning "Sysroot may be incomplete (only $LIB_COUNT libraries)"
    log_info "Check: ls $SYSROOT_DIR/system/lib/"
fi

log_success "Sysroot has $LIB_COUNT libraries"

# ============================================================================
# STEP 5: BUILD TEST PROGRAMS
# ============================================================================

log_header "STEP 5: Build Test Programs"

log_step "Compiling test programs..."

if "$REPO_DIR/build_test_programs.sh" 2>&1 | tail -30; then
    log_success "Test programs built"
else
    log_warning "Some test programs failed to build"
fi

TEST_COUNT=$(ls "$TEST_BINS" 2>/dev/null | wc -l)
log_success "Built $TEST_COUNT test programs"

# ============================================================================
# STEP 6: RUN BASIC TESTS
# ============================================================================

log_header "STEP 6: Run Basic Validation Tests"

if [ ! -x "$BUILD_DIR/UserlandVM" ]; then
    log_error "UserlandVM binary not executable"
    exit 1
fi

if [ $TEST_COUNT -eq 0 ]; then
    log_warning "No test programs available"
fi

# Try to run a simple test
if [ -f "$TEST_BINS/test_syscall_static" ]; then
    log_step "Running: test_syscall_static"
    echo "---"
    if timeout 5 "$BUILD_DIR/UserlandVM" "$TEST_BINS/test_syscall_static" 2>&1 | head -20; then
        log_success "Test executed"
    else
        log_warning "Test execution issue"
    fi
    echo "---"
fi

# ============================================================================
# FINAL SUMMARY
# ============================================================================

log_header "✓ Setup Complete!"

echo "Project Status:"
echo "  Build:        $([ -x "$BUILD_DIR/UserlandVM" ] && echo "✓ Ready" || echo "✗ Failed")"
echo "  Sysroot:      ✓ $LIB_COUNT libraries"
echo "  Test Progs:   ✓ $TEST_COUNT binaries"
echo ""

echo "Next Steps:"
echo ""
echo "  1. Run a single test:"
echo "     ./build.x86_64/UserlandVM test_programs/binaries/test_syscall_static"
echo ""
echo "  2. Run all tests:"
echo "     for test in test_programs/binaries/*; do"
echo "       echo \"Testing: \$(basename \$test)\""
echo "       ./build.x86_64/UserlandVM \"\$test\""
echo "     done"
echo ""
echo "  3. Test with dynamic linking:"
echo "     ./build.x86_64/UserlandVM test_programs/binaries/test_libc_basic"
echo ""
echo "  4. See testing guide:"
echo "     cat SYSROOT_COMPLEX_TESTING_GUIDE.md"
echo ""
echo "  5. See sprint plan:"
echo "     cat SPRINT_6_COMPLEX_PROGRAMS_PLAN.md"
echo ""

echo "Useful Commands:"
echo ""
echo "  # Download additional packages (with tools)"
echo "  ./setup_sysroot_from_hpkg.sh sysroot/haiku32 --with-ports"
echo ""
echo "  # Check sysroot contents"
echo "  ls -lh sysroot/haiku32/system/lib/"
echo ""
echo "  # Run with verbose output"
echo "  ./build.x86_64/UserlandVM program --verbose"
echo ""
echo "  # Trace syscalls"
echo "  strace -f ./build.x86_64/UserlandVM program 2>&1 | head -30"
echo ""

echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo "UserlandVM is ready for complex program testing!"
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
