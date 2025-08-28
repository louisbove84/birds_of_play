cmake_minimum_required(VERSION 3.10)
project(BirdsOfPlay)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(GTest REQUIRED)

# Find pybind11 for Python bindings
find_package(pybind11 REQUIRED)

# Find MongoDB packages
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

# Set MongoDB paths for CMake based on platform
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH 
        "/opt/homebrew/Cellar/mongo-cxx-driver/4.1.1/lib/cmake/mongocxx-4.1.1"
        "/opt/homebrew/Cellar/mongo-cxx-driver/4.1.1/lib/cmake/bsoncxx-4.1.1"
        "/opt/homebrew/Cellar/mongo-c-driver/2.1.0/lib/cmake/libmongoc-1.0"
        "/opt/homebrew/Cellar/mongo-c-driver/2.1.0/lib/cmake/libbson-1.0"
    )
    include_directories(
        "/opt/homebrew/Cellar/mongo-cxx-driver/4.1.1/include/mongocxx/v_noabi"
        "/opt/homebrew/Cellar/mongo-cxx-driver/4.1.1/include/bsoncxx/v_noabi"
        "/opt/homebrew/Cellar/mongo-c-driver/2.1.0/include/libmongoc-1.0"
        "/opt/homebrew/Cellar/mongo-c-driver/2.1.0/include/libbson-1.0"
    )
elseif(UNIX AND NOT APPLE)
    # Linux paths - we installed to /usr/local
    list(APPEND CMAKE_PREFIX_PATH 
        "/usr/local/lib/cmake/mongocxx-3.8.0"
        "/usr/local/lib/cmake/bsoncxx-3.8.0"
        "/usr/local/lib/cmake/libmongocxx-3.8.0"
        "/usr/local/lib/cmake/libbsoncxx-3.8.0"
    )
    
    # Also add the lib directory to the library search path
    link_directories("/usr/local/lib")
endif()

# Find MongoDB
find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)

# Add uuid library
find_library(UUID_LIBRARIES uuid)

# Add source files for motion detection library
set(LIB_SOURCES
    src/motion_processor.cpp
    src/motion_visualization.cpp
    src/motion_region_consolidator.cpp
    src/motion_pipeline.cpp
)

# Add header files for motion detection library
set(HEADERS
    include/motion_processor.hpp
    include/motion_visualization.hpp
    include/motion_region_consolidator.hpp
    include/motion_pipeline.hpp
    include/tracked_object.hpp
)

# Compiler-specific flags (inherited from parent CMakeLists.txt)
# Additional Clang-specific optimizations
if(COMPILER_CLANG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
endif()

# Add spdlog and make its headers available globally
add_subdirectory(libs/spdlog)
include_directories(libs/spdlog/include)

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
    set(EXTRA_LIBS stdc++ m)
elseif(APPLE)
    # macOS (do not add stdc++, use default)
    set(EXTRA_LIBS m)
else()
    set(EXTRA_LIBS m)
endif()

# Create motion detection library (no main executable here - main is in parent directory)
# Note: Main executable is now in birds_of_play/main.cpp
add_library(${PROJECT_NAME}_lib STATIC ${LIB_SOURCES} src/logger.cpp ${HEADERS})

# Include directories for library
target_include_directories(${PROJECT_NAME}_lib 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include
    PRIVATE 
        ${OpenCV_INCLUDE_DIRS}
        ${YAML_CPP_INCLUDE_DIR}
        ${UUID_INCLUDE_DIRS}
        ${MONGOCXX_INCLUDE_DIRS}
        ${BSONCXX_INCLUDE_DIRS}
)

# Link libraries for library
target_link_libraries(${PROJECT_NAME}_lib
    PUBLIC
        ${OpenCV_LIBS}
        yaml-cpp
        spdlog::spdlog_header_only
    PRIVATE
        ${UUID_LIBRARIES}
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
        ${EXTRA_LIBS}
        stdc++
        m
)

# Copy config file to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
              ${CMAKE_CURRENT_BINARY_DIR}/config.yaml COPYONLY)

# Enable testing
enable_testing()

add_executable(${PROJECT_NAME}_test tests/motion_processor_test.cpp ${LIB_SOURCES} src/logger.cpp)
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
        ${EXTRA_LIBS}
        stdc++
        m
)

add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)

# Unit Testing
# ==============================================================================
if(BUILD_TESTING)

    # Add motion_processor_test executable
    add_executable(motion_processor_test 
        tests/motion_processor_test.cpp
        src/motion_processor.cpp
        src/logger.cpp
    )

    # Add motion_region_consolidator_test executable
    add_executable(motion_region_consolidator_test 
        tests/motion_region_consolidator_test.cpp
        src/motion_region_consolidator.cpp
        src/motion_processor.cpp
        src/motion_visualization.cpp
        src/logger.cpp
    )

    # Add integration_test executable
    add_executable(integration_test 
        tests/integration_test.cpp
        src/motion_region_consolidator.cpp
        src/motion_processor.cpp
        src/motion_visualization.cpp
        src/logger.cpp
        src/motion_pipeline.cpp
    )





    # Link libraries for motion_processor_test
    target_link_libraries(motion_processor_test PRIVATE 
        ${OpenCV_LIBS}
        yaml-cpp
        spdlog::spdlog_header_only
        GTest::gtest
        GTest::gtest_main
    )

    # Add include directories for motion_processor_test
    target_include_directories(motion_processor_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include
    )

    # Link libraries for motion_region_consolidator_test
    target_link_libraries(motion_region_consolidator_test PRIVATE 
        ${OpenCV_LIBS}
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
        yaml-cpp
        spdlog::spdlog_header_only
        GTest::gtest
        GTest::gtest_main
    )

    # Add include directories for motion_region_consolidator_test
    target_include_directories(motion_region_consolidator_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include
    )

    # Link libraries for integration_test
    target_link_libraries(integration_test PRIVATE 
        ${OpenCV_LIBS}
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
        yaml-cpp
        spdlog::spdlog_header_only
        GTest::gtest
        GTest::gtest_main
    )

    # Add include directories for integration_test
    target_include_directories(integration_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include
    )





    # Copy test config to build directory
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/config.yaml
                  ${CMAKE_CURRENT_BINARY_DIR}/config.yaml COPYONLY)
    
    # Copy all test images from subdirectories recursively
    file(GLOB_RECURSE TEST_IMAGES "${CMAKE_CURRENT_SOURCE_DIR}/tests/img/*.jpg" "${CMAKE_CURRENT_SOURCE_DIR}/tests/img/*.png" "${CMAKE_CURRENT_SOURCE_DIR}/tests/img/*.jpeg")
    foreach(TEST_IMAGE ${TEST_IMAGES})
        file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests/img" ${TEST_IMAGE})
        configure_file(${TEST_IMAGE} ${CMAKE_CURRENT_BINARY_DIR}/tests/img/${REL_PATH} COPYONLY)
    endforeach()

    # ============================================================================
    # CONVENIENCE TARGETS FOR DEVELOPMENT
    # ============================================================================
    
    # Clean and rebuild everything
    add_custom_target(clean-rebuild
        COMMAND ${CMAKE_COMMAND} --build . --target clean
        COMMAND ${CMAKE_COMMAND} --build . --parallel
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Cleaning and rebuilding all targets"
        USES_TERMINAL
    )
    
    # Run all tests
    add_custom_target(run-all-tests
        COMMAND ./motion_processor_test --gtest
        COMMAND ./motion_region_consolidator_test
        COMMAND ./integration_test
        DEPENDS motion_processor_test motion_region_consolidator_test integration_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running all test executables"
        USES_TERMINAL
    )
    
    # Clean, rebuild, and run all tests
    add_custom_target(test-all
        COMMAND ${CMAKE_COMMAND} --build . --target clean
        COMMAND ${CMAKE_COMMAND} --build . --parallel
        COMMAND ./motion_processor_test --gtest
        COMMAND ./motion_region_consolidator_test
        COMMAND ./integration_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Clean rebuild and run all tests"
        USES_TERMINAL
    )
    
    # Run motion processor tests only
    add_custom_target(test-motion-processor
        COMMAND ./motion_processor_test --gtest
        DEPENDS motion_processor_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running motion processor tests"
        USES_TERMINAL
    )
    
    # Run motion region consolidator tests only
    add_custom_target(test-motion-consolidator
        COMMAND ./motion_region_consolidator_test
        DEPENDS motion_region_consolidator_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running motion region consolidator tests"
        USES_TERMINAL
    )
    
    # Run integration tests only
    add_custom_target(test-integration
        COMMAND ./integration_test
        DEPENDS integration_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running integration tests"
        USES_TERMINAL
    )
    
    # Clean build directory completely (removes build_debug folder)
    add_custom_target(clean-all
        COMMAND ${CMAKE_COMMAND} -E echo "Removing entire build directory..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Removing entire build directory"
        USES_TERMINAL
    )
    
    # Fresh start: remove build directory and recreate it
    add_custom_target(fresh-build
        COMMAND ${CMAKE_COMMAND} -E echo "Starting fresh build..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR} -B ${CMAKE_CURRENT_BINARY_DIR} -DBUILD_TESTING=ON
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --parallel
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Fresh build from scratch"
        USES_TERMINAL
    )
    
    # Fresh build and test
    add_custom_target(fresh-test
        COMMAND ${CMAKE_COMMAND} -E echo "Starting fresh build and test..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR} -B ${CMAKE_CURRENT_BINARY_DIR} -DBUILD_TESTING=ON
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --parallel
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR} ./motion_processor_test --gtest
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR} ./motion_region_consolidator_test
        COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR} ./integration_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Fresh build and run all tests"
        USES_TERMINAL
    )

endif() 

# ==============================================================================
# PYTHON BINDINGS
# ==============================================================================

# Create Python module using pybind11
pybind11_add_module(birds_of_play_python src/python_bindings.cpp)

# Link the Python module with our library and dependencies
target_link_libraries(birds_of_play_python
    PRIVATE
        ${PROJECT_NAME}_lib
        ${OpenCV_LIBS}
        yaml-cpp
        spdlog::spdlog_header_only
        ${UUID_LIBRARIES}
        mongo::mongocxx_shared
        mongo::bsoncxx_shared
        ${EXTRA_LIBS}
)

# Include directories for Python module
target_include_directories(birds_of_play_python
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include
        ${OpenCV_INCLUDE_DIRS}
        ${YAML_CPP_INCLUDE_DIR}
        ${UUID_INCLUDE_DIRS}
        ${MONGOCXX_INCLUDE_DIRS}
        ${BSONCXX_INCLUDE_DIRS}
)