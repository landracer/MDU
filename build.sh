#!/bin/bash
# Build script for multidisplay-unified
# Usage: ./build.sh [debug|release|clean]

set -e

BUILD_TYPE="${1:-release}"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build_$BUILD_TYPE"

echo "============================================"
echo "  MultiDisplay Unified Build Script"
echo "============================================"
echo "Project: $PROJECT_DIR"
echo "Build type: $BUILD_TYPE"
echo "Build dir: $BUILD_DIR"
echo "============================================"

# Clean if requested
if [ "$BUILD_TYPE" = "clean" ]; then
    echo "Cleaning build directories..."
    rm -rf "$PROJECT_DIR/build_debug"
    rm -rf "$PROJECT_DIR/build_release"
    echo "Clean complete."
    exit 0
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Detect platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="linux"
    echo "Platform: Linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
    echo "Platform: macOS"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    PLATFORM="windows"
    echo "Platform: Windows"
else
    PLATFORM="unknown"
    echo "Platform: Unknown ($OSTYPE)"
fi

# Configure with CMake
echo ""
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DENABLE_QWT=ON \
    -G "Unix Makefiles"

if [[ "$PLATFORM" == "windows" ]]; then
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DENABLE_QWT=ON \
        -G "Visual Studio 17 2022"
fi

# Build
echo ""
echo "Building..."
cmake --build . --config "$BUILD_TYPE" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo ""
    echo "============================================"
    echo "  Build successful!"
    echo "============================================"
    echo "Executable: $BUILD_DIR/multidisplay-unified"
    echo "============================================"
    
    # Show binary info
    if [[ "$PLATFORM" == "linux" ]]; then
        ls -lh "$BUILD_DIR/multidisplay-unified"
        file "$BUILD_DIR/multidisplay-unified"
    elif [[ "$PLATFORM" == "windows" ]]; then
        ls -lh "$BUILD_DIR/Release/multidisplay-unified.exe" 2>/dev/null || \
        ls -lh "$BUILD_DIR/multidisplay-unified.exe" 2>/dev/null
    fi
else
    echo ""
    echo "============================================"
    echo "  BUILD FAILED"
    echo "============================================"
    exit 1
fi
