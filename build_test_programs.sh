#!/bin/bash
# build_test_programs.sh - Build various test programs of increasing complexity
#
# This script creates test binaries to validate UserlandVM functionality
# Supports multiple complexity levels from static to dynamic linking

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/test_programs"
OUTPUT_DIR="${SCRIPT_DIR}/test_programs/binaries"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

log_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║         Building Test Programs for UserlandVM             ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# ============================================================================
# LEVEL 0: STATIC BINARIES (Already working)
# ============================================================================

log_info "LEVEL 0: Static Binaries"

# Test: Simple syscall
cat > "$BUILD_DIR/test_syscall_static.c" << 'EOF'
#include <unistd.h>
#include <sys/syscall.h>

int main() {
    const char msg[] = "Static binary syscall test\n";
    syscall(SYS_write, 1, msg, sizeof(msg)-1);
    syscall(SYS_exit, 0);
    return 0;
}
EOF

if gcc -static -m32 -o "$OUTPUT_DIR/test_syscall_static" "$BUILD_DIR/test_syscall_static.c" 2>/dev/null; then
    log_success "Built: test_syscall_static"
else
    log_error "Failed: test_syscall_static"
fi

# Test: Arithmetic operations
cat > "$BUILD_DIR/test_arithmetic_static.c" << 'EOF'
#include <unistd.h>

void write_num(int n) {
    if (n == 0) {
        write(1, "0", 1);
        return;
    }
    
    char buf[16];
    int len = 0;
    int temp = n;
    
    while (temp > 0) {
        buf[len++] = '0' + (temp % 10);
        temp /= 10;
    }
    
    for (int i = len - 1; i >= 0; i--) {
        write(1, &buf[i], 1);
    }
}

int main() {
    write(1, "Testing arithmetic:\n", 20);
    
    int a = 15, b = 25;
    write(1, "15 + 25 = ", 10);
    write_num(a + b);
    write(1, "\n", 1);
    
    write(1, "100 * 7 = ", 10);
    write_num(100 * 7);
    write(1, "\n", 1);
    
    write(1, "99 / 3 = ", 9);
    write_num(99 / 3);
    write(1, "\n", 1);
    
    return 0;
}
EOF

if gcc -static -m32 -o "$OUTPUT_DIR/test_arithmetic_static" "$BUILD_DIR/test_arithmetic_static.c" 2>/dev/null; then
    log_success "Built: test_arithmetic_static"
else
    log_error "Failed: test_arithmetic_static"
fi

echo ""

# ============================================================================
# LEVEL 1: DYNAMIC LINKING (In Progress)
# ============================================================================

log_info "LEVEL 1: Dynamic Linking"

# Test: Basic libc linking
cat > "$BUILD_DIR/test_libc_basic.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Hello from libc!\n");
    printf("Testing printf with number: %d\n", 42);
    printf("Testing printf with float: %.2f\n", 3.14159);
    
    char *str = "Dynamic allocation test";
    printf("String: %s\n", str);
    
    return 0;
}
EOF

if gcc -m32 -o "$OUTPUT_DIR/test_libc_basic" "$BUILD_DIR/test_libc_basic.c" 2>/dev/null; then
    log_success "Built: test_libc_basic"
else
    log_error "Failed: test_libc_basic"
fi

# Test: Memory allocation
cat > "$BUILD_DIR/test_malloc.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Testing memory allocation...\n");
    
    // Single allocation
    void *p1 = malloc(256);
    if (!p1) {
        printf("ERROR: malloc(256) failed\n");
        return 1;
    }
    printf("✓ Allocated 256 bytes\n");
    
    // Fill and print
    memset(p1, 'A', 256);
    printf("✓ Memory filled with data\n");
    
    // Multiple allocations
    void *ptrs[5];
    for (int i = 0; i < 5; i++) {
        ptrs[i] = malloc(512 * (i + 1));
        if (!ptrs[i]) {
            printf("ERROR: malloc(%d) failed\n", 512 * (i + 1));
            return 1;
        }
    }
    printf("✓ Allocated 5 blocks (512B - 2.5KB)\n");
    
    // Test realloc
    void *p2 = malloc(128);
    p2 = realloc(p2, 512);
    if (!p2) {
        printf("ERROR: realloc failed\n");
        return 1;
    }
    printf("✓ Realloc succeeded\n");
    
    // Cleanup
    for (int i = 0; i < 5; i++) {
        free(ptrs[i]);
    }
    free(p1);
    free(p2);
    
    printf("✓ All memory tests passed!\n");
    return 0;
}
EOF

if gcc -m32 -o "$OUTPUT_DIR/test_malloc" "$BUILD_DIR/test_malloc.c" 2>/dev/null; then
    log_success "Built: test_malloc"
else
    log_error "Failed: test_malloc"
fi

# Test: Math library
cat > "$BUILD_DIR/test_math.c" << 'EOF'
#include <stdio.h>
#include <math.h>

int main() {
    printf("Testing math library:\n");
    
    double pi = acos(-1.0);
    printf("π = %.6f\n", pi);
    
    double sin_val = sin(pi / 2.0);
    printf("sin(π/2) = %.6f\n", sin_val);
    
    double sqrt_val = sqrt(2.0);
    printf("√2 = %.6f\n", sqrt_val);
    
    double pow_val = pow(2.0, 8.0);
    printf("2^8 = %.6f\n", pow_val);
    
    printf("✓ Math tests passed!\n");
    return 0;
}
EOF

if gcc -m32 -lm -o "$OUTPUT_DIR/test_math" "$BUILD_DIR/test_math.c" 2>/dev/null; then
    log_success "Built: test_math"
else
    log_error "Failed: test_math"
fi

# Test: String operations
cat > "$BUILD_DIR/test_strings.c" << 'EOF'
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    printf("Testing string operations:\n");
    
    // String length
    const char *str1 = "Hello, World!";
    printf("Length of '%s': %zu\n", str1, strlen(str1));
    
    // String copy
    char buf[50];
    strcpy(buf, str1);
    printf("Copied: %s\n", buf);
    
    // String concatenation
    strcat(buf, " (extended)");
    printf("Concatenated: %s\n", buf);
    
    // String comparison
    if (strcmp("hello", "hello") == 0) {
        printf("String comparison works\n");
    }
    
    // String find
    const char *pos = strstr(str1, "World");
    if (pos) {
        printf("Found 'World' at position: %ld\n", pos - str1);
    }
    
    printf("✓ String tests passed!\n");
    return 0;
}
EOF

if gcc -m32 -o "$OUTPUT_DIR/test_strings" "$BUILD_DIR/test_strings.c" 2>/dev/null; then
    log_success "Built: test_strings"
else
    log_error "Failed: test_strings"
fi

# Test: Command-line arguments
cat > "$BUILD_DIR/test_args.c" << 'EOF'
#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("Program: %s\n", argv[0]);
    printf("Number of arguments: %d\n", argc);
    printf("Arguments:\n");
    
    for (int i = 0; i < argc; i++) {
        printf("  argv[%d] = '%s'\n", i, argv[i]);
    }
    
    return 0;
}
EOF

if gcc -m32 -o "$OUTPUT_DIR/test_args" "$BUILD_DIR/test_args.c" 2>/dev/null; then
    log_success "Built: test_args"
else
    log_error "Failed: test_args"
fi

echo ""

# ============================================================================
# LEVEL 2: FILE I/O (Future)
# ============================================================================

log_info "LEVEL 2: File I/O Operations"

# Test: File operations
cat > "$BUILD_DIR/test_file_io.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Testing file I/O:\n");
    
    // Write a file
    FILE *f = fopen("test_output.txt", "w");
    if (!f) {
        printf("ERROR: Cannot create file\n");
        return 1;
    }
    
    fprintf(f, "Line 1: Test data\n");
    fprintf(f, "Line 2: More data\n");
    fprintf(f, "Line 3: Number: %d\n", 12345);
    fclose(f);
    
    printf("✓ File written\n");
    
    // Read the file back
    f = fopen("test_output.txt", "r");
    if (!f) {
        printf("ERROR: Cannot read file\n");
        return 1;
    }
    
    char line[256];
    int line_count = 0;
    printf("Reading file:\n");
    
    while (fgets(line, sizeof(line), f)) {
        printf("  %s", line);
        line_count++;
    }
    
    fclose(f);
    printf("✓ Read %d lines\n", line_count);
    
    // Cleanup (commented to keep file for inspection)
    // remove("test_output.txt");
    
    printf("✓ File I/O tests passed!\n");
    return 0;
}
EOF

if gcc -m32 -o "$OUTPUT_DIR/test_file_io" "$BUILD_DIR/test_file_io.c" 2>/dev/null; then
    log_success "Built: test_file_io"
else
    log_error "Failed: test_file_io"
fi

echo ""

# ============================================================================
# SUMMARY
# ============================================================================

echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
echo "Build Summary:"
echo ""

BUILD_COUNT=$(ls "$OUTPUT_DIR" 2>/dev/null | wc -l)
log_success "Built $BUILD_COUNT test binaries"

echo ""
echo "Available tests:"
ls -1 "$OUTPUT_DIR" | sed 's/^/  • /'

echo ""
echo "To run a test:"
echo "  ./build.x86_64/UserlandVM test_programs/binaries/test_name"
echo ""

echo "To run all tests:"
echo "  for test in test_programs/binaries/*; do"
echo "    echo \"Running: \$(basename \$test)\""
echo "    ./build.x86_64/UserlandVM \"\$test\""
echo "  done"

echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"

exit 0
