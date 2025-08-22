# Build System

This project uses Clang as the standard compiler and provides several build scripts for different purposes.

## Quick Start

### Standard Release Build
```bash
./build.sh
```

### Debug Build
```bash
./build_debug.sh
```

### Run Tests
```bash
./run_tests.sh
```

### Analyze Builds
```bash
./analyze_builds.sh
```

### Clean All Builds
```bash
./clean_all.sh
```

## Build Scripts

- **`build.sh`** - Standard release build with Clang
- **`build_debug.sh`** - Debug build with Clang (includes debug symbols)
- **`run_tests.sh`** - Run all tests from the release build
- **`analyze_builds.sh`** - Compare release vs debug builds
- **`clean_all.sh`** - Clean all build directories

## Build Directories

- **`build/`** - Release build output
- **`build_debug/`** - Debug build output

## Compiler Configuration

The project is optimized for Clang and includes:
- Modern C++17 standard
- Strict warning flags (`-Wall -Wextra -Wpedantic`)
- Optimized for release builds (`-O2`)
- Debug symbols for debug builds (`-g -O0`)

## Dependencies

Make sure you have the following installed:
- Clang compiler
- CMake (3.14+)
- OpenCV
- yaml-cpp
- MongoDB C++ driver
- GTest (for testing)

