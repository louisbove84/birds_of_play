#include "motion_tracker.hpp"
#include "logger.hpp"
#include <opencv2/opencv.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>

void initLogger() {
    Logger::init("debug", "test_log.txt", true);
}

void createTestDirectory(const std::string& testName) {
    std::filesystem::create_directories("test_results/" + testName);
}

// Test preprocessFrame
void testPreprocessFrame() {
    createTestDirectory("preprocess_frame");
    
    MotionTracker tracker("config.yaml");
    cv::Mat frame = cv::imread("test_image.jpg");
    assert(!frame.empty() && "Failed to load test_image.jpg");

    cv::Mat processed = tracker.preprocessFrame(frame);
    assert(!processed.empty() && "preprocessFrame returned empty frame");
    assert(processed.type() == CV_8UC1 && "Frame should be single-channel (grayscale or HSV)");
    assert(processed.size() == frame.size() && "Frame size should match");
    
    // Save output images for visual inspection
    cv::imwrite("test_results/preprocess_frame/original_frame.jpg", frame);
    cv::imwrite("test_results/preprocess_frame/preprocessed_frame.jpg", processed);
    std::cout << "✓ Saved preprocessing results to test_results/preprocess_frame/" << std::endl;
}

// Test detectMotion
void testDetectMotion() {
    createTestDirectory("detect_motion");
    
    MotionTracker tracker("config.yaml");
    cv::Mat frame1 = cv::imread("test_image.jpg");
    cv::Mat frame2 = cv::imread("test_image2.jpg");
    assert(!frame1.empty() && !frame2.empty() && "Failed to load test images");

    tracker.setPrevFrame(tracker.preprocessFrame(frame1));
    cv::Mat frameDiff, thresh;
    cv::Mat motionMask = tracker.detectMotion(tracker.preprocessFrame(frame2), frameDiff, thresh);
    
    assert(!motionMask.empty() && "Motion mask should not be empty");
    assert(motionMask.type() == CV_8UC1 && "Motion mask should be single-channel");
    assert(!frameDiff.empty() && "Frame difference should not be empty");
    assert(!thresh.empty() && "Thresholded mask should not be empty");
    
    // Save motion detection outputs
    cv::imwrite("test_results/detect_motion/frame1_reference.jpg", frame1);
    cv::imwrite("test_results/detect_motion/frame2_comparison.jpg", frame2);
    cv::imwrite("test_results/detect_motion/frame_difference.jpg", frameDiff);
    cv::imwrite("test_results/detect_motion/threshold_mask.jpg", thresh);
    cv::imwrite("test_results/detect_motion/motion_mask_final.jpg", motionMask);
    std::cout << "✓ Saved motion detection results to test_results/detect_motion/" << std::endl;
}

// Test applyMorphologicalOps
void testApplyMorphologicalOps() {
    createTestDirectory("morphological_ops");
    
    MotionTracker tracker("config.yaml");
    cv::Mat thresh = cv::imread("motion_mask.png", cv::IMREAD_GRAYSCALE);
    assert(!thresh.empty() && "Failed to load motion_mask.png");

    cv::Mat processed = tracker.applyMorphologicalOps(thresh);
    assert(!processed.empty() && "Processed mask should not be empty");
    assert(processed.type() == CV_8UC1 && "Processed mask should be single-channel");
    assert(processed.size() == thresh.size() && "Processed mask size should match");
    
    // Save morphological operations output
    cv::imwrite("test_results/morphological_ops/input_mask.jpg", thresh);
    cv::imwrite("test_results/morphological_ops/processed_mask.jpg", processed);
    std::cout << "✓ Saved morphological operations results to test_results/morphological_ops/" << std::endl;
}

// Test extractContours
void testExtractContours() {
    createTestDirectory("extract_contours");
    
    MotionTracker tracker("config.yaml");
    cv::Mat processed = cv::imread("motion_mask.png", cv::IMREAD_GRAYSCALE);
    assert(!processed.empty() && "Failed to load motion_mask.png");

    std::vector<cv::Rect> newBounds = tracker.extractContours(processed);
    assert(!newBounds.empty() && "Contours should be detected");
    
    // Create visualization with bounding boxes
    cv::Mat visualization;
    cv::cvtColor(processed, visualization, cv::COLOR_GRAY2BGR);
    for (const auto& rect : newBounds) {
        assert(rect.width > 0 && rect.height > 0 && "Invalid bounding box");
        std::cout << "Contour bounds: " << rect << std::endl;
        // Draw bounding box in red
        cv::rectangle(visualization, rect, cv::Scalar(0, 0, 255), 2);
        // Add text label
        cv::putText(visualization, std::to_string(rect.width) + "x" + std::to_string(rect.height), 
                   cv::Point(rect.x, rect.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
    }
    
    // Save contour detection outputs
    cv::imwrite("test_results/extract_contours/input_mask.jpg", processed);
    cv::imwrite("test_results/extract_contours/contours_with_bounding_boxes.jpg", visualization);
    std::cout << "✓ Saved contour extraction results to test_results/extract_contours/" << std::endl;
}

// Test logTrackingResults
void testLogTrackingResults() {
    MotionTracker tracker("config.yaml");
    TrackedObject obj(1, cv::Rect(100, 100, 30, 30), "uuid1");
    obj.trajectory.push_back(cv::Point(110, 110));
    obj.trajectory.push_back(cv::Point(120, 120));
    obj.confidence = 0.8;
    tracker.setTrackedObjects({obj});
    
    std::ofstream logFile("test_log.txt");
    auto oldCoutBuf = std::cout.rdbuf(logFile.rdbuf());
    tracker.logTrackingResults();
    std::cout.rdbuf(oldCoutBuf);
    logFile.close();
    
    std::ifstream inFile("test_log.txt");
    std::string line;
    bool found = false;
    while (std::getline(inFile, line)) {
        if (line.find("Object 1") != std::string::npos && 
            line.find("confidence=0.8") != std::string::npos) {
            found = true;
        }
    }
    inFile.close();
    assert(found && "Log should contain Object 1 with confidence=0.8");
}

int main() {
    std::cout << "Running MotionTracker tests..." << std::endl;
    initLogger();
    
    testPreprocessFrame();
    std::cout << "testPreprocessFrame passed" << std::endl;
    
    testDetectMotion();
    std::cout << "testDetectMotion passed" << std::endl;
    
    testApplyMorphologicalOps();
    std::cout << "testApplyMorphologicalOps passed" << std::endl;
    
    testExtractContours();
    std::cout << "testExtractContours passed" << std::endl;
    
    testLogTrackingResults();
    std::cout << "testLogTrackingResults passed" << std::endl;
    
    std::cout << "All tests passed!" << std::endl;
    std::cout << "\n📷 Visual Output Summary:" << std::endl;
    std::cout << "  📁 test_results/" << std::endl;
    std::cout << "    └── preprocess_frame/        - Original and preprocessed frames" << std::endl;
    std::cout << "    └── detect_motion/           - Motion detection sequence" << std::endl;
    std::cout << "    └── morphological_ops/       - Before/after morphological operations" << std::endl;
    std::cout << "    └── extract_contours/        - Contour detection with bounding boxes" << std::endl;
    std::cout << "\n✨ All test results organized in structured folders!" << std::endl;
    return 0;
}