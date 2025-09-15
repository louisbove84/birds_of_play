#include <gtest/gtest.h>
#include "motion_processor.hpp"
#include "logger.hpp"
#include "test_helpers.hpp"
#include <opencv2/opencv.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>

void initLogger() {
    try {
        Logger::init("debug", "motion_processor_test_log.txt", true);
    } catch (const std::exception& e) {
        // Logger already initialized, ignore
    }
}

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

// ============================================================================
// MANUAL TESTING FUNCTIONS (Legacy)
// ============================================================================

// Test preprocessFrame
void testPreprocessFrame() {
    MotionProcessor processor("config.yaml");
    setupTestVisualization(processor, "01_preprocess_frame");
    cv::Mat frame = cv::imread("1/test_image.jpg");
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
    cv::Mat frame1 = cv::imread("1/test_image.jpg");
    cv::Mat frame2 = cv::imread("1/test_image2.jpg");
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
    cv::Mat frame1 = cv::imread("1/test_image.jpg");
    cv::Mat frame2 = cv::imread("1/test_image2.jpg");
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

// ============================================================================
// GOOGLE TEST FRAMEWORK TESTS
// ============================================================================

class MotionProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger
        initLogger();

        // Find test resource directory
        std::string testDir = findTestResourceDir();

        // Set up test paths relative to the test resource directory
        configPath = testDir + "/config.yaml";
        testImage1Path = testDir + "/img/1/test_image.jpg";
        testImage2Path = testDir + "/img/1/test_image2.jpg";

        // Create output directory
        outputDir = "test_results/motion_processor/06_google_test_mode";
        std::filesystem::create_directories(outputDir);

        // Initialize motion processor
        motionProcessor = std::make_unique<MotionProcessor>(configPath);
        motionProcessor->enableVisualization(true);
        motionProcessor->setVisualizationPath(outputDir);
    }
    
    void TearDown() override {
        spdlog::shutdown();
    }
    
    std::string configPath;
    std::string testImage1Path;
    std::string testImage2Path;
    std::string outputDir;
    std::unique_ptr<MotionProcessor> motionProcessor;
};

// Test standalone motion processing with visualization
TEST_F(MotionProcessorTest, StandaloneProcessingWithVisualization) {
    // Load test images
    cv::Mat frame1 = cv::imread(testImage1Path);
    cv::Mat frame2 = cv::imread(testImage2Path);
    
    ASSERT_FALSE(frame1.empty()) << "Failed to load " << testImage1Path;
    ASSERT_FALSE(frame2.empty()) << "Failed to load " << testImage2Path;
    
    LOG_INFO("Processing frame 1 (baseline)");
    MotionProcessor::ProcessingResult result1 = motionProcessor->processFrame(frame1);
    
    // Save visualization for first frame
    std::string outputPath1 = outputDir + "/01_baseline_frame_processing.jpg";
    motionProcessor->saveProcessingVisualization(result1, outputPath1);
    
    LOG_INFO("Processing frame 2 (motion detection)");
    MotionProcessor::ProcessingResult result2 = motionProcessor->processFrame(frame2);
    
    // Save visualization for second frame
    std::string outputPath2 = outputDir + "/02_motion_detection_processing.jpg";
    motionProcessor->saveProcessingVisualization(result2, outputPath2);
    
    // Verify results
    EXPECT_FALSE(result1.hasMotion) << "First frame should not have motion";
    EXPECT_TRUE(result2.hasMotion) << "Second frame should have motion";
    EXPECT_GT(result2.detectedBounds.size(), 0) << "Should detect motion regions";
    
    LOG_INFO("Motion detection results:");
    LOG_INFO("Frame 1 - Motion: {}, Regions: {}", result1.hasMotion, result1.detectedBounds.size());
    LOG_INFO("Frame 2 - Motion: {}, Regions: {}", result2.hasMotion, result2.detectedBounds.size());
    LOG_INFO("Visualizations saved to: {} and {}", outputPath1, outputPath2);
    
    // Verify output files exist
    EXPECT_TRUE(std::filesystem::exists(outputPath1)) << "Frame 1 visualization not created";
    EXPECT_TRUE(std::filesystem::exists(outputPath2)) << "Frame 2 visualization not created";
}

// Test individual processing steps
TEST_F(MotionProcessorTest, IndividualProcessingSteps) {
    cv::Mat frame1 = cv::imread(testImage1Path);
    cv::Mat frame2 = cv::imread(testImage2Path);
    ASSERT_FALSE(frame1.empty()) << "Failed to load test image 1";
    ASSERT_FALSE(frame2.empty()) << "Failed to load test image 2";
    
    // Test preprocessing
    cv::Mat processed1 = motionProcessor->preprocessFrame(frame1);
    cv::Mat processed2 = motionProcessor->preprocessFrame(frame2);
    EXPECT_FALSE(processed1.empty()) << "Preprocessing frame 1 failed";
    EXPECT_FALSE(processed2.empty()) << "Preprocessing frame 2 failed";
    
    // Set up motion detection by setting previous frame
    motionProcessor->setPrevFrame(processed1);
    motionProcessor->setFirstFrame(false);
    
    // Test motion detection
    cv::Mat frameDiff, thresh;
    cv::Mat motionResult = motionProcessor->detectMotion(processed2, frameDiff, thresh);
    EXPECT_FALSE(motionResult.empty()) << "Motion detection failed";
    
    // Test morphological operations
    cv::Mat morphResult = motionProcessor->applyMorphologicalOps(thresh);
    EXPECT_FALSE(morphResult.empty()) << "Morphological operations failed";
    
    // Test contour extraction
    std::vector<cv::Rect> contours = motionProcessor->extractContours(morphResult);
    EXPECT_GT(contours.size(), 0) << "Should extract contours";
    
    LOG_INFO("Individual processing steps completed successfully");
    LOG_INFO("Contours extracted: {}", contours.size());
}

// Test configuration parameters
TEST_F(MotionProcessorTest, ConfigurationParameters) {
    EXPECT_GT(motionProcessor->getMinContourArea(), 0) << "Min contour area should be positive";
    EXPECT_GT(motionProcessor->getMaxThreshold(), 0) << "Max threshold should be positive";
    EXPECT_FALSE(motionProcessor->isBackgroundSubtractionEnabled()) << "Background subtraction should be disabled by default";
    
    LOG_INFO("Configuration parameters verified");
    LOG_INFO("Min contour area: {}", motionProcessor->getMinContourArea());
    LOG_INFO("Max threshold: {}", motionProcessor->getMaxThreshold());
}

// Test frame management
TEST_F(MotionProcessorTest, FrameManagement) {
    cv::Mat testFrame = cv::Mat::ones(100, 100, CV_8UC1) * 128;
    
    // Test initial state
    EXPECT_TRUE(motionProcessor->isFirstFrame()) << "Should start with first frame flag";
    
    // Set previous frame
    motionProcessor->setPrevFrame(testFrame);
    
    // Test setFirstFrame
    motionProcessor->setFirstFrame(false);
    EXPECT_FALSE(motionProcessor->isFirstFrame()) << "First frame flag should be false";
    
    motionProcessor->setFirstFrame(true);
    EXPECT_TRUE(motionProcessor->isFirstFrame()) << "First frame flag should be true";
    
    LOG_INFO("Frame management functions work correctly");
}

// Test complete processing pipeline with Google Test
TEST_F(MotionProcessorTest, CompleteProcessingPipeline) {
    cv::Mat frame1 = cv::imread(testImage1Path);
    cv::Mat frame2 = cv::imread(testImage2Path);
    
    ASSERT_FALSE(frame1.empty()) << "Failed to load " << testImage1Path;
    ASSERT_FALSE(frame2.empty()) << "Failed to load " << testImage2Path;
    
    // Process first frame (should return empty result due to firstFrame flag)
    MotionProcessor::ProcessingResult result1 = motionProcessor->processFrame(frame1);
    EXPECT_FALSE(result1.processedFrame.empty()) << "First frame should be processed";
    EXPECT_TRUE(result1.detectedBounds.empty()) << "First frame should have no detected bounds";
    EXPECT_FALSE(result1.hasMotion) << "First frame should have no motion";
    
    // Process second frame (should detect motion)
    MotionProcessor::ProcessingResult result2 = motionProcessor->processFrame(frame2);
    EXPECT_FALSE(result2.processedFrame.empty()) << "Second frame should be processed";
    EXPECT_FALSE(result2.frameDiff.empty()) << "Frame difference should be computed";
    EXPECT_FALSE(result2.thresh.empty()) << "Threshold should be computed";
    EXPECT_FALSE(result2.morphological.empty()) << "Morphological processing should be done";
    
    LOG_INFO("Motion detected: {}", result2.hasMotion ? "Yes" : "No");
    LOG_INFO("Detected bounds: {}", result2.detectedBounds.size());
    
    // Save complete pipeline results
    cv::imwrite(outputDir + "/03_frame1_original.jpg", frame1);
    cv::imwrite(outputDir + "/04_frame2_original.jpg", frame2);
    cv::imwrite(outputDir + "/05_frame1_preprocessed.jpg", result1.processedFrame);
    cv::imwrite(outputDir + "/06_frame2_preprocessed.jpg", result2.processedFrame);
    
    if (!result2.frameDiff.empty()) {
        cv::imwrite(outputDir + "/07_frame_difference.jpg", result2.frameDiff);
    }
    if (!result2.thresh.empty()) {
        cv::imwrite(outputDir + "/08_threshold_mask.jpg", result2.thresh);
    }
    if (!result2.morphological.empty()) {
        cv::imwrite(outputDir + "/09_morphological_cleaned.jpg", result2.morphological);
    }
    
    // Create visualization with detected bounds if any
    if (!result2.detectedBounds.empty()) {
        cv::Mat boundsViz = frame2.clone();
        for (const auto& rect : result2.detectedBounds) {
            cv::rectangle(boundsViz, rect, cv::Scalar(0, 255, 0), 2);
        }
        cv::imwrite(outputDir + "/10_final_detected_bounds.jpg", boundsViz);
    }
    
    LOG_INFO("Complete pipeline results saved to: {}", outputDir);
}

// Global test environment setup
class MotionProcessorTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        initLogger();
    }
    
    void TearDown() override {
        spdlog::shutdown();
    }
};

// ============================================================================
// MAIN FUNCTION - SUPPORTS BOTH MANUAL AND GOOGLE TEST MODES
// ============================================================================

int main(int argc, char** argv) {
    // Check if running in Google Test mode
    if (argc > 1 && std::string(argv[1]) == "--gtest") {
        // Google Test mode
        ::testing::InitGoogleTest(&argc, argv);
        ::testing::AddGlobalTestEnvironment(new MotionProcessorTestEnvironment());
        return RUN_ALL_TESTS();
    } else {
        // Manual test mode (legacy)
        std::cout << "Running MotionProcessor manual tests..." << std::endl;
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
        
        std::cout << "All manual tests passed!" << std::endl;
        std::cout << "\n📷 Visual Output Summary:" << std::endl;
        std::cout << "  📁 test_results/motion_processor/" << std::endl;
        std::cout << "    ├── 01_preprocess_frame/     - Original and preprocessed frames" << std::endl;
        std::cout << "    ├── 02_detect_motion/        - Motion detection sequence" << std::endl;
        std::cout << "    ├── 03_morphological_ops/    - Before/after morphological operations" << std::endl;
        std::cout << "    ├── 04_extract_contours/     - Contour detection with bounding boxes" << std::endl;
        std::cout << "    └── 05_complete_pipeline/    - End-to-end processing pipeline" << std::endl;
        std::cout << "\n✨ All MotionProcessor functions comprehensively tested!" << std::endl;
        std::cout << "\n💡 To run Google Test mode: ./motion_processor_test --gtest" << std::endl;
        return 0;
    }
}
