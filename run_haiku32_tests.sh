#!/bin/bash

# Master test runner for Haiku32 with timeouts
# Runs quick and comprehensive tests

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -f "./build.x86_64/UserlandVM" ]; then
	echo "Error: UserlandVM binary not found at ./build.x86_64/UserlandVM"
	echo "Please build the project first with: meson compile -C build.x86_64"
	exit 1
fi

if [ ! -d "./sysroot/haiku32" ]; then
	echo "Error: sysroot/haiku32 not found"
	echo "Please set up the Haiku32 sysroot first"
	exit 1
fi

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  UserlandVM - Haiku32 Test Runner                           ║"
echo "║  Timeout: 5 seconds | Output: Last 10 lines                 ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

if [ "$1" = "-q" ] || [ "$1" = "--quick" ]; then
	echo "Running QUICK test (webpositive + common commands)..."
	echo ""
	bash ./test_webpositive_5sec.sh
elif [ "$1" = "-f" ] || [ "$1" = "--full" ]; then
	echo "Running FULL test (all available commands)..."
	echo ""
	bash ./test_haiku32_timeout_5sec_tail10.sh
else
	echo "Available test modes:"
	echo "  ./run_haiku32_tests.sh -q   Quick test (webpositive + basics)"
	echo "  ./run_haiku32_tests.sh -f   Full test (all commands)"
	echo ""
	echo "Running quick test by default..."
	echo ""
	bash ./test_webpositive_5sec.sh
fi

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Test run completed                                         ║"
echo "╚══════════════════════════════════════════════════════════════╝"
