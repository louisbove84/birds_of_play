# Motion Detection Component

A C++ application for real-time motion tracking, designed for bird detection and tracking.

## Prerequisites

- CMake (version 3.10 or higher)
- OpenCV (version 4.x recommended)
- yaml-cpp (install with `brew install yaml-cpp` on macOS or `sudo apt-get install libyaml-cpp-dev` on Ubuntu)
- C++17 compatible compiler

## Building the Project

1. Create a build directory:
```bash
mkdir build
cd build
```

2. Configure and build the project:
```bash
cmake ..
make
```

## Running the Application

After building, run the application from the build directory:
```bash
./BirdsOfPlay
```

- Press 'ESC' to exit the application
- The application will display a window showing the camera feed with motion detection
- Moving objects will be highlighted with green rectangles

## Project Structure

- `include/` - Header files
  - `camera_manager.hpp` - Camera interface management
  - `motion_tracker.hpp` - Motion detection and tracking
- `src/` - Source files
  - `main.cpp` - Application entry point
  - `camera_manager.cpp` - Camera implementation
  - `motion_tracker.cpp` - Motion tracking implementation
- `CMakeLists.txt` - Build configuration

## Future Enhancements

- Support for multiple cameras
- Advanced motion tracking algorithms
- Bird-specific detection and tracking
- Data logging and analysis
- Network streaming capabilities
- Integration with ML component for species classification 
- ...

target_link_libraries(BirdsOfPlay
    ${OpenCV_LIBS}
    yaml-cpp
    pthread
)