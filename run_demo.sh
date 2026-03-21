#!/usr/bin/env bash
set -euo pipefail

# Quick wrapper for building and running the cabin_drowsiness_demo project.
# Usage:
#   ./run_demo.sh [camera-index] [face-cascade] [eye-cascade]
# Default cascade paths are auto-detected by the app.

CAMERA_INDEX=${1:-0}
FACE_CASCADE=${2:-}
EYE_CASCADE=${3:-}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "==> Building project"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..
cmake --build .

CMD="$BUILD_DIR/cabin_drowsiness_demo --camera $CAMERA_INDEX"
[ -n "$FACE_CASCADE" ] && CMD+=" --face-cascade $FACE_CASCADE"
[ -n "$EYE_CASCADE" ] && CMD+=" --eye-cascade $EYE_CASCADE"

echo "==> Running: $CMD"
exec bash -c "$CMD"