#!/bin/bash

# Test runner for Clang builds
set -e

echo "Running tests for Clang build..."

BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Please run ./build.sh first."
    exit 1
fi

cd $BUILD_DIR

# Run all tests
echo "Running CMake tests..."
ctest --verbose

# Run specific test executables if they exist
if [ -f "motion_detection/BirdsOfPlay_test" ]; then
    echo "Running BirdsOfPlay tests..."
    ./motion_detection/BirdsOfPlay_test
fi

if [ -f "motion_detection/motion_tracker_test" ]; then
    echo "Running motion tracker tests..."
    ./motion_detection/motion_tracker_test
fi

if [ -f "motion_detection/motion_processor_test" ]; then
    echo "Running motion processor tests..."
    ./motion_detection/motion_processor_test
fi

echo "Tests completed!"
