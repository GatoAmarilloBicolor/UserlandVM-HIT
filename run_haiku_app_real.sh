#!/bin/bash

# Run Haiku applications with REAL windows in Haiku desktop
# NOT emulation, NOT ASCII art - REAL Haiku Be API windows

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VM="${SCRIPT_DIR}/userlandvm_real_windows"
SYSROOT="${SCRIPT_DIR}/sysroot/haiku32"

if [ ! -x "$VM" ]; then
    echo "❌ VM not compiled"
    echo "Compile first: g++ -std=c++17 main_windows.cpp BeAPIInterceptor.cpp -o userlandvm_real_windows"
    exit 1
fi

if [ $# -eq 0 ]; then
    echo "Usage: $0 <app_name> [args...]"
    echo ""
    echo "Examples:"
    echo "  $0 webpositive"
    echo ""
    exit 1
fi

APP=$1
shift

# Check if app exists in sysroot
if [ ! -f "${SYSROOT}/bin/${APP}" ]; then
    # Try as full path
    if [ ! -f "$APP" ]; then
        echo "❌ App not found: $APP"
        exit 1
    fi
    APP=$APP
fi

echo "Running $APP with REAL Haiku windows..."
"$VM" "$APP" "$@"
