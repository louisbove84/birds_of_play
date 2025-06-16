cmake_minimum_required(VERSION 3.10)
project(BirdsOfPlay)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(GTest REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS} include)

# Add main executable
add_executable(BirdsOfPlay 
    src/main.cpp
    src/motion_tracker.cpp
)

# Link libraries for main executable
target_link_libraries(BirdsOfPlay ${OpenCV_LIBS})

# Add test executable
add_executable(motion_tracker_test
    tests/motion_tracker_test.cpp
    src/motion_tracker.cpp
)

# Link libraries for test executable
target_link_libraries(motion_tracker_test
    ${OpenCV_LIBS}
    GTest::GTest
    GTest::Main
)

# Enable testing
enable_testing()
add_test(NAME motion_tracker_test COMMAND motion_tracker_test) 