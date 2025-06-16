cmake_minimum_required(VERSION 3.10)
project(BirdsOfPlay)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++")

# Include directories
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add executable
add_executable(BirdsOfPlay 
    src/main.cpp
    src/motion_tracker.cpp
)

# Link libraries for main executable
target_link_libraries(BirdsOfPlay
    ${OpenCV_LIBS}
    stdc++
    pthread
)

# Add test executable
add_executable(motion_tracker_test
    tests/motion_tracker_test.cpp
    src/motion_tracker.cpp
)

# Link libraries for test executable
target_link_libraries(motion_tracker_test
    ${OpenCV_LIBS}
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    stdc++
    pthread
)

# Enable testing
enable_testing()
add_test(NAME motion_tracker_test COMMAND motion_tracker_test) 