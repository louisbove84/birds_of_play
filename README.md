# Birds of Play: Motion Detection System

A real-time motion detection and region consolidation system built with C++ and OpenCV.

## 🎯 Features

- **Real-time motion detection** using frame differencing and background subtraction
- **Region consolidation** to group nearby motion areas
- **Live webcam processing** with visual feedback
- **Comprehensive test suite** with integration testing
- **Modular architecture** for easy extension
- **Scalable src/ directory structure** for multi-component development

## 🚀 Quick Start

### Prerequisites

**macOS (Apple Silicon):**
```bash
brew install cmake opencv yaml-cpp
```

**Ubuntu/Debian:**
```bash
sudo apt install build-essential cmake libopencv-dev libyaml-cpp-dev
```

### Build and Run

```bash
# Clone the repository
git clone <your-repo-url>
cd birds_of_play

# Build the project
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON
make -j8

# Run the main webcam demo
./birds_of_play

# Run tests (from build directory)
make test-motion-processor
make test-motion-consolidator
make test-integration
```

## 📁 Project Structure

```
birds_of_play/
├── src/                        # Source code directory
│   ├── main.cpp               # Main webcam application
│   └── motion_detection/      # Core motion detection component
│       ├── src/               # Source files
│       │   ├── motion_processor.cpp
│       │   ├── motion_region_consolidator.cpp
│       │   ├── motion_pipeline.cpp
│       │   └── motion_visualization.cpp
│       ├── include/           # Header files
│       ├── tests/             # Test suite
│       ├── libs/              # Dependencies (spdlog)
│       ├── CMakeLists.txt     # Component build configuration
│       └── config.yaml        # Component configuration
├── build/                     # Build output directory
├── CMakeLists.txt            # Root build configuration
└── README.md                 # This file
```

## 🔧 Configuration

Edit `src/motion_detection/config.yaml` to customize:

```yaml
motion_detection:
  min_contour_area: 50          # Minimum area for motion detection
  background_subtraction: false  # Enable background subtraction
  gaussian_blur_size: 5         # Blur kernel size
  threshold_value: 30           # Motion detection threshold

region_consolidation:
  max_distance_threshold: 50.0  # Max distance for grouping regions
  objectDetectionModelMinInputSize: 320  # Min input size for object detection
  objectDetectionModelMaxInputSize: 640  # Max input size for object detection
```

## 🧪 Testing

The project includes comprehensive tests:

- **Unit Tests**: Test individual components
- **Integration Tests**: Test the complete pipeline with real images
- **Visual Output**: All tests generate visualization images for inspection

```bash
# Run all tests (from build directory)
make test-all

# Run specific test suites
make test-motion-processor           # Motion detection tests
make test-motion-consolidator        # Region consolidation tests
make test-integration               # End-to-end pipeline tests

# Clean rebuild and test
make clean-rebuild
make run-all-tests
```

Test results and visualizations are saved in `test_results/` folders within the build directory.

## 🎮 Usage

### Webcam Demo

Run the main application to see live motion detection:

```bash
./birds_of_play
```

**Controls:**
- `q` or `ESC`: Quit
- `s`: Save current frame

### Library Usage

```cpp
#include "src/motion_detection/include/motion_pipeline.hpp"

// Initialize components
MotionProcessor motionProcessor("src/motion_detection/config.yaml");
MotionRegionConsolidator regionConsolidator;

// Process frame
cv::Mat frame = // ... get your frame
auto [result, regions] = processFrameAndConsolidate(
    motionProcessor, regionConsolidator, frame, "output.jpg");

// Use results
std::cout << "Detected " << result.detectedBounds.size() << " motion areas" << std::endl;
std::cout << "Consolidated to " << regions.size() << " regions" << std::endl;
```

## 📊 Performance

- **Processing Speed**: ~3-5ms per frame (1920x1080)
- **Memory Usage**: ~40-50MB typical
- **Supported Formats**: Any OpenCV-compatible image/video format
- **Platform Support**: macOS, Linux (Windows untested)

## 🛠️ Development

### Project Architecture

The project uses a scalable `src/` directory structure designed for multi-component development:

```
src/
├── motion_detection/          # Current motion detection component
├── image_classification/      # Future: YOLO, ResNet, etc.
├── object_tracking/           # Future: Kalman filters, etc.
├── audio_processing/          # Future: Bird song detection
└── main.cpp                   # Orchestrates all components
```

### Adding New Components

1. Create component directory: `mkdir -p src/new_component/{include,src,tests}`
2. Add component to root `CMakeLists.txt`: `add_subdirectory(src/new_component)`
3. Create component `CMakeLists.txt` with library and test targets
4. Link in `main.cpp` and other components as needed

### Adding Features to Existing Components

1. Add source files to `src/motion_detection/src/`
2. Add headers to `src/motion_detection/include/`
3. Update `src/motion_detection/CMakeLists.txt`
4. Add tests in `src/motion_detection/tests/`

### Code Style

- Follow existing naming conventions
- Use meaningful variable names
- Add logging for important operations
- Include tests for new functionality
- Use the shared `motion_pipeline.hpp` for common functionality

## 🔧 Build System

### Custom Make Targets

```bash
# Build targets
make clean-rebuild          # Clean and rebuild everything
make fresh-build           # Fresh CMake configuration and build
make run-all-tests         # Build and run all tests

# Test targets
make test-all              # Run all test suites
make test-motion-processor # Motion processor tests only
make test-motion-consolidator # Region consolidator tests only
make test-integration      # Integration tests only
```

### Debug Build

```bash
mkdir build_debug && cd build_debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j8
```

## 📄 License

MIT License - see LICENSE.md for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

---

*Built with ❤️ for real-time motion detection*