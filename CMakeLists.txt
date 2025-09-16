cmake_minimum_required(VERSION 3.14)
project(birds_of_play)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler detection and configuration (optimized for Clang/AppleClang)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    message(STATUS "Using Clang compiler: ${CMAKE_CXX_COMPILER_VERSION}")
    set(COMPILER_CLANG TRUE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wshadow -Wconversion")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fno-omit-frame-pointer")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
    endif()
else()
    message(WARNING "Non-Clang compiler detected: ${CMAKE_CXX_COMPILER_ID}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# Enable testing
enable_testing()

# Add motion_detection subdirectory (contains all the actual code and tests)
add_subdirectory(src/motion_detection)

# Find required packages for main executable
find_package(OpenCV REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)



# Add Homebrew include path for macOS (Apple Silicon)
if(EXISTS "/opt/homebrew/include")
    include_directories(/opt/homebrew/include)
endif()

# Add Homebrew lib path for macOS (Apple Silicon)
if(EXISTS "/opt/homebrew/lib")
    link_directories(/opt/homebrew/lib)
endif()

# Create main executable using the motion detection library
add_executable(birds_of_play src/main.cpp src/mongodb_functions.cpp)

# Create test executable for Python bindings
add_executable(test_python_bindings test_python_bindings.cpp)

# Link against the motion detection library
target_link_libraries(birds_of_play
    PRIVATE
        BirdsOfPlay_lib
        ${OpenCV_LIBS}
        yaml-cpp
        pybind11::embed
        Python3::Python
)

# Link test executable
target_link_libraries(test_python_bindings
    PRIVATE
        pybind11::embed
        Python3::Python
)

# Include directories for main executable
target_include_directories(birds_of_play
    PRIVATE
        src/motion_detection/include
        ${OpenCV_INCLUDE_DIRS}
        ${YAML_CPP_INCLUDE_DIR}
        ${pybind11_INCLUDE_DIRS}
)

# Include directories for test executable
target_include_directories(test_python_bindings
    PRIVATE
        ${pybind11_INCLUDE_DIRS}
)
