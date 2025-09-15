#ifndef TEST_HELPERS_HPP
#define TEST_HELPERS_HPP

#include <string>
#include <filesystem>
#include <unistd.h>
#include <limits.h>

/**
 * Find the test resource directory relative to the executable location.
 * This allows tests to find test images regardless of where the executable is run from.
 */
inline std::string findTestResourceDir() {
    // Get the path to the current executable
    char exePath[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);
    if (count == -1) {
        // Fallback: try to get executable path using getcwd and argv[0] approach
        std::filesystem::path currentPath = std::filesystem::current_path();
        return (currentPath / "../../../src/motion_detection/tests").string();
    }

    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

    // Navigate up from build directory to find source directory
    // Pattern: build/src/motion_detection/tests/ -> src/motion_detection/tests/
    std::filesystem::path sourceDir = exeDir / "../../../src/motion_detection/tests";

    // If the source directory exists, use it
    if (std::filesystem::exists(sourceDir)) {
        return sourceDir.string();
    }

    // Fallback: try relative to current directory
    return ".";
}

#endif // TEST_HELPERS_HPP
