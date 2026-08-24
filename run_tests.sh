#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
COMBUST="${BUILD_DIR}/combust"

if [ ! -f "$COMBUST" ]; then
    echo "Error: combust binary not found at $COMBUST. Please build the project first."
    exit 1
fi

echo "==================================================="
echo "  Sulfur++ Automated Test Suite"
echo "==================================================="
echo ""

PASSED=0
FAILED=0
TMP_OUT="/tmp/sulfur_test_out.txt"

for test_file in "${SCRIPT_DIR}/tests/"*.sfpp; do
    if [ -f "$test_file" ]; then
        test_name="$(basename "$test_file")"
        printf "[RUNNING] %s... " "$test_name"
        if "$COMBUST" "$test_file" > "$TMP_OUT" 2>&1; then
            echo "[PASS]"
            PASSED=$((PASSED + 1))
        else
            echo "[FAIL]"
            cat "$TMP_OUT"
            FAILED=$((FAILED + 1))
        fi
        echo "---------------------------------------------------"
    fi
done

echo ""
echo "Test Summary:"
echo "  Passed: ${PASSED}"
echo "  Failed: ${FAILED}"
echo "==================================================="

if [ $FAILED -ne 0 ]; then
    exit 1
fi
exit 0
