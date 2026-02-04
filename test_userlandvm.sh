#!/bin/bash

# UserlandVM-HIT - Comprehensive Testing Suite
# Tests emulator functionality with various Haiku applications

set -e

# Configuration
SYSROOT_PATH=${SYSROOT_PATH:-"sysroot/haiku32"}
USERLANDVM_BIN=${USERLANDVM_BIN:-"./UserlandVM"}
TEST_TIMEOUT=${TEST_TIMEOUT:-30}              # Timeout per test (seconds)
VERBOSE_TESTS=${VERBOSE_TESTS:-false}         # Enable verbose output
SKIP_GUI_TESTS=${SKIP_GUI_TESTS:-false}      # Skip GUI tests on headless systems

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Test statistics
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED_TESTS++))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED_TESTS++))
}

log_skip() {
    echo -e "${YELLOW}[SKIP]${NC} $1"
    ((SKIPPED_TESTS++))
}

log_test() {
    echo -e "${CYAN}[TEST]${NC} $1"
    ((TOTAL_TESTS++))
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

# Environment check
check_environment() {
    log_info "Checking test environment..."
    
    # Check if sysroot exists
    if [ ! -d "$SYSROOT_PATH" ]; then
        log_fail "Sysroot not found at: $SYSROOT_PATH"
        log_info "Please run setup_sysroot.sh or extract a sysroot package"
        exit 1
    fi
    
    # Check if UserlandVM binary exists
    if [ ! -f "$USERLANDVM_BIN" ]; then
        log_fail "UserlandVM binary not found: $USERLANDVM_BIN"
        log_info "Please compile UserlandVM first with: meson setup builddir && ninja -C builddir"
        exit 1
    fi
    
    # Check if binary is executable
    if [ ! -x "$USERLANDVM_BIN" ]; then
        log_fail "UserlandVM binary is not executable: $USERLANDVM_BIN"
        exit 1
    fi
    
    # Display environment info
    local sysroot_size=$(du -sh "$SYSROOT_PATH" | cut -f1)
    log_info "Sysroot: $SYSROOT_PATH ($sysroot_size)"
    log_info "Binary: $USERLANDVM_BIN"
    log_info "Timeout: ${TEST_TIMEOUT}s per test"
    
    if [ "$VERBOSE_TESTS" = "true" ]; then
        log_info "Verbose mode enabled"
    fi
    
    echo ""
}

# Create simple test binaries
create_test_binaries() {
    log_info "Creating test binaries..."
    
    # Ensure test directory exists
    mkdir -p tests/binaries
    
    # Simple "Hello World" test
    cat > tests/hello.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Hello from UserlandVM-HIT!\\n");
    printf("Testing basic C library functionality.\\n");
    return 42;
}
EOF
    
    # Test file operations
    cat > tests/file_test.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    FILE *file;
    const char *test_text = "UserlandVM file test";
    char buffer[100];
    
    // Test file creation
    file = fopen("/tmp/test_vm.txt", "w");
    if (!file) {
        printf("FAIL: Could not create file\\n");
        return 1;
    }
    
    // Test file writing
    if (fwrite(test_text, 1, strlen(test_text), file) != strlen(test_text)) {
        printf("FAIL: Could not write to file\\n");
        fclose(file);
        return 1;
    }
    fclose(file);
    
    // Test file reading
    file = fopen("/tmp/test_vm.txt", "r");
    if (!file) {
        printf("FAIL: Could not open file for reading\\n");
        return 1;
    }
    
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[bytes_read] = '\\0';
    fclose(file);
    
    if (strcmp(buffer, test_text) == 0) {
        printf("PASS: File operations successful\\n");
        return 0;
    } else {
        printf("FAIL: File content mismatch\\n");
        return 1;
    }
}
EOF
    
    # Test basic math operations
    cat > tests/math_test.c << 'EOF'
#include <stdio.h>
#include <math.h>

int main() {
    double a = 3.14159;
    double b = 2.71828;
    
    printf("Math test: %.6f + %.6f = %.6f\\n", a, b, a + b);
    printf("Math test: sqrt(16) = %.6f\\n", sqrt(16.0));
    printf("Math test: sin(π/2) = %.6f\\n", sin(3.14159 / 2.0));
    
    // Check basic results
    if (fabs((a + b) - 5.85987) < 0.00001 && 
        fabs(sqrt(16.0) - 4.0) < 0.00001 &&
        fabs(sin(3.14159 / 2.0) - 1.0) < 0.00001) {
        printf("PASS: Math operations successful\\n");
        return 0;
    } else {
        printf("FAIL: Math operations failed\\n");
        return 1;
    }
}
EOF
    
    # Compile test binaries with Haiku cross-compiler if available, otherwise host compiler
    local compiler_cmd="gcc"
    local compile_flags="-static -nostdlib -I$SYSROOT_PATH/develop/headers"
    
    # Try to use Haiku compiler if available
    if command -v "$SYSROOT_PATH/develop/tools/bin/gcc" >/dev/null 2>&1; then
        compiler_cmd="$SYSROOT_PATH/develop/tools/bin/gcc"
        compile_flags="-nostdlib -I$SYSROOT_PATH/develop/headers"
    fi
    
    # Compile tests
    for test in hello file_test math_test; do
        if "$compiler_cmd" $compile_flags "tests/${test}.c" -o "tests/binaries/${test}" 2>/dev/null; then
            log_success "Compiled ${test}"
        else
            log_info "Could not compile ${test} with Haiku compiler, trying host compiler..."
            if gcc "tests/${test}.c" -o "tests/binaries/${test}" 2>/dev/null; then
                log_info "Compiled ${test} with host compiler"
            else
                log_fail "Could not compile ${test}"
                return 1
            fi
        fi
    done
    
    chmod +x tests/binaries/*
    log_success "Test binaries created"
}

# Test basic functionality
test_basic_functionality() {
    log_test "Basic Functionality Tests"
    
    local test_count=3
    local current=0
    
    # Test simple binary execution
    if [ -f "tests/binaries/hello" ]; then
        echo "Testing hello world..."
        if timeout $TEST_TIMEOUT "$USERLANDVM_BIN" tests/binaries/hello 2>/dev/null; then
            log_success "Hello world test"
        else
            log_fail "Hello world test"
        fi
    else
        log_skip "Hello world test (binary not available)"
    fi
    show_progress $((++current)) $test_count "Basic tests"
    
    # Test file operations
    if [ -f "tests/binaries/file_test" ]; then
        echo "Testing file operations..."
        if timeout $TEST_TIMEOUT "$USERLANDVM_BIN" tests/binaries/file_test 2>/dev/null; then
            log_success "File operations test"
        else
            log_fail "File operations test"
        fi
    else
        log_skip "File operations test (binary not available)"
    fi
    show_progress $((++current)) $test_count "Basic tests"
    
    # Test math operations
    if [ -f "tests/binaries/math_test" ]; then
        echo "Testing math operations..."
        if timeout $TEST_TIMEOUT "$USERLANDVM_BIN" tests/binaries/math_test 2>/dev/null; then
            log_success "Math operations test"
        else
            log_fail "Math operations test"
        fi
    else
        log_skip "Math operations test (binary not available)"
    fi
    show_progress $((++current)) $test_count "Basic tests"
}

# Test Haiku applications
test_haiku_applications() {
    log_test "Haiku Applications Tests"
    
    local haiku_apps=(
        "Terminal"
        "DeskCalc"
        "StyledEdit"
    )
    
    local test_count=${#haiku_apps[@]}
    local current=0
    
    for app in "${haiku_apps[@]}"; do
        local app_path="$SYSROOT_PATH/apps/$app"
        
        if [ -f "$app_path" ]; then
            echo "Testing $app..."
            if [ "$SKIP_GUI_TESTS" = "true" ]; then
                log_skip "$app (GUI tests disabled)"
            else
                # Run with timeout and check if it doesn't crash immediately
                if timeout $TEST_TIMEOUT "$USERLANDVM_BIN" "$app_path" --help 2>/dev/null >/dev/null; then
                    log_success "$app application"
                else
                    # Check if it at least starts without crashing
                    if timeout 5 "$USERLANDVM_BIN" "$app_path" 2>/dev/null >/dev/null; then
                        log_success "$app application (started successfully)"
                    else
                        log_fail "$app application"
                    fi
                fi
            fi
        else
            log_skip "$app (not found in sysroot)"
        fi
        show_progress $((++current)) $test_count "Haiku apps"
    done
}

# Test system calls functionality
test_syscalls() {
    log_test "System Calls Tests"
    
    # Create a simple syscall test
    cat > tests/syscall_test.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main() {
    // Test basic syscalls
    printf("Testing getpid(): %d\\n", getpid());
    printf("Testing getuid(): %d\\n", getuid());
    
    // Test file syscalls
    struct stat st;
    if (stat("/tmp", &st) == 0) {
        printf("PASS: stat() syscall working\\n");
    } else {
        printf("FAIL: stat() syscall failed\\n");
        return 1;
    }
    
    // Test memory allocation
    void *ptr = malloc(1024);
    if (ptr) {
        printf("PASS: malloc() syscall working\\n");
        free(ptr);
    } else {
        printf("FAIL: malloc() syscall failed\\n");
        return 1;
    }
    
    return 0;
}
EOF
    
    gcc tests/syscall_test.c -o tests/binaries/syscall_test 2>/dev/null
    if [ -f "tests/binaries/syscall_test" ]; then
        echo "Testing system calls..."
        if timeout $TEST_TIMEOUT "$USERLANDVM_BIN" tests/binaries/syscall_test 2>/dev/null; then
            log_success "System calls test"
        else
            log_fail "System calls test"
        fi
    else
        log_skip "System calls test (compilation failed)"
    fi
}

# Test performance
test_performance() {
    log_test "Performance Tests"
    
    if [ ! -f "tests/binaries/hello" ]; then
        log_skip "Performance test (test binary not available)"
        return
    fi
    
    echo "Testing performance..."
    
    # Measure startup time
    local start_time=$(date +%s%N)
    timeout $TEST_TIMEOUT "$USERLANDVM_BIN" tests/binaries/hello >/dev/null 2>&1
    local end_time=$(date +%s%N)
    local duration=$(( (end_time - start_time) / 1000000 ))  # Convert to milliseconds
    
    if [ $duration -lt 5000 ]; then  # Less than 5 seconds
        log_success "Performance test (${duration}ms startup)"
    elif [ $duration -lt 10000 ]; then  # Less than 10 seconds
        log_success "Performance test (${duration}ms startup, acceptable)"
    else
        log_fail "Performance test (${duration}ms startup, too slow)"
    fi
}

# Test multi-architecture support
test_architectures() {
    log_test "Architecture Support Tests"
    
    local arch_count=0
    local current=0
    
    # Test x86-32 support
    if ls arch/x86_32/meson.build >/dev/null 2>&1; then
        log_success "x86-32 support available"
        ((arch_count++))
    else
        log_fail "x86-32 support missing"
    fi
    show_progress $((++current)) $arch_count "Architecture support"
    
    # Test RISC-V support
    if ls arch/riscv/meson.build >/dev/null 2>&1; then
        log_success "RISC-V support available"
        ((arch_count++))
    else
        log_fail "RISC-V support missing"
    fi
    show_progress $((++current)) $arch_count "Architecture support"
    
    # Test ARM support
    if ls arch/arm/meson.build >/dev/null 2>&1; then
        log_success "ARM support available"
        ((arch_count++))
    else
        log_skip "ARM support (not implemented yet)"
    fi
    show_progress $((++current)) $arch_count "Architecture support"
}

# Generate test report
generate_report() {
    echo ""
    echo "UserlandVM-HIT Test Report"
    echo "==========================="
    echo "Total Tests: $TOTAL_TESTS"
    echo "Passed: $PASSED_TESTS"
    echo "Failed: $FAILED_TESTS"
    echo "Skipped: $SKIPPED_TESTS"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo ""
        log_success "All critical tests passed!"
        
        if [ $PASSED_TESTS -ge $((TOTAL_TESTS * 80 / 100)) ]; then
            log_success "Test suite: EXCELLENT (>80% pass rate)"
        else
            log_info "Test suite: GOOD (${PASSED_TESTS}/${TOTAL_TESTS} pass rate)"
        fi
    else
        echo ""
        log_fail "Some tests failed. Check the output above for details."
        
        if [ $PASSED_TESTS -ge $((TOTAL_TESTS * 60 / 100)) ]; then
            log_info "Test suite: ACCEPTABLE (>60% pass rate)"
        else
            log_fail "Test suite: NEEDS IMPROVEMENT (<60% pass rate)"
        fi
    fi
    
    # Exit with appropriate code
    if [ $FAILED_TESTS -gt 0 ]; then
        exit 1
    else
        exit 0
    fi
}

# Cleanup function
cleanup() {
    log_info "Cleaning up test files..."
    rm -rf tests/binaries
    rm -f tests/*.c
    log_info "Cleanup completed"
}

# Main execution
main() {
    echo "UserlandVM-HIT - Comprehensive Testing Suite"
    echo "=========================================="
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --verbose)
                VERBOSE_TESTS=true
                shift
                ;;
            --skip-gui)
                SKIP_GUI_TESTS=true
                shift
                ;;
            --timeout=*)
                TEST_TIMEOUT="${1#*=}"
                shift
                ;;
            --sysroot=*)
                SYSROOT_PATH="${1#*=}"
                shift
                ;;
            --binary=*)
                USERLANDVM_BIN="${1#*=}"
                shift
                ;;
            --cleanup-only)
                cleanup
                exit 0
                ;;
            --help)
                echo "Usage: $0 [options]"
                echo "Options:"
                echo "  --verbose          Enable verbose output"
                echo "  --skip-gui         Skip GUI tests"
                echo "  --timeout=N        Set timeout per test (default: 30s)"
                echo "  --sysroot=PATH     Set sysroot path (default: sysroot/haiku32)"
                echo "  --binary=PATH       Set UserlandVM binary path"
                echo "  --cleanup-only     Clean up test files only"
                echo "  --help             Show this help"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    check_environment
    create_test_binaries
    test_basic_functionality
    test_haiku_applications
    test_syscalls
    test_performance
    test_architectures
    generate_report
}

# Handle cleanup on exit
trap cleanup EXIT

# Run main function
main "$@"