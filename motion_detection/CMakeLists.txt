cmake_minimum_required(VERSION 3.10)
project(BirdsOfPlay)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)

# Set MongoDB paths for CMake based on platform
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH 
        "/opt/homebrew/Cellar/mongo-cxx-driver/4.1.0/lib/cmake/mongocxx-4.1.0"
        "/opt/homebrew/Cellar/mongo-cxx-driver/4.1.0/lib/cmake/bsoncxx-4.1.0"
    )
elseif(UNIX AND NOT APPLE)
    # Linux paths
    list(APPEND CMAKE_PREFIX_PATH 
        "/usr/lib/x86_64-linux-gnu/cmake/mongocxx-3.7.0"
        "/usr/lib/x86_64-linux-gnu/cmake/bsoncxx-3.7.0"
        "/usr/lib/x86_64-linux-gnu/cmake/libmongocxx-3.7.0"
        "/usr/lib/x86_64-linux-gnu/cmake/libbsoncxx-3.7.0"
    )
endif()

# Find MongoDB
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

# Add uuid library
find_library(UUID_LIBRARIES uuid)

# Add source files (excluding main.cpp)
set(LIB_SOURCES
    src/motion_tracker.cpp
    src/data_collector.cpp
)

# Add header files
set(HEADERS
    include/motion_tracker.hpp
    include/data_collector.hpp
)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Include directories
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${MONGOCXX_INCLUDE_DIRS}
    ${BSONCXX_INCLUDE_DIRS}
)

# Add Homebrew include path for macOS (Apple Silicon)
if(EXISTS "/opt/homebrew/include")
    include_directories(/opt/homebrew/include)
endif()

# Add Homebrew lib path for macOS (Apple Silicon)
if(EXISTS "/opt/homebrew/lib")
    link_directories(/opt/homebrew/lib)
endif()

# Platform-specific linking
if(UNIX AND NOT APPLE)
    # Linux (e.g., Ubuntu CI)
    set(EXTRA_LIBS stdc++)
elseif(APPLE)
    # macOS (do not add stdc++, use default)
    set(EXTRA_LIBS "")
else()
    set(EXTRA_LIBS "")
endif()

# Create main executable
add_executable(${PROJECT_NAME} ${LIB_SOURCES} src/main.cpp ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
        ${YAML_CPP_INCLUDE_DIR}
        ${UUID_INCLUDE_DIRS}
        ${MONGOCXX_INCLUDE_DIRS}
        ${BSONCXX_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${OpenCV_LIBS}
        yaml-cpp
        ${UUID_LIBRARIES}
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
)

# Copy config file to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
              ${CMAKE_CURRENT_BINARY_DIR}/config.yaml COPYONLY)

# Enable testing
enable_testing()

add_executable(${PROJECT_NAME}_test tests/motion_tracker_test.cpp ${LIB_SOURCES})
target_include_directories(${PROJECT_NAME}_test 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
        ${YAML_CPP_INCLUDE_DIR}
        ${UUID_INCLUDE_DIRS}
        ${MONGOCXX_INCLUDE_DIRS}
        ${BSONCXX_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}_test
    PRIVATE
        GTest::Main
        ${OpenCV_LIBS}
        yaml-cpp
        ${UUID_LIBRARIES}
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
)

add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test) 