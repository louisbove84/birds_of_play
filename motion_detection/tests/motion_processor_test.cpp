#include "motion_processor.hpp"
#include "logger.hpp"
#include <opencv2/opencv.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>

/**
 * Motion Processor Test Suite
 * 
 * This test suite verifies the functionality of the MotionProcessor class.
 * Each test enables visualization mode for debugging and saves intermediate
 * results to the test_results directory.
 * 
 * Visualization System:
 * - The MotionProcessor has a visualization system that can be enabled/disabled
 * - When enabled (via enableVisualization()), it saves debug images showing:
 *   - Contours (red = detected, green = passed filters)
 *   - Bounding boxes (blue)
 *   - Statistics (area, solidity)
 * - In production, keep visualization disabled for better performance
 * - Enable only during testing/debugging when visual feedback is needed
 */

void initLogger() {
    Logger::init("debug", "test_log.txt", true);
}

/**
 * Creates a test directory and configures the processor for visualization
 * @param processor The MotionProcessor instance to configure
 * @param testName The name of the test (used for directory structure)
 */
void setupTestVisualization(MotionProcessor& processor, const std::string& testName) {
    std::string testPath = "test_results/motion_processor/" + testName;
    std::filesystem::create_directories(testPath);
    processor.enableVisualization(true);
    processor.setVisualizationPath(testPath);
}

// Test preprocessFrame
void testPreprocessFrame() {
    MotionProcessor processor("config.yaml");
    setupTestVisualization(processor, "01_preprocess_frame");
    cv::Mat frame = cv::imread("test_image.jpg");
    assert(!frame.empty() && "Failed to load test_image.jpg");

    cv::Mat processed = processor.preprocessFrame(frame);
    assert(!processed.empty() && "preprocessFrame returned empty frame");
    assert(processed.type() == CV_8UC1 && "Frame should be single-channel (grayscale or HSV)");
    assert(processed.size() == frame.size() && "Frame size should match");
    
    // Save output images for visual inspection
    cv::imwrite("test_results/motion_processor/01_preprocess_frame/original_frame.jpg", frame);
    cv::imwrite("test_results/motion_processor/01_preprocess_frame/preprocessed_frame.jpg", processed);
    std::cout << "✓ Saved preprocessing results to test_results/motion_processor/01_preprocess_frame/" << std::endl;
}

// Test detectMotion
void testDetectMotion() {
    MotionProcessor processor("config.yaml");
    setupTestVisualization(processor, "02_detect_motion");
    cv::Mat frame1 = cv::imread("test_image.jpg");
    cv::Mat frame2 = cv::imread("test_image2.jpg");
    assert(!frame1.empty() && !frame2.empty() && "Failed to load test images");

    cv::Mat processed1 = processor.preprocessFrame(frame1);
    cv::Mat processed2 = processor.preprocessFrame(frame2);
    
    processor.setPrevFrame(processed1);
    processor.setFirstFrame(false); // Ensure we're not in first frame mode
    
    cv::Mat frameDiff, thresh;
    cv::Mat motionMask = processor.detectMotion(processed2, frameDiff, thresh);
    
    assert(!motionMask.empty() && "Motion mask should not be empty");
    assert(motionMask.type() == CV_8UC1 && "Motion mask should be single-channel");
    assert(!frameDiff.empty() && "Frame difference should not be empty");
    assert(!thresh.empty() && "Thresholded mask should not be empty");
    
    // Save motion detection outputs
    cv::imwrite("test_results/motion_processor/02_detect_motion/frame1_reference.jpg", frame1);
    cv::imwrite("test_results/motion_processor/02_detect_motion/frame2_comparison.jpg", frame2);
    cv::imwrite("test_results/motion_processor/02_detect_motion/processed1.jpg", processed1);
    cv::imwrite("test_results/motion_processor/02_detect_motion/processed2.jpg", processed2);
    cv::imwrite("test_results/motion_processor/02_detect_motion/frame_difference.jpg", frameDiff);
    cv::imwrite("test_results/motion_processor/02_detect_motion/threshold_mask.jpg", thresh);
    cv::imwrite("test_results/motion_processor/02_detect_motion/motion_mask_final.jpg", motionMask);
    std::cout << "✓ Saved motion detection results to test_results/motion_processor/02_detect_motion/" << std::endl;
}

// Test applyMorphologicalOps
void testApplyMorphologicalOps() {
    MotionProcessor processor("config.yaml");
    setupTestVisualization(processor, "03_morphological_ops");
    cv::Mat thresh = cv::imread("motion_mask.png", cv::IMREAD_GRAYSCALE);
    assert(!thresh.empty() && "Failed to load motion_mask.png");

    cv::Mat processed = processor.applyMorphologicalOps(thresh);
    assert(!processed.empty() && "Processed mask should not be empty");
    assert(processed.type() == CV_8UC1 && "Processed mask should be single-channel");
    assert(processed.size() == thresh.size() && "Processed mask size should match");
    
    // Save morphological operations output
    cv::imwrite("test_results/motion_processor/03_morphological_ops/input_mask.jpg", thresh);
    cv::imwrite("test_results/motion_processor/03_morphological_ops/processed_mask.jpg", processed);
    std::cout << "✓ Saved morphological operations results to test_results/motion_processor/03_morphological_ops/" << std::endl;
}

// Test extractContours
void testExtractContours() {
    MotionProcessor processor("config.yaml");
    setupTestVisualization(processor, "04_extract_contours");
    cv::Mat processed = cv::imread("motion_mask.png", cv::IMREAD_GRAYSCALE);
    assert(!processed.empty() && "Failed to load motion_mask.png");

    std::vector<cv::Rect> newBounds = processor.extractContours(processed);
    assert(!newBounds.empty() && "Contours should be detected");
    
    // Create visualization with bounding boxes
    cv::Mat visualization;
    cv::cvtColor(processed, visualization, cv::COLOR_GRAY2BGR);
    for (const auto& rect : newBounds) {
        assert(rect.width > 0 && rect.height > 0 && "Invalid bounding box");
        std::cout << "Contour bounds: [" << rect.width << " x " << rect.height 
                  << " from (" << rect.x << ", " << rect.y << ")]" << std::endl;
        // Draw bounding box in red
        cv::rectangle(visualization, rect, cv::Scalar(0, 0, 255), 2);
        // Add text label
        cv::putText(visualization, std::to_string(rect.width) + "x" + std::to_string(rect.height), 
                   cv::Point(rect.x, rect.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
    }
    
    // Save contour detection outputs
    cv::imwrite("test_results/motion_processor/04_extract_contours/input_mask.jpg", processed);
    cv::imwrite("test_results/motion_processor/04_extract_contours/contours_with_bounding_boxes.jpg", visualization);
    std::cout << "✓ Saved contour extraction results to test_results/motion_processor/04_extract_contours/" << std::endl;
}

// Test complete processing pipeline
void testCompleteProcessingPipeline() {
    MotionProcessor processor("config.yaml");
    setupTestVisualization(processor, "05_complete_pipeline");
    cv::Mat frame1 = cv::imread("test_image.jpg");
    cv::Mat frame2 = cv::imread("test_image2.jpg");
    assert(!frame1.empty() && !frame2.empty() && "Failed to load test images");
    
    // Process first frame (should return empty result due to firstFrame flag)
    MotionProcessor::ProcessingResult result1 = processor.processFrame(frame1);
    assert(!result1.processedFrame.empty() && "First frame should be processed");
    assert(result1.detectedBounds.empty() && "First frame should have no detected bounds");
    assert(result1.hasMotion == false && "First frame should have no motion");
    
    // Process second frame (should detect motion)
    MotionProcessor::ProcessingResult result2 = processor.processFrame(frame2);
    assert(!result2.processedFrame.empty() && "Second frame should be processed");
    assert(!result2.frameDiff.empty() && "Frame difference should be computed");
    assert(!result2.thresh.empty() && "Threshold should be computed");
    assert(!result2.morphological.empty() && "Morphological processing should be done");
    
    std::cout << "Motion detected: " << (result2.hasMotion ? "Yes" : "No") << std::endl;
    std::cout << "Detected bounds: " << result2.detectedBounds.size() << std::endl;
    
    // Save complete pipeline results
    cv::imwrite("test_results/motion_processor/05_complete_pipeline/frame1_original.jpg", frame1);
    cv::imwrite("test_results/motion_processor/05_complete_pipeline/frame2_original.jpg", frame2);
    cv::imwrite("test_results/motion_processor/05_complete_pipeline/frame1_processed.jpg", result1.processedFrame);
    cv::imwrite("test_results/motion_processor/05_complete_pipeline/frame2_processed.jpg", result2.processedFrame);
    
    if (!result2.frameDiff.empty()) {
        cv::imwrite("test_results/motion_processor/05_complete_pipeline/frame_difference.jpg", result2.frameDiff);
    }
    if (!result2.thresh.empty()) {
        cv::imwrite("test_results/motion_processor/05_complete_pipeline/threshold.jpg", result2.thresh);
    }
    if (!result2.morphological.empty()) {
        cv::imwrite("test_results/motion_processor/05_complete_pipeline/morphological.jpg", result2.morphological);
    }
    
    // Create visualization with detected bounds if any
    if (!result2.detectedBounds.empty()) {
        cv::Mat boundsViz = frame2.clone();
        for (const auto& rect : result2.detectedBounds) {
            cv::rectangle(boundsViz, rect, cv::Scalar(0, 255, 0), 2);
        }
        cv::imwrite("test_results/motion_processor/05_complete_pipeline/detected_bounds.jpg", boundsViz);
    }
    
    std::cout << "✓ Saved complete pipeline results to test_results/motion_processor/05_complete_pipeline/" << std::endl;
}

// Test frame management functions
void testFrameManagement() {
    MotionProcessor processor("config.yaml");
    cv::Mat testFrame = cv::Mat::ones(100, 100, CV_8UC1) * 128;
    
    // Test initial state
    assert(processor.isFirstFrame() == true && "Should start with first frame flag");
    
    // Set previous frame
    processor.setPrevFrame(testFrame);
    
    // Test setFirstFrame
    processor.setFirstFrame(false);
    assert(processor.isFirstFrame() == false && "First frame flag should be false");
    
    processor.setFirstFrame(true);
    assert(processor.isFirstFrame() == true && "First frame flag should be true");
    
    std::cout << "✓ Frame management functions work correctly" << std::endl;
}

// Test configuration getters
void testConfigurationGetters() {
    MotionProcessor processor("config.yaml");
    
    int minArea = processor.getMinContourArea();
    int maxThresh = processor.getMaxThreshold();
    bool bgSub = processor.isBackgroundSubtractionEnabled();
    
    assert(minArea > 0 && "Min contour area should be positive");
    assert(maxThresh > 0 && "Max threshold should be positive");
    
    std::cout << "Min contour area: " << minArea << std::endl;
    std::cout << "Max threshold: " << maxThresh << std::endl;
    std::cout << "Background subtraction enabled: " << (bgSub ? "Yes" : "No") << std::endl;
}

int main() {
    std::cout << "Running MotionProcessor tests..." << std::endl;
    initLogger();
    
    testPreprocessFrame();
    std::cout << "testPreprocessFrame passed" << std::endl;
    
    testDetectMotion();
    std::cout << "testDetectMotion passed" << std::endl;
    
    testApplyMorphologicalOps();
    std::cout << "testApplyMorphologicalOps passed" << std::endl;
    
    testExtractContours();
    std::cout << "testExtractContours passed" << std::endl;
    
    testCompleteProcessingPipeline();
    std::cout << "testCompleteProcessingPipeline passed" << std::endl;
    
    testFrameManagement();
    std::cout << "testFrameManagement passed" << std::endl;
    
    testConfigurationGetters();
    std::cout << "testConfigurationGetters passed" << std::endl;
    
    std::cout << "All tests passed!" << std::endl;
    std::cout << "\n📷 Visual Output Summary:" << std::endl;
    std::cout << "  📁 test_results/motion_processor/" << std::endl;
    std::cout << "    ├── 01_preprocess_frame/     - Original and preprocessed frames" << std::endl;
    std::cout << "    ├── 02_detect_motion/        - Motion detection sequence" << std::endl;
    std::cout << "    ├── 03_morphological_ops/    - Before/after morphological operations" << std::endl;
    std::cout << "    ├── 04_extract_contours/     - Contour detection with bounding boxes" << std::endl;
    std::cout << "    └── 05_complete_pipeline/    - End-to-end processing pipeline" << std::endl;
    std::cout << "\n✨ All MotionProcessor functions comprehensively tested!" << std::endl;
    return 0;
}
