# Build and Test Commands

This project now has convenient build and test commands to streamline development.

## 🚀 Quick Start

**First time setup:**
```bash
./test_runner.sh fresh-test
```

**Daily development:**
```bash
./test_runner.sh test-motion     # Quick motion processor test
./test_runner.sh test-all        # Clean rebuild and test everything
```

## 📋 Available Commands

### Shell Script Commands (Recommended)

The `./test_runner.sh` script provides the most convenient interface:

| Command | Description |
|---------|-------------|
| `fresh-test` | Remove build folder, rebuild everything, and run all tests |
| `fresh-build` | Remove build folder and rebuild everything |
| `test-all` | Clean, rebuild, and run all tests (faster than fresh-test) |
| `test-motion` | Run only motion processor tests |
| `test-consolidator` | Run only motion region consolidator tests |
| `clean-rebuild` | Clean and rebuild without running tests |
| `run-tests` | Run all tests (assumes already built) |
| `clean-all` | Remove entire build directory |

### CMake/Make Commands

From the `build_debug` directory, you can also use:

| Command | Description |
|---------|-------------|
| `make fresh-test` | Complete fresh build and test |
| `make fresh-build` | Complete fresh build only |
| `make test-all` | Clean, rebuild, and test |
| `make test-motion-processor` | Run motion processor tests only |
| `make test-motion-consolidator` | Run motion region consolidator tests only |
| `make clean-rebuild` | Clean and rebuild |
| `make run-all-tests` | Run all tests |
| `make clean-all` | Remove build directory |

## 🔧 Manual Commands

If you prefer manual control:

```bash
# Setup
mkdir build_debug && cd build_debug
cmake .. -DBUILD_TESTING=ON

# Build
make -j$(nproc)  # or make -j$(sysctl -n hw.ncpu) on macOS

# Test
./motion_processor_test --gtest
./motion_region_consolidator_test

# Clean
make clean
```

## 📁 Test Output Structure

All test outputs are organized in `test_results/`:

```
test_results/
├── motion_processor/
│   ├── 01_preprocess_frame/          # Manual test mode
│   ├── 02_detect_motion/
│   ├── 03_morphological_ops/
│   ├── 04_extract_contours/
│   ├── 05_complete_pipeline/
│   └── 06_google_test_mode/          # Google Test mode
│       ├── 01_baseline_frame_processing.jpg
│       ├── 02_motion_detection_processing.jpg
│       ├── 03_frame1_original.jpg
│       └── ... (numbered sequence)
└── motion_region_consolidator/
    └── google_test_mode/
        ├── 01_real_bird_consolidation_visualization.jpg
        ├── 02_synthetic_consolidation.jpg
        └── ... (numbered sequence)
```

## 💡 Development Workflow

1. **Start fresh:** `./test_runner.sh fresh-test`
2. **Quick iteration:** `./test_runner.sh test-motion` or `./test_runner.sh test-consolidator`
3. **Full validation:** `./test_runner.sh test-all`
4. **Clean slate:** `./test_runner.sh clean-all` then `./test_runner.sh fresh-test`

## 🎯 Tips

- Use `fresh-test` when you want to ensure a completely clean build
- Use `test-all` for faster iteration (doesn't remove the entire build directory)
- Use individual test commands (`test-motion`, `test-consolidator`) for focused testing
- All commands include colored output and progress indicators
- The shell script automatically detects the number of CPU cores for parallel builds
