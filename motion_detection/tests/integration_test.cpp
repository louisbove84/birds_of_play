#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <map>

#include "motion_processor.hpp"
#include "motion_region_consolidator.hpp"
#include "object_tracker.hpp"
#include "logger.hpp"

namespace fs = std::filesystem;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger
        initLogger();
        
        // Create output directories
        fs::create_directories("test_results/integration_test/1_motion_processor_visualizations");
        fs::create_directories("test_results/integration_test/2_consolidation_visualizations");
        
        // Initialize motion processor with test config
        motionProcessor = std::make_unique<MotionProcessor>("config.yaml");
        
        // Initialize consolidation config
        ConsolidationConfig config;
        config.maxDistanceThreshold = 100.0;
        config.minObjectsPerRegion = 2;
        config.regionExpansionFactor = 1.2;
        config.minRegionArea = 1000;
        config.maxRegionArea = 1000000;
        config.overlapThreshold = 0.3;
        config.maxFramesWithoutUpdate = 30;
        config.idealModelRegionSize = 640;
        config.gridCellSize = 50;
        config.frameSize = cv::Size(1920, 1080);
        
        regionConsolidator = std::make_unique<MotionRegionConsolidator>(config);
        
        // Collect all test images from all subdirectories
        collectAllTestImages();
    }
    
    void TearDown() override {
        spdlog::shutdown();
    }
    
    void initLogger() {
        try {
            Logger::init("debug", "test_results/integration_test/integration_test.log", true);
        } catch (const std::exception& e) {
            // Logger already initialized, ignore
        }
    }
    
    void collectAllTestImages() {
        testImages.clear();
        
        // Recursively collect all images from tests/img directory
        std::string baseDir = "tests/img";
        if (fs::exists(baseDir)) {
            for (const auto& entry : fs::recursive_directory_iterator(baseDir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".jpg" || ext == ".png" || ext == ".jpeg") {
                        // Create a clean name from the relative path
                        fs::path relativePath = fs::relative(entry.path(), baseDir);
                        std::string cleanName = relativePath.string();
                        std::replace(cleanName.begin(), cleanName.end(), '/', '_');
                        
                        testImages.push_back({
                            entry.path().string(),
                            cleanName
                        });
                    }
                }
            }
        }
        
        LOG_INFO("Found {} test images for integration testing", testImages.size());
    }
    
    // Helper function to process real images and generate TrackedObjects (adapted from motion_region_consolidator_test.cpp)
    std::vector<TrackedObject> processRealImages(const std::string& image1Path, 
                                               const std::string& image2Path) {
        std::vector<TrackedObject> trackedObjects;
        
        // Load the test images
        cv::Mat frame1 = cv::imread(image1Path);
        cv::Mat frame2 = cv::imread(image2Path);
        
        if (frame1.empty() || frame2.empty()) {
            LOG_ERROR("Failed to load test images: {} or {}", image1Path, image2Path);
            return trackedObjects;
        }
        
        LOG_INFO("Loaded test images: {}x{} and {}x{}", 
                 frame1.cols, frame1.rows, frame2.cols, frame2.rows);
        
        // Reset motion processor for clean processing
        motionProcessor->setPrevFrame(cv::Mat());
        
        // Process first frame (establishes baseline)
        MotionProcessor::ProcessingResult result1 = motionProcessor->processFrame(frame1);
        LOG_INFO("First frame processed - hasMotion: {}, bounds: {}", 
                 result1.hasMotion, result1.detectedBounds.size());
        
        // Process second frame (detects motion between frames)
        MotionProcessor::ProcessingResult result2 = motionProcessor->processFrame(frame2);
        LOG_INFO("Second frame processed - hasMotion: {}, bounds: {}", 
                 result2.hasMotion, result2.detectedBounds.size());
        
        // Convert detected bounds to TrackedObjects
        int objectId = 0;
        for (const auto& bounds : result2.detectedBounds) {
            trackedObjects.emplace_back(objectId, bounds, "uuid_" + std::to_string(objectId));
            objectId++;
        }
        
        LOG_INFO("Generated {} TrackedObjects from motion detection", trackedObjects.size());
        return trackedObjects;
    }
    
    void processImagePairWithBothComponents(const std::string& image1Path, const std::string& image2Path, const std::string& pairName) {
        LOG_INFO("Processing image pair: {} (using two-frame motion detection)", pairName);
        
        // Use processRealImages to get proper motion detection between two frames
        std::vector<TrackedObject> trackedObjects = processRealImages(image1Path, image2Path);
        
        // Load second image for visualization (the one with detected motion)
        cv::Mat frame2 = cv::imread(image2Path);
        if (frame2.empty()) {
            LOG_ERROR("Failed to load second image for visualization: {}", image2Path);
            return;
        }
        
        // Step 1: Save MotionProcessor visualization (using the two-frame processing result)
        // We need to reprocess to get the ProcessingResult for visualization
        motionProcessor->setPrevFrame(cv::Mat());
        cv::Mat frame1 = cv::imread(image1Path);
        if (!frame1.empty()) {
            motionProcessor->processFrame(frame1);  // Baseline frame
            auto result2 = motionProcessor->processFrame(frame2);  // Motion detection frame
            
            std::string motionProcessorOutput = "test_results/integration_test/1_motion_processor_visualizations/" + 
                                               pairName + "_motion_processing.jpg";
            motionProcessor->saveProcessingVisualization(result2, motionProcessorOutput);
        }
        
        // Step 2: Process with MotionRegionConsolidator and save its visualization
        std::string consolidationOutput = "test_results/integration_test/2_consolidation_visualizations/" + 
                                         pairName + "_consolidation.jpg";
        
        // Use MotionRegionConsolidator's consolidateRegionsWithVisualization method
        auto regions = regionConsolidator->consolidateRegionsWithVisualization(
            trackedObjects, frame2, consolidationOutput);
        
        LOG_INFO("Completed {}: {} motion objects -> {} consolidated regions", 
                pairName, trackedObjects.size(), regions.size());
    }
    
    std::unique_ptr<MotionProcessor> motionProcessor;
    std::unique_ptr<MotionRegionConsolidator> regionConsolidator;
    std::vector<std::pair<std::string, std::string>> testImages; // {path, name}
};

// Global test environment for logger initialization
class IntegrationTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Global setup if needed
    }
    
    void TearDown() override {
        spdlog::shutdown();
    }
};

TEST_F(IntegrationTest, ProcessAllTestImages) {
    LOG_INFO("=== STARTING INTEGRATION TEST ===");
    LOG_INFO("Processing {} test images with both MotionProcessor and MotionRegionConsolidator", testImages.size());
    
    EXPECT_GT(testImages.size(), 0) << "No test images found";
    
    // Group images by directory for pair processing
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> imagesByDir;
    for (const auto& [imagePath, imageName] : testImages) {
        // Extract directory from image name (e.g., "1_test_image.jpg" -> "1")
        std::string dir = imageName.substr(0, imageName.find('_'));
        imagesByDir[dir].push_back({imagePath, imageName});
    }
    
    // Process image pairs for better motion detection
    for (const auto& [dir, images] : imagesByDir) {
        if (images.size() >= 2) {
            // Process pairs of images for motion detection
            for (size_t i = 0; i < images.size() - 1; ++i) {
                const auto& [image1Path, image1Name] = images[i];
                const auto& [image2Path, image2Name] = images[i + 1];
                
                std::string pairName = dir + "_pair_" + std::to_string(i) + "_" + std::to_string(i + 1);
                LOG_INFO("--- Processing image pair: {} vs {} ---", image1Name, image2Name);
                processImagePairWithBothComponents(image1Path, image2Path, pairName);
            }
        } 
    }
    
    LOG_INFO("=== INTEGRATION TEST COMPLETED ===");
    LOG_INFO("Generated visualizations in:");
    LOG_INFO("  - Motion Processing: test_results/integration_test/1_motion_processor_visualizations/");
    LOG_INFO("  - Consolidation: test_results/integration_test/2_consolidation_visualizations/");
}

TEST_F(IntegrationTest, VerifyOutputFiles) {
    // Process a few image pairs and verify output files are created
    int processedCount = 0;
    const int maxToProcess = 2; // Limit for this test
    
    // Group images by directory for pair processing
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> imagesByDir;
    for (const auto& [imagePath, imageName] : testImages) {
        std::string dir = imageName.substr(0, imageName.find('_'));
        imagesByDir[dir].push_back({imagePath, imageName});
    }
    
    // Process image pairs only
    for (const auto& [dir, images] : imagesByDir) {
        if (images.size() >= 2 && processedCount < maxToProcess) {
            const auto& [image1Path, image1Name] = images[0];
            const auto& [image2Path, image2Name] = images[1];
            
            std::string pairName = dir + "_pair_0_1";
            processImagePairWithBothComponents(image1Path, image2Path, pairName);
            
            // Verify output files exist
            std::string motionOutput = "test_results/integration_test/1_motion_processor_visualizations/" + 
                                      pairName + "_motion_processing.jpg";
            std::string consolidationOutput = "test_results/integration_test/2_consolidation_visualizations/" + 
                                             pairName + "_consolidation.jpg";
            
            EXPECT_TRUE(fs::exists(motionOutput)) << "Motion processor visualization missing: " << motionOutput;
            EXPECT_TRUE(fs::exists(consolidationOutput)) << "Consolidation visualization missing: " << consolidationOutput;
            
            processedCount++;
        }
    }
    
    EXPECT_GT(processedCount, 0) << "No image pairs were processed for verification";
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add global test environment
    ::testing::AddGlobalTestEnvironment(new IntegrationTestEnvironment());
    
    return RUN_ALL_TESTS();
}
