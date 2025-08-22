#!/bin/bash

# Debug build script for Clang compiler
set -e

echo "Building debug version with Clang compiler..."

# Set compiler environment variables
export CC=clang
export CXX=clang++

# Create build directory
BUILD_DIR="build_debug"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_FLAGS="-Wall -Wextra -g -O0 -fno-omit-frame-pointer" \
    -DBUILD_TESTING=ON

# Build (use sysctl for macOS, nproc for Linux)
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    make -j$(sysctl -n hw.ncpu)
else
    # Linux
    make -j$(nproc)
fi

echo "Clang debug build completed successfully!"
echo "Build directory: $BUILD_DIR"
