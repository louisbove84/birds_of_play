#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "../include/motion_tracker.hpp"

class MotionTrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test frame
        testFrame = cv::Mat::zeros(480, 640, CV_8UC3);
        
        // Create a motion tracker instance
        tracker = std::make_unique<MotionTracker>();
    }

    void TearDown() override {
        // Clean up
        testFrame.release();
    }

    cv::Mat testFrame;
    std::unique_ptr<MotionTracker> tracker;
};

// Test initialization
TEST_F(MotionTrackerTest, Initialization) {
    EXPECT_TRUE(tracker != nullptr);
}

// Test motion detection with static frame
TEST_F(MotionTrackerTest, NoMotionDetection) {
    // Process the same frame twice (should detect no motion)
    auto result1 = tracker->processFrame(testFrame);
    auto result2 = tracker->processFrame(testFrame);
    
    EXPECT_FALSE(result1.hasMotion);
    EXPECT_FALSE(result2.hasMotion);
    EXPECT_TRUE(result1.motionRegions.empty());
    EXPECT_TRUE(result2.motionRegions.empty());
}

// Test motion detection with moving object
TEST_F(MotionTrackerTest, MotionDetection) {
    // Create a frame with a moving object
    cv::Mat frame1 = testFrame.clone();
    cv::Mat frame2 = testFrame.clone();
    
    // Draw a rectangle in frame2 to simulate motion
    cv::rectangle(frame2, cv::Rect(100, 100, 50, 50), cv::Scalar(255, 255, 255), -1);
    
    // Process both frames
    auto result1 = tracker->processFrame(frame1);
    auto result2 = tracker->processFrame(frame2);
    
    EXPECT_FALSE(result1.hasMotion);
    EXPECT_TRUE(result2.hasMotion);
    EXPECT_FALSE(result2.motionRegions.empty());
}

// Test motion region properties
TEST_F(MotionTrackerTest, MotionRegionProperties) {
    // Create frames with a moving object
    cv::Mat frame1 = testFrame.clone();
    cv::Mat frame2 = testFrame.clone();
    
    // Draw a rectangle in frame2
    cv::Rect expectedRect(100, 100, 50, 50);
    cv::rectangle(frame2, expectedRect, cv::Scalar(255, 255, 255), -1);
    
    // Process frames
    tracker->processFrame(frame1);
    auto result = tracker->processFrame(frame2);
    
    // Check if motion was detected
    EXPECT_TRUE(result.hasMotion);
    EXPECT_FALSE(result.motionRegions.empty());
    
    // Check if the detected region matches the expected region
    if (!result.motionRegions.empty()) {
        const auto& detectedRect = result.motionRegions[0];
        EXPECT_NEAR(detectedRect.x, expectedRect.x, 10);
        EXPECT_NEAR(detectedRect.y, expectedRect.y, 10);
        EXPECT_NEAR(detectedRect.width, expectedRect.width, 10);
        EXPECT_NEAR(detectedRect.height, expectedRect.height, 10);
    }
}

// Test motion tracking with multiple objects
TEST_F(MotionTrackerTest, MultipleObjects) {
    // Create frames with multiple moving objects
    cv::Mat frame1 = testFrame.clone();
    cv::Mat frame2 = testFrame.clone();
    
    // Draw multiple rectangles in frame2
    cv::rectangle(frame2, cv::Rect(100, 100, 50, 50), cv::Scalar(255, 255, 255), -1);
    cv::rectangle(frame2, cv::Rect(300, 300, 50, 50), cv::Scalar(255, 255, 255), -1);
    
    // Process frames
    tracker->processFrame(frame1);
    auto result = tracker->processFrame(frame2);
    
    // Check if motion was detected
    EXPECT_TRUE(result.hasMotion);
    EXPECT_GE(result.motionRegions.size(), 1);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 