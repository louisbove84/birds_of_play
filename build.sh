#!/bin/bash

# Build script for Clang compiler (standard build)
set -e

echo "Building with Clang compiler..."

# Set compiler environment variables
export CC=clang
export CXX=clang++

# Create build directory
BUILD_DIR="build"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_FLAGS="-Wall -Wextra -O2" \
    -DBUILD_TESTING=ON

# Build (use sysctl for macOS, nproc for Linux)
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    make -j$(sysctl -n hw.ncpu)
else
    # Linux
    make -j$(nproc)
fi

echo "Clang build completed successfully!"
echo "Build directory: $BUILD_DIR"
