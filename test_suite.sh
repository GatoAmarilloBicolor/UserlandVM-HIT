#!/bin/bash
echo "========== USERLANDVM TEST SUITE =========="
echo "Date: $(date)"
echo "Binary: $(ls -lh builddir/UserlandVM | awk '{print $5, $NF}')"
echo ""

PASS=0
FAIL=0

# Test 1: Static x86-32
echo "Test 1: Static x86-32 (TestX86)"
timeout 10 ./builddir/UserlandVM ./TestX86 >/dev/null 2>&1
if [ $? -eq 10 ]; then
  echo "✅ PASS (exit 10)"
  ((PASS++))
else
  echo "❌ FAIL"
  ((FAIL++))
fi

# Test 2: x86-64 rejection
echo ""
echo "Test 2: x86-64 architecture rejection"
timeout 5 ./builddir/UserlandVM /bin/ls >/dev/null 2>&1
if [ $? -eq 43 ]; then
  echo "✅ PASS (exit 43 - not supported)"
  ((PASS++))
else
  echo "❌ FAIL"
  ((FAIL++))
fi

# Test 3: Reproducibility (5 runs)
echo ""
echo "Test 3: Reproducibility (5 runs)"
for i in {1..5}; do
  timeout 10 ./builddir/UserlandVM ./TestX86 >/dev/null 2>&1
  CODE=$?
  if [ $CODE -eq 10 ]; then
    echo "  Run $i: ✅"
  else
    echo "  Run $i: ❌ (exit $CODE)"
  fi
done

# Test 4: Performance
echo ""
echo "Test 4: Performance benchmark"
TIME=$( { time timeout 10 ./builddir/UserlandVM ./TestX86 >/dev/null 2>&1; } 2>&1 | grep real | awk '{print $2}')
echo "  Execution time: $TIME"

# Test 5: Binary integrity
echo ""
echo "Test 5: Binary integrity"
if file builddir/UserlandVM | grep -q "ELF 64-bit"; then
  echo "✅ PASS (ELF 64-bit binary)"
  ((PASS++))
else
  echo "❌ FAIL"
  ((FAIL++))
fi

# Summary
echo ""
echo "========== TEST SUMMARY =========="
echo "Passed:  $PASS"
echo "Failed:  $FAIL"
echo "Total:   $((PASS + FAIL))"
