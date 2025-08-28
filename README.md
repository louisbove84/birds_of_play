# Birds of Play: Motion Detection System

A real-time motion detection and region consolidation system built with C++ and OpenCV.

## 🎯 Features

- **Real-time motion detection** using frame differencing and background subtraction
- **Region consolidation** to group nearby motion areas
- **Live webcam processing** with visual feedback
- **Comprehensive test suite** with integration testing
- **Modular architecture** for easy extension

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

# Run tests
cd motion_detection
./motion_processor_test
./motion_region_consolidator_test
./integration_test
```

## 📁 Project Structure

```
birds_of_play/
├── main.cpp                    # Main webcam application
├── motion_detection/           # Core motion detection library
│   ├── src/                   # Source files
│   │   ├── motion_processor.cpp
│   │   ├── motion_region_consolidator.cpp
│   │   └── motion_pipeline.cpp
│   ├── include/               # Header files
│   ├── tests/                 # Test suite
│   └── config.yaml           # Configuration file
└── CMakeLists.txt            # Build configuration
```

## 🔧 Configuration

Edit `motion_detection/config.yaml` to customize:

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
# Run all tests
cd build/motion_detection
make test

# Run specific test suites
./motion_processor_test           # Motion detection tests
./motion_region_consolidator_test # Region consolidation tests
./integration_test               # End-to-end pipeline tests
```

Test results and visualizations are saved in `test_results/` folders.

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
#include "motion_detection/include/motion_pipeline.hpp"

// Initialize components
MotionProcessor motionProcessor("config.yaml");
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

### Adding New Features

1. Add source files to `motion_detection/src/`
2. Add headers to `motion_detection/include/`
3. Update `motion_detection/CMakeLists.txt`
4. Add tests in `motion_detection/tests/`

### Code Style

- Follow existing naming conventions
- Use meaningful variable names
- Add logging for important operations
- Include tests for new functionality

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