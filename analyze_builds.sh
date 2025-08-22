#!/bin/bash

# Clang build analysis script
set -e

echo "Analyzing Clang builds..."

# Check if builds exist
if [ ! -d "build" ]; then
    echo "Release build not found. Please run ./build.sh first."
    exit 1
fi

if [ ! -d "build_debug" ]; then
    echo "Debug build not found. Please run ./build_debug.sh first."
    exit 1
fi

echo "=== Compiler Information ==="
echo "Clang version:"
clang --version | head -1

echo -e "\n=== Build Size Comparison ==="
echo "Release build sizes:"
find build -name "*.o" -exec ls -lh {} \; | wc -l
find build -name "BirdsOfPlay" -exec ls -lh {} \;
find build -name "*_test" -exec ls -lh {} \;

echo -e "\nDebug build sizes:"
find build_debug -name "*.o" -exec ls -lh {} \; | wc -l
find build_debug -name "BirdsOfPlay" -exec ls -lh {} \;
find build_debug -name "*_test" -exec ls -lh {} \;

echo -e "\n=== Performance Comparison ==="
echo "Running performance tests..."

# Run performance tests if they exist
if [ -f "build/motion_detection/BirdsOfPlay" ] && [ -f "build_debug/motion_detection/BirdsOfPlay" ]; then
    echo "Release performance:"
    time ./build/motion_detection/BirdsOfPlay --help > /dev/null 2>&1 || true
    
    echo -e "\nDebug performance:"
    time ./build_debug/motion_detection/BirdsOfPlay --help > /dev/null 2>&1 || true
fi

echo -e "\n=== Test Results Comparison ==="
echo "Release test results:"
cd build
ctest --output-on-failure || true
cd ..

echo -e "\nDebug test results:"
cd build_debug
ctest --output-on-failure || true
cd ..

echo -e "\nAnalysis completed!"
