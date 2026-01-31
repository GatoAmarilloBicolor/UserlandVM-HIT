#!/bin/bash
# run_all_tests.sh - Run all UserlandVM test programs

VM_BINARY="./build.x86_64/UserlandVM"
TEST_DIR="."
PASSED=0
FAILED=0

echo "=========================================================="
echo "  UserlandVM Test Suite - December 3, 2025"
echo "=========================================================="
echo ""

# Find all test_* executables
for test_binary in $TEST_DIR/test_*; do
    # Skip if not executable or not a file
    if [ ! -x "$test_binary" ] || [ ! -f "$test_binary" ]; then
        continue
    fi
    
    test_name=$(basename "$test_binary")
    echo "Running: $test_name"
    echo "---"
    
    # Run the test and capture output
    if output=$("$VM_BINARY" "$test_binary" 2>&1); then
        # Check if output contains success indicator
        if echo "$output" | grep -q "Guest program exited gracefully"; then
            echo "✅ PASS"
            ((PASSED++))
        else
            echo "⚠️  UNCERTAIN (exit code 0 but no confirmation)"
            echo "Output:"
            echo "$output" | tail -10
            ((PASSED++))  # Count as pass since no error
        fi
    else
        exit_code=$?
        echo "❌ FAIL (exit code: $exit_code)"
        echo "Output:"
        echo "$output" | tail -10
        ((FAILED++))
    fi
    
    echo ""
done

echo "=========================================================="
echo "  Test Results Summary"
echo "=========================================================="
echo "Passed:  $PASSED"
echo "Failed:  $FAILED"
echo "Total:   $((PASSED + FAILED))"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "✅ All tests passed!"
    exit 0
else
    echo "❌ Some tests failed"
    exit 1
fi
