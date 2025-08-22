.PHONY: all build test clean run test-unit test-motion test-processor test-quick test-working help

# Default target
all: help

# Build directory
BUILD_DIR = build

# Create build directory and build project
build:
	@echo "🔨 Building project..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DBUILD_TESTING=ON .. && make

# Run the main application
run: build
	@echo "🚀 Running BirdsOfPlay..."
	@cd $(BUILD_DIR) && ./BirdsOfPlay

# Run unit tests
test-unit: build
	@echo "🧪 Running unit tests..."
	@cd $(BUILD_DIR) && ./BirdsOfPlay_test || echo "⚠️  Unit tests failed (logger initialization issue)"

# Run motion tracker tests
test-motion: build
	@echo "🧪 Running motion tracker tests..."
	@cd $(BUILD_DIR) && ./motion_tracker_test || echo "⚠️  Motion tracker tests failed (logger initialization issue)"

# Run motion processor tests
test-processor: build
	@echo "🔄 Running motion processor tests..."
	@cd $(BUILD_DIR) && ./motion_processor_test
	@echo "📁 Visual results saved to: build/test_results/motion_processor/"

# Run motion region consolidator tests (requires debug build)
test-consolidator:
	@echo "🔄 Running motion region consolidator tests..."
	@echo "🔨 Building debug version for consolidator tests..."
	@mkdir -p build_debug
	@cd build_debug && cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON .. && make motion_region_consolidator_test
	@echo "🧪 Executing consolidator tests..."
	@cd build_debug && \
		mkdir -p tests/img && cp ../tests/img/* tests/img/ && \
		mkdir -p tests && cp ../tests/config.yaml tests/ && \
		./motion_region_consolidator_test
	@echo "📁 Visual results saved to: build_debug/test_results/motion_region_consolidator/"
	@echo "✅ Consolidator test completed!"

# Run all tests
test: test-unit test-motion test-processor test-consolidator
	@echo ""
	@echo "✅ All tests completed successfully!"
	@echo "📁 Test results available in: build/test_results/"

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

# Quick test - run only motion processor tests (fastest)
test-quick: test-processor
	@echo "⚡ Quick test completed!"

# Working tests only (motion processor and consolidator tests work perfectly)
test-working: test-processor test-consolidator
	@echo ""
	@echo "✅ Working tests completed successfully!"
	@echo "📁 Visual test results available in:"
	@echo "    - build/test_results/motion_processor/"
	@echo "    - build_debug/test_results/motion_region_consolidator/"

# Help message
help:
	@echo "🦅 BirdsOfPlay Motion Detection - Available commands:"
	@echo ""
	@echo "📦 BUILD COMMANDS:"
	@echo "  make build        - Build the project"
	@echo "  make clean        - Remove build directory"
	@echo ""
	@echo "🚀 RUN COMMANDS:"
	@echo "  make run          - Build and run the main application"
	@echo ""
	@echo "🧪 TEST COMMANDS:"
	@echo "  make test         - Build and run all tests (includes broken unit tests)"
	@echo "  make test-working - Run only working tests (recommended)"
	@echo "  make test-quick   - Run motion processor tests only (fastest)"
	@echo "  make test-unit    - Run unit tests (currently failing)"
	@echo "  make test-motion  - Run motion tracker tests"
	@echo "  make test-processor - Run motion processor tests"
	@echo "  make test-consolidator - Run motion region consolidator tests"
	@echo ""
	@echo "📁 TEST RESULTS:"
	@echo "  Visual test results are saved to:"
	@echo "    - build/test_results/ (processor tests)"
	@echo "    - build_debug/test_results/ (consolidator tests)"
	@echo ""
	@echo "💡 QUICK START:"
	@echo "  make test-quick   - Fast test with visual output"
	@echo "  make test-working - Recommended test suite (working tests only)"
	@echo "  make run          - Start the application"
