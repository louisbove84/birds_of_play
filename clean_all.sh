#!/bin/bash

# Clean all build directories
set -e

echo "Cleaning all build directories..."

# Remove all build directories
rm -rf build
rm -rf build_debug

# Remove any other build directories that might exist
find . -maxdepth 2 -name "build_*" -type d -exec rm -rf {} \; 2>/dev/null || true

echo "All build directories cleaned!"
