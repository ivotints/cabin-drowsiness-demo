#!/usr/bin/env bash
set -euo pipefail

# Quick wrapper for building and running unit tests for cabin_drowsiness_demo.
# Usage:
#   ./run_tests.sh
# Optional:
#   TEST_FILTER=pattern ./run_tests.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "==> Building project for tests"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..
cmake --build .

echo "==> Running tests"
if [[ -n "${TEST_FILTER:-}" ]]; then
  ctest --output-on-failure -R "$TEST_FILTER"
else
  ctest --output-on-failure
fi
