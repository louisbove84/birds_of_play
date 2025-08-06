#include <gtest/gtest.h>
#include "src/camera_manager.hpp"
#include <opencv2/opencv.hpp>

// Basic test for CameraManager
class CameraManagerTest : public ::testing::Test {
protected:
    CameraManager camera_manager_;
};

TEST_F(CameraManagerTest, InitialState) {
    // Test that camera starts in uninitialized state
    cv::Mat frame;
    EXPECT_FALSE(camera_manager_.getFrame(frame));
}

TEST_F(CameraManagerTest, InitializationWithInvalidDevice) {
    // Test initialization with invalid device ID
    EXPECT_FALSE(camera_manager_.initialize(999));
}

TEST_F(CameraManagerTest, MultipleInitializationCalls) {
    // Test that multiple initialization calls don't break anything
    camera_manager_.initialize(999);  // This will fail, but shouldn't crash
    camera_manager_.initialize(999);  // Second call should also not crash
    SUCCEED();  // If we reach here, no crashes occurred
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}