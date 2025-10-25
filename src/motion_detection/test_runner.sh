#!/bin/bash

# Birds of Play Motion Detection Test Runner
# This script provides convenient commands for building and testing

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🐦 Birds of Play Motion Detection Test Runner${NC}"
echo ""

# Function to print usage
print_usage() {
    echo -e "${YELLOW}Usage: ./test_runner.sh [command]${NC}"
    echo ""
    echo -e "${GREEN}Available commands:${NC}"
    echo "  fresh-test      - Remove build folder, rebuild everything, and run all tests"
    echo "  fresh-build     - Remove build folder and rebuild everything"
    echo "  test-all        - Clean, rebuild, and run all tests (faster than fresh-test)"
    echo "  test-motion     - Run only motion processor tests"
    echo "  test-consolidator - Run only motion region consolidator tests"
    echo "  test-integration - Run only integration tests"
    echo "  clean-rebuild   - Clean and rebuild without running tests"
    echo "  run-tests       - Run all tests (assumes already built)"
    echo "  clean-all       - Remove entire build directory"
    echo ""
    echo -e "${YELLOW}Examples:${NC}"
    echo "  ./test_runner.sh fresh-test      # Start completely fresh"
    echo "  ./test_runner.sh test-motion     # Quick motion processor test"
    echo "  ./test_runner.sh test-all        # Clean rebuild and test"
    echo ""
    echo -e "${BLUE}Note: The first run should use 'fresh-test' to set up everything${NC}"
}

# Check if command provided
if [ $# -eq 0 ]; then
    print_usage
    exit 1
fi

COMMAND=$1
BUILD_DIR="build_debug"

# Navigate to motion detection directory if not already there
if [[ ! -f "CMakeLists.txt" ]]; then
    echo -e "${RED}Error: CMakeLists.txt not found. Please run this script from the motion_detection directory.${NC}"
    exit 1
fi

case $COMMAND in
    "fresh-test")
        echo -e "${GREEN}🚀 Starting fresh build and test...${NC}"
        if [ -d "$BUILD_DIR" ]; then
            echo -e "${YELLOW}Removing existing build directory...${NC}"
            rm -rf "$BUILD_DIR"
        fi
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        echo -e "${BLUE}Configuring with CMake...${NC}"
        cmake .. -DBUILD_TESTING=ON
        echo -e "${BLUE}Building all targets...${NC}"
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
        echo -e "${GREEN}Running all tests...${NC}"
        ./motion_processor_test --gtest
        ./motion_region_consolidator_test
        ./integration_test
        echo -e "${GREEN}✅ Fresh build and test completed successfully!${NC}"
        ;;
        
    "fresh-build")
        echo -e "${GREEN}🔨 Starting fresh build...${NC}"
        if [ -d "$BUILD_DIR" ]; then
            echo -e "${YELLOW}Removing existing build directory...${NC}"
            rm -rf "$BUILD_DIR"
        fi
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        echo -e "${BLUE}Configuring with CMake...${NC}"
        cmake .. -DBUILD_TESTING=ON
        echo -e "${BLUE}Building all targets...${NC}"
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
        echo -e "${GREEN}✅ Fresh build completed successfully!${NC}"
        ;;
        
    "test-all")
        echo -e "${GREEN}🧪 Clean rebuild and test all...${NC}"
        if [ ! -d "$BUILD_DIR" ]; then
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake .. -DBUILD_TESTING=ON
        else
            cd "$BUILD_DIR"
        fi
        make test-all
        echo -e "${GREEN}✅ All tests completed successfully!${NC}"
        ;;
        
    "test-motion")
        echo -e "${GREEN}🎯 Testing motion processor...${NC}"
        if [ ! -d "$BUILD_DIR" ]; then
            echo -e "${RED}Build directory not found. Run 'fresh-build' first.${NC}"
            exit 1
        fi
        cd "$BUILD_DIR"
        make test-motion-processor
        echo -e "${GREEN}✅ Motion processor tests completed!${NC}"
        ;;
        
    "test-consolidator")
        echo -e "${GREEN}🎯 Testing motion region consolidator...${NC}"
        if [ ! -d "$BUILD_DIR" ]; then
            echo -e "${RED}Build directory not found. Run 'fresh-build' first.${NC}"
            exit 1
        fi
        cd "$BUILD_DIR"
        make test-motion-consolidator
        echo -e "${GREEN}✅ Motion region consolidator tests completed!${NC}"
        ;;
        
    "test-integration")
        echo -e "${GREEN}🎯 Testing integration pipeline...${NC}"
        if [ ! -d "$BUILD_DIR" ]; then
            echo -e "${RED}Build directory not found. Run 'fresh-build' first.${NC}"
            exit 1
        fi
        cd "$BUILD_DIR"
        make test-integration
        echo -e "${GREEN}✅ Integration tests completed!${NC}"
        ;;
        
    "clean-rebuild")
        echo -e "${GREEN}🧹 Clean and rebuild...${NC}"
        if [ ! -d "$BUILD_DIR" ]; then
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake .. -DBUILD_TESTING=ON
        else
            cd "$BUILD_DIR"
        fi
        make clean-rebuild
        echo -e "${GREEN}✅ Clean rebuild completed!${NC}"
        ;;
        
    "run-tests")
        echo -e "${GREEN}🏃 Running all tests...${NC}"
        if [ ! -d "$BUILD_DIR" ]; then
            echo -e "${RED}Build directory not found. Run 'fresh-build' first.${NC}"
            exit 1
        fi
        cd "$BUILD_DIR"
        make run-all-tests
        echo -e "${GREEN}✅ All tests completed!${NC}"
        ;;
        
    "clean-all")
        echo -e "${YELLOW}🗑️  Removing entire build directory...${NC}"
        if [ -d "$BUILD_DIR" ]; then
            rm -rf "$BUILD_DIR"
            echo -e "${GREEN}✅ Build directory removed!${NC}"
        else
            echo -e "${BLUE}Build directory doesn't exist.${NC}"
        fi
        ;;
        
    *)
        echo -e "${RED}Unknown command: $COMMAND${NC}"
        echo ""
        print_usage
        exit 1
        ;;
esac
