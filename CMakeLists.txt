cmake_minimum_required(VERSION 3.10)
project(BirdsOfPlay VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV package
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Add source files
set(SOURCES
    src/main.cpp
    src/motion_tracker.cpp
    src/camera_manager.cpp
)

# Add header files
set(HEADERS
    include/motion_tracker.hpp
    include/camera_manager.hpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE include) 