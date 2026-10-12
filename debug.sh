#!/bin/bash
# Debug script for multidisplay-unified
# Usage: ./debug.sh [--test|--bridge /dev/ttyUSB3]
# Runs the app under gdb and prints backtrace on crash

set -e

APP_DIR="$(dirname "$0")"
APP="$APP_DIR/multidisplay-unified"

if [ ! -f "$APP" ]; then
    echo "Error: $APP not found. Run cmake && make first."
    exit 1
fi

# Build arguments
ARGS=""
for arg in "$@"; do
    ARGS="$ARGS $arg"
done

echo "========================================="
echo "Debugging multidisplay-unified"
echo "========================================="
echo "Command: QT_QPA_PLATFORM=xcb gdb -batch -ex 'run --test' -ex 'bt full' $APP"
echo "========================================="

QT_QPA_PLATFORM=xcb gdb -batch \
    -ex "set pagination off" \
    -ex "handle SIGSEGV nostop noprint pass" \
    -ex "run$ARGS" \
    -ex "bt full" \
    "$APP" 2>&1 | tail -100
