#!/bin/bash
# Simple WebPositive test runner
# Usage: ./run_webpositive.sh [--debug]

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$REPO_DIR/build.x86_64"
VM_BINARY="$BUILD_DIR/UserlandVM"
SYSROOT="$REPO_DIR/sysroot/haiku32"

QUIET_MODE="-q"
if [ "$1" == "--debug" ] || [ "$1" == "-d" ]; then
    QUIET_MODE=""
    echo "[DEBUG MODE] Showing all output"
fi

echo "Running WebPositive with 10-second timeout..."
echo "Process will show as 'webpositive-userlandvm' in 'ps'"
echo "=============================================

"

# Run with timeout
timeout 10 "$VM_BINARY" $QUIET_MODE "$SYSROOT/bin/webpositive" 2>&1 | tail -20

EXIT_CODE=$?
echo ""
echo "============================================="
echo ""

if [ $EXIT_CODE -eq 124 ]; then
    echo "Status: Timeout reached (10 seconds)"
elif [ $EXIT_CODE -eq 0 ]; then
    echo "Status: Program completed"
else
    echo "Status: Exit code $EXIT_CODE"
fi
