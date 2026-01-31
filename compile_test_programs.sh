#!/bin/bash
# Compile simple test programs for x86-32 (static)
# These will test various features

OUT_DIR="./test_programs/binaries"
mkdir -p "$OUT_DIR"

# Use native gcc with -m32 flag
GCC="gcc -m32"

if ! $GCC --version >/dev/null 2>&1; then
    echo "❌ gcc -m32 not available"
    echo "System gcc: $(gcc --version | head -1)"
    exit 1
fi

echo "🔨 Compiling test programs for x86-32 (Haiku)"
echo "=============================================="
echo ""

# Test 1: Simple hello
cat > /tmp/test_hello.c << 'EOF'
#include <unistd.h>

const char msg[] = "Hello from test!\n";

int main() {
    write(1, msg, sizeof(msg)-1);
    return 0;
}
EOF

echo -n "Compiling hello ... "
i586-pc-haiku-gcc -m32 -static -nostdlib -o "$OUT_DIR/test_hello" /tmp/test_hello.c 2>/dev/null && echo "✅" || echo "❌"

# Test 2: Arithmetic
cat > /tmp/test_add.c << 'EOF'
#include <unistd.h>

int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    char buf[2] = {'0' + result, '\n'};
    write(1, buf, 2);
    return 0;
}
EOF

echo -n "Compiling arithmetic ... "
i586-pc-haiku-gcc -m32 -static -nostdlib -o "$OUT_DIR/test_add" /tmp/test_add.c 2>/dev/null && echo "✅" || echo "❌"

# Test 3: Multiple syscalls
cat > /tmp/test_multi.c << 'EOF'
#include <unistd.h>

int main() {
    write(1, "A", 1);
    write(1, "B", 1);
    write(1, "C", 1);
    write(1, "\n", 1);
    return 0;
}
EOF

echo -n "Compiling multi-syscall ... "
i586-pc-haiku-gcc -m32 -static -nostdlib -o "$OUT_DIR/test_multi" /tmp/test_multi.c 2>/dev/null && echo "✅" || echo "❌"

echo ""
echo "Compiled binaries:"
ls -lh "$OUT_DIR"/ 2>/dev/null | grep -v "^total" | awk '{print "  " $9 " (" $5 ")"}'
