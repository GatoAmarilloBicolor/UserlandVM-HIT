#!/bin/bash

# UserlandVM-HIT - Sysroot Verification Script
# Validates sysroot integrity and essential components

set -e

# Configuration
SYSROOT_PATH=${SYSROOT_PATH:-"sysroot/haiku32"}
VERIFICATION_MODE=${VERIFICATION_MODE:-"full"}  # quick, standard, full
REPORT_FILE=${REPORT_FILE:-"sysroot_verification_report.txt"}

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Statistics
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNING_CHECKS=0

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED_CHECKS++))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED_CHECKS++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    ((WARNING_CHECKS++))
}

check_passed() {
    ((TOTAL_CHECKS++))
}

check_failed() {
    ((TOTAL_CHECKS++))
}

check_warning() {
    ((TOTAL_CHECKS++))
}

# Progress indicator
show_progress() {
    local current=$1
    local total=$2
    local percent=$((current * 100 / total))
    local filled=$((percent / 2))
    local empty=$((50 - filled))
    
    printf "\r${BLUE}[PROGRESS]${NC} [%s%s] %d%% (%d/%d)" "$(printf '%*s' $filled | tr ' ' '=')" "$(printf '%*s' $empty)" "$percent" "$current" "$total"
    
    if [ $current -eq $total ]; then
        echo ""
    fi
}

# Initialize report
init_report() {
    echo "UserlandVM-HIT Sysroot Verification Report" > "$REPORT_FILE"
    echo "Generated on: $(date)" >> "$REPORT_FILE"
    echo "=========================================" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
}

# Write to report
write_report() {
    echo "$1" >> "$REPORT_FILE"
}

# Basic directory structure check
check_directory_structure() {
    log_info "Checking directory structure..."
    local dirs=(
        "bin"
        "lib"
        "develop/headers"
        "data"
        "apps"
    )
    
    local dir_count=${#dirs[@]}
    local current=0
    
    for dir in "${dirs[@]}"; do
        if [ -d "$SYSROOT_PATH/$dir" ]; then
            log_success "Directory exists: $dir"
            check_passed
            write_report "PASS: Directory $dir exists"
        else
            log_fail "Directory missing: $dir"
            check_failed
            write_report "FAIL: Directory $dir missing"
        fi
        show_progress $((++current)) $dir_count "Directory structure"
    done
}

# Essential libraries check
check_essential_libraries() {
    log_info "Checking essential libraries..."
    local essential_libs=(
        "lib/libroot.so"
        "lib/libbe.so"
        "lib/libstdc++.so.6"
        "lib/libgcc_s.so.1"
    )
    
    if [ "$VERIFICATION_MODE" = "full" ]; then
        essential_libs+=(
            "lib/libnetwork.so"
            "lib/libbsd.so"
            "lib/libz.so.1"
            "lib/libm.so.1"
        )
    fi
    
    local lib_count=${#essential_libs[@]}
    local current=0
    
    for lib in "${essential_libs[@]}"; do
        if [ -f "$SYSROOT_PATH/$lib" ]; then
            local lib_size=$(stat -f%z "$SYSROOT_PATH/$lib" 2>/dev/null || stat -c%s "$SYSROOT_PATH/$lib" 2>/dev/null || echo "unknown")
            log_success "Library exists: $lib (${lib_size} bytes)"
            check_passed
            write_report "PASS: Library $lib exists (${lib_size} bytes)"
        else
            log_fail "Library missing: $lib"
            check_failed
            write_report "FAIL: Library $lib missing"
        fi
        show_progress $((++current)) $lib_count "Essential libraries"
    done
}

# Development headers check
check_development_headers() {
    if [ "$VERIFICATION_MODE" = "quick" ]; then
        log_warning "Skipping header check in quick mode"
        check_warning
        write_report "SKIP: Header check skipped (quick mode)"
        return
    fi
    
    log_info "Checking development headers..."
    local header_dirs=(
        "develop/headers/os"
        "develop/headers/GL"
        "develop/headers/posix"
    )
    
    local dir_count=${#header_dirs[@]}
    local current=0
    
    for dir in "${header_dirs[@]}"; do
        if [ -d "$SYSROOT_PATH/$dir" ]; then
            local header_count=$(find "$SYSROOT_PATH/$dir" -name "*.h" | wc -l)
            log_success "Headers exist: $dir ($header_count headers)"
            check_passed
            write_report "PASS: Header directory $dir exists ($header_count headers)"
        else
            log_fail "Header directory missing: $dir"
            check_failed
            write_report "FAIL: Header directory $dir missing"
        fi
        show_progress $((++current)) $dir_count "Development headers"
    done
}

# System binaries check
check_system_binaries() {
    log_info "Checking system binaries..."
    local essential_bins=(
        "bin/sh"
        "bin/ls"
        "bin/cat"
        "bin/echo"
        "bin/pwd"
    )
    
    if [ "$VERIFICATION_MODE" = "full" ]; then
        essential_bins+=(
            "bin/bash"
            "bin/grep"
            "bin/find"
            "bin/ps"
        )
    fi
    
    local bin_count=${#essential_bins[@]}
    local current=0
    
    for bin in "${essential_bins[@]}"; do
        if [ -f "$SYSROOT_PATH/$bin" ]; then
            if [ -x "$SYSROOT_PATH/$bin" ]; then
                log_success "Binary exists and executable: $bin"
                check_passed
                write_report "PASS: Binary $bin exists and executable"
            else
                log_warning "Binary exists but not executable: $bin"
                check_warning
                write_report "WARN: Binary $bin exists but not executable"
            fi
        else
            log_fail "Binary missing: $bin"
            check_failed
            write_report "FAIL: Binary $bin missing"
        fi
        show_progress $((++current)) $bin_count "System binaries"
    done
}

# Data files check
check_data_files() {
    log_info "Checking data files..."
    local data_items=(
        "data/fonts/ttfonts"
        "data/locale/en.catalog"
    )
    
    if [ "$VERIFICATION_MODE" = "full" ]; then
        data_items+=(
            "data/fonts/otfonts"
            "data/locale/es.catalog"
            "data/mime_db"
        )
    fi
    
    local data_count=${#data_items[@]}
    local current=0
    
    for item in "${data_items[@]}"; do
        if [ -e "$SYSROOT_PATH/$item" ]; then
            log_success "Data item exists: $item"
            check_passed
            write_report "PASS: Data item $item exists"
        else
            log_warning "Data item missing: $item"
            check_warning
            write_report "WARN: Data item $item missing"
        fi
        show_progress $((++current)) $data_count "Data files"
    done
}

# Applications check
check_applications() {
    if [ "$VERIFICATION_MODE" = "quick" ]; then
        log_warning "Skipping applications check in quick mode"
        check_warning
        write_report "SKIP: Applications check skipped (quick mode)"
        return
    fi
    
    log_info "Checking applications..."
    local apps=(
        "apps/Terminal"
        "apps/DeskCalc"
    )
    
    if [ "$VERIFICATION_MODE" = "full" ]; then
        apps+=(
            "apps/StyledEdit"
            "apps/Showcase"
        )
    fi
    
    local app_count=${#apps[@]}
    local current=0
    
    for app in "${apps[@]}"; do
        if [ -f "$SYSROOT_PATH/$app" ]; then
            if [ -x "$SYSROOT_PATH/$app" ]; then
                log_success "Application exists and executable: $app"
                check_passed
                write_report "PASS: Application $app exists and executable"
            else
                log_warning "Application exists but not executable: $app"
                check_warning
                write_report "WARN: Application $app exists but not executable"
            fi
        else
            log_warning "Application missing: $app"
            check_warning
            write_report "WARN: Application $app missing"
        fi
        show_progress $((++current)) $app_count "Applications"
    done
}

# Size optimization check
check_size_optimization() {
    log_info "Checking size optimization..."
    
    local total_size=$(du -sb "$SYSROOT_PATH" | cut -f1)
    local size_mb=$((total_size / 1024 / 1024))
    
    write_report "Total sysroot size: ${size_mb}MB"
    
    if [ $size_mb -lt 100 ]; then
        log_success "Excellent size optimization: ${size_mb}MB (< 100MB)"
        check_passed
        write_report "PASS: Excellent size optimization (${size_mb}MB)"
    elif [ $size_mb -lt 200 ]; then
        log_success "Good size optimization: ${size_mb}MB (< 200MB)"
        check_passed
        write_report "PASS: Good size optimization (${size_mb}MB)"
    elif [ $size_mb -lt 500 ]; then
        log_warning "Acceptable size optimization: ${size_mb}MB (< 500MB)"
        check_warning
        write_report "WARN: Acceptable size optimization (${size_mb}MB)"
    else
        log_fail "Poor size optimization: ${size_mb}MB (> 500MB)"
        check_failed
        write_report "FAIL: Poor size optimization (${size_mb}MB)"
    fi
}

# Permission check
check_permissions() {
    log_info "Checking file permissions..."
    
    local permission_issues=0
    
    # Check for non-readable files
    while IFS= read -r -d '' file; do
        if [ ! -r "$file" ]; then
            log_warning "File not readable: $file"
            check_warning
            write_report "WARN: File not readable: $file"
            ((permission_issues++))
        fi
    done < <(find "$SYSROOT_PATH" -type f ! -perm -u=r 2>/dev/null)
    
    # Check for non-executable binaries
    while IFS= read -r -d '' file; do
        if [[ "$file" =~ (bin|apps) ]] && [ -f "$file" ] && [ ! -x "$file" ]; then
            log_warning "Binary not executable: $file"
            check_warning
            write_report "WARN: Binary not executable: $file"
            ((permission_issues++))
        fi
    done < <(find "$SYSROOT_PATH" -type f \( -path "*/bin/*" -o -path "*/apps/*" \) ! -perm -u=x 2>/dev/null)
    
    if [ $permission_issues -eq 0 ]; then
        log_success "All file permissions correct"
        check_passed
        write_report "PASS: All file permissions correct"
    else
        log_warning "Found $permission_issues permission issues"
        check_warning
        write_report "WARN: Found $permission_issues permission issues"
    fi
}

# Compatibility check
check_compatibility() {
    if [ "$VERIFICATION_MODE" = "quick" ]; then
        log_warning "Skipping compatibility check in quick mode"
        check_warning
        write_report "SKIP: Compatibility check skipped (quick mode)"
        return
    fi
    
    log_info "Checking Haiku compatibility..."
    
    # Check for BeOS-style structure
    local beos_indicators=(
        "develop/headers/os"
        "lib/libbe.so"
        "data/fonts"
    )
    
    local compatibility_score=0
    local total_checks=${#beos_indicators[@]}
    
    for indicator in "${beos_indicators[@]}"; do
        if [ -e "$SYSROOT_PATH/$indicator" ]; then
            ((compatibility_score++))
        fi
    done
    
    local compatibility_percent=$((compatibility_score * 100 / total_checks))
    write_report "Haiku compatibility score: ${compatibility_percent}%"
    
    if [ $compatibility_percent -ge 90 ]; then
        log_success "Excellent Haiku compatibility: ${compatibility_percent}%"
        check_passed
        write_report "PASS: Excellent Haiku compatibility (${compatibility_percent}%)"
    elif [ $compatibility_percent -ge 75 ]; then
        log_success "Good Haiku compatibility: ${compatibility_percent}%"
        check_passed
        write_report "PASS: Good Haiku compatibility (${compatibility_percent}%)"
    elif [ $compatibility_percent -ge 60 ]; then
        log_warning "Acceptable Haiku compatibility: ${compatibility_percent}%"
        check_warning
        write_report "WARN: Acceptable Haiku compatibility (${compatibility_percent}%)"
    else
        log_fail "Poor Haiku compatibility: ${compatibility_percent}%"
        check_failed
        write_report "FAIL: Poor Haiku compatibility (${compatibility_percent}%)"
    fi
}

# Generate final report
generate_final_report() {
    echo ""
    echo "Verification Summary"
    echo "==================="
    echo "Total Checks: $TOTAL_CHECKS"
    echo "Passed: $PASSED_CHECKS"
    echo "Failed: $FAILED_CHECKS"
    echo "Warnings: $WARNING_CHECKS"
    
    # Add to report
    write_report ""
    write_report "VERIFICATION SUMMARY"
    write_report "===================="
    write_report "Total Checks: $TOTAL_CHECKS"
    write_report "Passed: $PASSED_CHECKS"
    write_report "Failed: $FAILED_CHECKS"
    write_report "Warnings: $WARNING_CHECKS"
    
    # Calculate overall score
    local pass_rate=0
    if [ $TOTAL_CHECKS -gt 0 ]; then
        pass_rate=$((PASSED_CHECKS * 100 / TOTAL_CHECKS))
    fi
    
    write_report ""
    write_report "OVERALL SCORE: ${pass_rate}%"
    
    if [ $FAILED_CHECKS -eq 0 ]; then
        echo ""
        log_success "✅ All critical checks passed! Sysroot is ready for use."
        if [ $pass_rate -ge 95 ]; then
            log_success "Score: EXCELLENT (${pass_rate}%)"
            write_report "RATING: EXCELLENT"
        elif [ $pass_rate -ge 85 ]; then
            log_success "Score: VERY GOOD (${pass_rate}%)"
            write_report "RATING: VERY GOOD"
        else
            log_success "Score: GOOD (${pass_rate}%)"
            write_report "RATING: GOOD"
        fi
        echo ""
        log_info "Report saved to: $REPORT_FILE"
        return 0
    else
        echo ""
        log_fail "❌ Some critical checks failed!"
        if [ $pass_rate -ge 70 ]; then
            log_warning "Score: NEEDS ATTENTION (${pass_rate}%)"
            write_report "RATING: NEEDS ATTENTION"
        else
            log_fail "Score: REQUIRES REPAIR (${pass_rate}%)"
            write_report "RATING: REQUIRES REPAIR"
        fi
        echo ""
        log_info "Check the report for details: $REPORT_FILE"
        return 1
    fi
}

# Main execution
main() {
    echo "UserlandVM-HIT - Sysroot Verification"
    echo "==================================="
    echo "Mode: $VERIFICATION_MODE"
    echo "Path: $SYSROOT_PATH"
    echo ""
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --quick)
                VERIFICATION_MODE="quick"
                shift
                ;;
            --standard)
                VERIFICATION_MODE="standard"
                shift
                ;;
            --full)
                VERIFICATION_MODE="full"
                shift
                ;;
            --sysroot=*)
                SYSROOT_PATH="${1#*=}"
                shift
                ;;
            --report=*)
                REPORT_FILE="${1#*=}"
                shift
                ;;
            --help)
                echo "Usage: $0 [options]"
                echo "Options:"
                echo "  --quick            Quick verification (basic checks only)"
                echo "  --standard         Standard verification (most checks)"
                echo "  --full             Full verification (all checks)"
                echo "  --sysroot=PATH    Set sysroot path to verify"
                echo "  --report=FILE      Set output report file"
                echo "  --help             Show this help"
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    # Check if sysroot exists
    if [ ! -d "$SYSROOT_PATH" ]; then
        log_fail "Sysroot not found: $SYSROOT_PATH"
        exit 1
    fi
    
    # Initialize report
    init_report
    
    # Run verification checks
    check_directory_structure
    check_essential_libraries
    check_development_headers
    check_system_binaries
    check_data_files
    check_applications
    check_size_optimization
    check_permissions
    check_compatibility
    
    # Generate final report
    generate_final_report
}

# Run main function
main "$@"