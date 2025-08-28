# Birds of Play Python Bindings

This document explains how to use the Python bindings for the Birds of Play motion detection library.

## Setup

### Prerequisites

1. **Python 3.8+** with virtual environment support
2. **CMake 3.14+**
3. **C++17 compatible compiler** (Clang/GCC/MSVC)
4. **OpenCV 4.x**
5. **pybind11**

### Installation

1. **Create and activate virtual environment:**
   ```bash
   python3 -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   ```

2. **Install Python dependencies:**
   ```bash
   pip install -e .
   ```

3. **Install development dependencies (optional):**
   ```bash
   pip install -e ".[dev]"
   ```

4. **Build the project:**
   ```bash
   mkdir build && cd build
   cmake .. -DBUILD_TESTING=ON
   make -j$(nproc)  # On Windows: cmake --build . --parallel
   ```

## Project Structure

The project uses modern Python packaging with `pyproject.toml`:

- **`pyproject.toml`**: Project configuration and dependencies
- **`src/motion_detection/src/python_bindings.cpp`**: Python bindings source
- **`build/src/motion_detection/birds_of_play_python.*.so`**: Built Python module
- **`example_python_usage.py`**: C++ motion detection demo launcher

## Usage

### Basic Example

```python
import numpy as np
import cv2
import sys
import os

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build', 'src', 'motion_detection'))

# Import the Python bindings
import birds_of_play_python

# Create a motion processor
processor = birds_of_play_python.MotionProcessor()

# Create a test image
test_image = np.random.randint(0, 255, (480, 640, 3), dtype=np.uint8)

# Process the frame
processed_frame = processor.process_frame(test_image)

print(f"Input shape: {test_image.shape}")
print(f"Output shape: {processed_frame.shape}")
```

### Video Processing Example

```python
import cv2
import birds_of_play_python

# Create motion processor
processor = birds_of_play_python.MotionProcessor()

# Open video file
cap = cv2.VideoCapture('input_video.mp4')

while True:
    ret, frame = cap.read()
    if not ret:
        break
    
    # Process frame
    processed_frame = processor.process_frame(frame)
    
    # Display or save the processed frame
    cv2.imshow('Motion Detection', processed_frame)
    
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
```

### Webcam Real-time Processing

```python
import cv2
import birds_of_play_python

# Create motion processor
processor = birds_of_play_python.MotionProcessor()

# Open webcam
cap = cv2.VideoCapture(0)

while True:
    ret, frame = cap.read()
    if not ret:
        break
    
    # Process frame
    processed_frame = processor.process_frame(frame)
    
    # Display both original and processed
    combined = np.hstack([frame, processed_frame])
    cv2.imshow('Original | Processed', combined)
    
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
```

## API Reference

### MotionProcessor

The main class for motion detection processing.

#### Constructor
```python
MotionProcessor(config_path="")
```
- `config_path`: Path to configuration file (optional, defaults to "config.yaml")

#### Methods

##### `process_frame(input_frame)`
Process a single frame and return the processed result.

- **Parameters:**
  - `input_frame`: numpy array of shape (height, width, 3) with uint8 values
  
- **Returns:**
  - numpy array of processed frame (typically grayscale)

##### `get_detections()`
Get current detections (currently returns empty list - to be implemented).

- **Returns:**
  - List of detection dictionaries

##### `reset()`
Reset the processor state.

##### `get_last_result()`
Get the last processing result (currently returns empty dict - to be implemented).

- **Returns:**
  - Dictionary containing processing results

## Configuration

The motion processor uses a YAML configuration file. If no config file is provided, it will use default settings.

Example configuration:
```yaml
# Motion detection parameters
min_contour_area: 100
max_threshold: 255
background_subtraction: true

# Image preprocessing
contrast_enhancement: true
blur_type: "gaussian"
gaussian_blur_size: 5

# Morphological operations
morphology: true
morph_kernel_size: 3
morph_close: true
```

## Testing

Run the C++ motion detection demo:

```bash
python example_python_usage.py
```

This will launch the full C++ motion detection system with webcam support.

## Troubleshooting

### Common Issues

1. **Import Error:**
   - Make sure you've built the project with `make`
   - Check that the Python path includes the build directory
   - Verify the virtual environment is activated

2. **OpenCV Warnings:**
   - The duplicate class warnings are harmless and can be ignored
   - They occur due to multiple OpenCV installations

3. **Config File Not Found:**
   - The processor will use default settings if config.yaml is not found
   - Create a config.yaml file in the project root for custom settings

4. **Build Errors:**
   - Ensure all dependencies are installed (OpenCV, pybind11, etc.)
   - Check that CMake version is 3.14 or higher
   - Verify C++17 support is available

### Performance Tips

1. **Batch Processing:** Process multiple frames in a loop for better performance
2. **Image Size:** Smaller images process faster
3. **Configuration:** Adjust parameters based on your use case
4. **Memory:** Release video captures and windows when done

## Examples

See `example_python_usage.py` for a complete demonstration including:
- Live webcam motion detection
- Video file processing
- Real-time motion tracking and region consolidation

## Contributing

To extend the Python bindings:

1. Modify `src/motion_detection/src/python_bindings.cpp`
2. Add new methods to the wrapper classes
3. Update the PYBIND11_MODULE section
4. Rebuild the project
5. Update this documentation

## License

This project is licensed under the same terms as the main Birds of Play project.
