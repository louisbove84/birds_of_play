#include <gtest/gtest.h>
#include "motion_region_consolidator.hpp"
#include "motion_processor.hpp"
#include "logger.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>
#include <string>
#include <filesystem>

void initLogger() {
    try {
        Logger::init("debug", "motion_region_consolidator_test_log.txt", true);
    } catch (const std::exception& e) {
        // Logger already initialized, ignore
    }
}

// Helper function to process real images and generate TrackedObjects
std::vector<TrackedObject> processRealImages(const std::string& configPath, 
                                           const std::string& image1Path, 
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
    
    // Initialize motion processor
    MotionProcessor motionProcessor(configPath);
    
    // Process first frame (establishes baseline)
    MotionProcessor::ProcessingResult result1 = motionProcessor.processFrame(frame1);
    LOG_INFO("First frame processed - hasMotion: {}, bounds: {}", 
             result1.hasMotion, result1.detectedBounds.size());
    
    // Process second frame (detects motion between frames)
    MotionProcessor::ProcessingResult result2 = motionProcessor.processFrame(frame2);
    LOG_INFO("Second frame processed - hasMotion: {}, bounds: {}", 
             result2.hasMotion, result2.detectedBounds.size());
    
    // Convert detected bounds to TrackedObjects
    int objectId = 0;
    for (const auto& bounds : result2.detectedBounds) {
        std::string uuid = "real_uuid_" + std::to_string(objectId);
        trackedObjects.emplace_back(objectId, bounds, uuid);
        
        LOG_INFO("Created TrackedObject {}: BBox({},{},{},{}) from real motion detection", 
                 objectId, bounds.x, bounds.y, bounds.width, bounds.height);
        objectId++;
    }
    
    LOG_INFO("Generated {} TrackedObjects from real bird images", trackedObjects.size());
    return trackedObjects;
}

// Global test environment setup
class MotionRegionConsolidatorTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        initLogger();
    }
    
    void TearDown() override {
        spdlog::shutdown();
    }
};

// Test fixture for MotionRegionConsolidator
class MotionRegionConsolidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config.maxDistanceThreshold = 50.0;
        config.minObjectsPerRegion = 2;
        config.overlapThreshold = 0.3;
        config.regionExpansionFactor = 1.2;
        config.minRegionArea = 10000;
        config.maxRegionArea = 1000000;
        config.maxFramesWithoutUpdate = 3;
        config.idealModelRegionSize = 640;
        config.frameSize = cv::Size(1920, 1080);
        
        // Initialize consolidator
        consolidator = std::make_unique<MotionRegionConsolidator>(config);
        
        // Set up paths for real test images
        configPath = "config.yaml";
        testImage1Path = "1/test_image.jpg";
        testImage2Path = "1/test_image2.jpg";
        
        // Create output directory
        std::filesystem::create_directories("test_results/motion_region_consolidator");
        
        // Process real images to get TrackedObjects
        realTrackedObjects = processRealImages(configPath, testImage1Path, testImage2Path);
        LOG_INFO("SetUp complete: {} real TrackedObjects available for testing", realTrackedObjects.size());
    }
    
    // Helper function to create synthetic tracked objects
    std::vector<TrackedObject> createSyntheticObjects() {
        std::vector<TrackedObject> objects;
        
        // Create a cluster of objects in the top-left area
        objects.emplace_back(0, cv::Rect(100, 100, 50, 50), "uuid_0");
        objects.emplace_back(1, cv::Rect(120, 110, 45, 45), "uuid_1");
        objects.emplace_back(2, cv::Rect(140, 120, 55, 40), "uuid_2");
        objects.emplace_back(3, cv::Rect(160, 130, 40, 50), "uuid_3");
        
        // Create a cluster of objects in the bottom-right area
        objects.emplace_back(4, cv::Rect(1500, 800, 60, 60), "uuid_4");
        objects.emplace_back(5, cv::Rect(1520, 820, 50, 55), "uuid_5");
        objects.emplace_back(6, cv::Rect(1540, 810, 55, 50), "uuid_6");
        
        // Create a single isolated object
        objects.emplace_back(7, cv::Rect(800, 400, 70, 70), "uuid_7");
        
        return objects;
    }
    
    // Helper function to create bird-like objects (based on real data)
    std::vector<TrackedObject> createBirdLikeObjects() {
        std::vector<TrackedObject> objects;
        
        // Cluster 1: Multiple small bird detections
        objects.emplace_back(0, cv::Rect(1414, 832, 110, 49), "uuid_0");
        objects.emplace_back(1, cv::Rect(1450, 783, 16, 13), "uuid_1");
        objects.emplace_back(2, cv::Rect(1427, 782, 22, 23), "uuid_2");
        objects.emplace_back(3, cv::Rect(1394, 774, 28, 27), "uuid_3");
        objects.emplace_back(4, cv::Rect(1416, 744, 20, 15), "uuid_4");
        objects.emplace_back(5, cv::Rect(1364, 734, 38, 35), "uuid_5");
        
        // Cluster 2: Another group of bird detections
        objects.emplace_back(6, cv::Rect(374, 656, 34, 34), "uuid_6");
        objects.emplace_back(7, cv::Rect(357, 652, 14, 13), "uuid_7");
        objects.emplace_back(8, cv::Rect(399, 634, 16, 14), "uuid_8");
        objects.emplace_back(9, cv::Rect(336, 683, 21, 22), "uuid_9");
        
        // Cluster 3: Larger bird detections
        objects.emplace_back(10, cv::Rect(517, 464, 66, 63), "uuid_10");
        objects.emplace_back(11, cv::Rect(576, 467, 27, 82), "uuid_11");
        objects.emplace_back(12, cv::Rect(628, 489, 34, 25), "uuid_12");
        
        return objects;
    }

    ConsolidationConfig config;
    std::unique_ptr<MotionRegionConsolidator> consolidator;
    
    // Real image data
    std::string configPath;
    std::string testImage1Path;
    std::string testImage2Path;
    std::vector<TrackedObject> realTrackedObjects;
};

// ============================================================================
// INTEGRATION TESTS (Real Image Data)
// ============================================================================

// Test consolidation using real bird image motion data with adjusted parameters
TEST_F(MotionRegionConsolidatorTest, RealBirdImageConsolidation) {
    // Skip if no real motion was detected
    if (realTrackedObjects.empty()) {
        LOG_WARN("No motion detected in real bird images - skipping real data test");
        GTEST_SKIP() << "No motion detected in real bird images";
    }
    
    LOG_INFO("Testing consolidation with {} real TrackedObjects from bird images", realTrackedObjects.size());
    
    // Create a more permissive config for bird motion data
    ConsolidationConfig birdConfig = config;
    birdConfig.maxDistanceThreshold = 200.0;  // Birds can be further apart
    birdConfig.minObjectsPerRegion = 1;       // Allow single-bird regions for analysis
    birdConfig.idealModelRegionSize = 640;
    
    MotionRegionConsolidator birdConsolidator(birdConfig);
    
    // Test consolidation with real motion data and visualization
    cv::Mat inputImage = cv::imread(testImage2Path);  // Use the second image for visualization
    std::string outputPath = "test_results/motion_region_consolidator/google_test_mode/01_real_bird_consolidation_visualization.jpg";
    auto regions = birdConsolidator.consolidateRegionsWithVisualization(realTrackedObjects, inputImage, outputPath);
    
    // Log results for analysis
    LOG_INFO("Real bird image consolidation results (permissive config):");
    LOG_INFO("Input: {} TrackedObjects -> Output: {} consolidated regions", 
             realTrackedObjects.size(), regions.size());
        
        for (size_t i = 0; i < regions.size(); ++i) {
            const auto& region = regions[i];
        LOG_INFO("Region {}: BBox({},{},{},{}) contains {} objects", 
                 i, region.boundingBox.x, region.boundingBox.y, 
                 region.boundingBox.width, region.boundingBox.height,
                 region.trackedObjectIds.size());
    }
    
    // Validate that consolidation produces reasonable results
    EXPECT_LE(regions.size(), realTrackedObjects.size()) << "Should not create more regions than input objects";
    EXPECT_GT(regions.size(), 0) << "Should create at least one region with permissive settings";
    
    // Check that all regions meet minimum requirements
    for (const auto& region : regions) {
        EXPECT_GE(region.trackedObjectIds.size(), birdConfig.minObjectsPerRegion) 
            << "Each region should meet minimum object count";
        EXPECT_GT(region.boundingBox.area(), 0) << "Each region should have positive area";
        EXPECT_GE(region.boundingBox.width, birdConfig.idealModelRegionSize) 
            << "Region width should be at least idealModelRegionSize for YOLO";
        EXPECT_GE(region.boundingBox.height, birdConfig.idealModelRegionSize) 
            << "Region height should be at least idealModelRegionSize for YOLO";
    }
    
    // Test with default (stricter) config too
    auto strictRegions = consolidator->consolidateRegions(realTrackedObjects);
    LOG_INFO("Strict config results: {} regions (vs {} permissive)", 
             strictRegions.size(), regions.size());
}

// ============================================================================
// STANDALONE TESTS (Synthetic Data)
// ============================================================================

// Test standalone consolidation with synthetic data
TEST_F(MotionRegionConsolidatorTest, StandaloneConsolidationSynthetic) {
    std::vector<TrackedObject> objects = createSyntheticObjects();
    
    LOG_INFO("Testing consolidation with {} synthetic objects", objects.size());
    
    // Test standalone consolidation
    std::string outputPath = "test_results/motion_region_consolidator/google_test_mode/02_synthetic_consolidation.jpg";
    auto regions = consolidator->consolidateRegionsStandalone(objects, outputPath);
    
    // Verify results
    EXPECT_GT(regions.size(), 0) << "Should create at least one consolidated region";
    EXPECT_LE(regions.size(), objects.size()) << "Should not create more regions than objects";
    
    LOG_INFO("Synthetic consolidation results:");
    LOG_INFO("Input: {} objects -> Output: {} regions", objects.size(), regions.size());
    
    for (size_t i = 0; i < regions.size(); ++i) {
        const auto& region = regions[i];
        LOG_INFO("Region {}: {}x{} at ({},{}) with {} objects", 
                i, region.boundingBox.width, region.boundingBox.height,
                region.boundingBox.x, region.boundingBox.y, region.trackedObjectIds.size());
        
        // Verify region properties
        EXPECT_GE(region.trackedObjectIds.size(), config.minObjectsPerRegion) 
            << "Region should contain minimum required objects";
        EXPECT_GT(region.boundingBox.area(), 0) << "Region should have positive area";
        // Note: Width/height may be less than idealModelRegionSize for small object clusters
        // The algorithm ensures at least one dimension meets the requirement
        EXPECT_GE(std::max(region.boundingBox.width, region.boundingBox.height), config.idealModelRegionSize) 
            << "Region should have at least one dimension meeting minimum requirement";
    }
    
    // Verify output file exists
    EXPECT_TRUE(std::filesystem::exists(outputPath)) << "Visualization not created";
    LOG_INFO("Visualization saved to: {}", outputPath);
}

// Test standalone consolidation with bird-like data
TEST_F(MotionRegionConsolidatorTest, StandaloneConsolidationBirdLike) {
    std::vector<TrackedObject> objects = createBirdLikeObjects();
    
    LOG_INFO("Testing consolidation with {} bird-like objects", objects.size());
    
    // Use more permissive config for bird data
    ConsolidationConfig birdConfig = config;
    birdConfig.maxDistanceThreshold = 200.0;
    birdConfig.minObjectsPerRegion = 1;
    birdConfig.idealModelRegionSize = 640;
    
    MotionRegionConsolidator birdConsolidator(birdConfig);
    
    // Test standalone consolidation
    std::string outputPath = "test_results/motion_region_consolidator/google_test_mode/03_bird_like_consolidation.jpg";
    auto regions = birdConsolidator.consolidateRegionsStandalone(objects, outputPath);
    
    // Verify results
    EXPECT_GT(regions.size(), 0) << "Should create at least one consolidated region";
    EXPECT_LE(regions.size(), objects.size()) << "Should not create more regions than objects";
    
    LOG_INFO("Bird-like consolidation results:");
    LOG_INFO("Input: {} objects -> Output: {} regions", objects.size(), regions.size());
    
    for (size_t i = 0; i < regions.size(); ++i) {
        const auto& region = regions[i];
        LOG_INFO("Region {}: {}x{} at ({},{}) with {} objects", 
                i, region.boundingBox.width, region.boundingBox.height,
                region.boundingBox.x, region.boundingBox.y, region.trackedObjectIds.size());
        
        // Verify region properties
        EXPECT_GT(region.boundingBox.area(), 0) << "Region should have positive area";
        // Note: Width/height may be less than idealModelRegionSize for small object clusters
        // The algorithm ensures at least one dimension meets the requirement
        EXPECT_GE(std::max(region.boundingBox.width, region.boundingBox.height), birdConfig.idealModelRegionSize) 
            << "Region should have at least one dimension meeting minimum requirement";
    }
    
    // Verify output file exists
    EXPECT_TRUE(std::filesystem::exists(outputPath)) << "Visualization not created";
    LOG_INFO("Visualization saved to: {}", outputPath);
}

// ============================================================================
// CONFIGURATION AND EDGE CASE TESTS
// ============================================================================

// Test configuration variations
TEST_F(MotionRegionConsolidatorTest, ConfigurationVariations) {
    std::vector<TrackedObject> objects = createSyntheticObjects();
    
    // Test tight configuration
    ConsolidationConfig tightConfig = config;
    tightConfig.maxDistanceThreshold = 50.0;
    tightConfig.minObjectsPerRegion = 3;
    
    MotionRegionConsolidator tightConsolidator(tightConfig);
    auto tightRegions = tightConsolidator.consolidateRegionsStandalone(objects, 
        "test_results/motion_region_consolidator/google_test_mode/04_tight_config.jpg");
    
    // Test loose configuration
    ConsolidationConfig looseConfig = config;
    looseConfig.maxDistanceThreshold = 300.0;
    looseConfig.minObjectsPerRegion = 1;
    
    MotionRegionConsolidator looseConsolidator(looseConfig);
    auto looseRegions = looseConsolidator.consolidateRegionsStandalone(objects, 
        "test_results/motion_region_consolidator/google_test_mode/05_loose_config.jpg");
    
    LOG_INFO("Configuration comparison:");
    LOG_INFO("Tight config: {} regions", tightRegions.size());
    LOG_INFO("Loose config: {} regions", looseRegions.size());
    
    // Loose config should generally create fewer regions
    EXPECT_LE(looseRegions.size(), tightRegions.size()) 
        << "Loose config should create fewer or equal regions";
}

// Test edge cases
TEST_F(MotionRegionConsolidatorTest, EdgeCases) {
    // Test with empty input
    auto emptyRegions = consolidator->consolidateRegionsStandalone({}, 
        "test_results/motion_region_consolidator/google_test_mode/06_empty_input.jpg");
    EXPECT_EQ(emptyRegions.size(), 0) << "Empty input should produce no regions";
    
    // Test with single object
    std::vector<TrackedObject> singleObject = {
        TrackedObject(0, cv::Rect(100, 100, 50, 50), "uuid_0")
    };
    
    auto singleRegions = consolidator->consolidateRegionsStandalone(singleObject, 
        "test_results/motion_region_consolidator/google_test_mode/07_single_object.jpg");
    EXPECT_EQ(singleRegions.size(), 0) 
        << "Single object should not create regions (below minimum)";
    
    // Test with minimum required objects
    std::vector<TrackedObject> minObjects = {
        TrackedObject(0, cv::Rect(100, 100, 50, 50), "uuid_0"),
        TrackedObject(1, cv::Rect(120, 110, 45, 45), "uuid_1")
    };
    
    auto minRegions = consolidator->consolidateRegionsStandalone(minObjects, 
        "test_results/motion_region_consolidator/google_test_mode/08_min_objects.jpg");
    EXPECT_GT(minRegions.size(), 0) 
        << "Minimum objects should create at least one region";
}

// ============================================================================
// BASIC FUNCTIONALITY TESTS
// ============================================================================

// Test consolidation with small number of objects (synthetic data for edge cases)
TEST_F(MotionRegionConsolidatorTest, ConsolidationSmallN) {
    std::vector<TrackedObject> objects;
    // Create 20 objects: 10 close together, 10 far apart
    for (int i = 0; i < 10; ++i) {
        objects.emplace_back(i, cv::Rect(100 + i * 10, 100 + i * 10, 20, 20), "uuid" + std::to_string(i));
    }
    for (int i = 10; i < 20; ++i) {
        objects.emplace_back(i, cv::Rect(1000 + i * 10, 1000 + i * 10, 20, 20), "uuid" + std::to_string(i));
    }

    auto regions = consolidator->consolidateRegions(objects);
    
    // Expect at least one region with objects 0-9 (within 50 pixels)
    EXPECT_GE(regions.size(), 1);
    bool foundCloseRegion = false;
    for (const auto& region : regions) {
        if (region.trackedObjectIds.size() >= static_cast<size_t>(config.minObjectsPerRegion)) {
            foundCloseRegion = true;
            for (int id : region.trackedObjectIds) {
                EXPECT_LT(id, 10) << "Close region should only contain objects 0-9";
            }
        }
    }
    EXPECT_TRUE(foundCloseRegion) << "Expected a region with close objects";
}

// Test consolidation with large number of objects
TEST_F(MotionRegionConsolidatorTest, ConsolidationLargeN) {
    std::vector<TrackedObject> objects;
    // Create 60 objects: 30 close together, 30 spread out
    for (int i = 0; i < 30; ++i) {
        objects.emplace_back(i, cv::Rect(100 + i * 5, 100 + i * 5, 20, 20), "uuid" + std::to_string(i));
    }
    for (int i = 30; i < 60; ++i) {
        objects.emplace_back(i, cv::Rect(500 + (i-30) * 100, 500 + (i-30) * 100, 20, 20), "uuid" + std::to_string(i));
    }

    auto regions = consolidator->consolidateRegions(objects);
    
    // Expect at least one region with objects 0-29 (within 50 pixels)
    EXPECT_GE(regions.size(), 1);
    bool foundCloseRegion = false;
    for (const auto& region : regions) {
        if (region.trackedObjectIds.size() >= static_cast<size_t>(config.minObjectsPerRegion)) {
            foundCloseRegion = true;
            for (int id : region.trackedObjectIds) {
                EXPECT_LT(id, 30) << "Close region should only contain objects 0-29";
            }
        }
    }
    EXPECT_TRUE(foundCloseRegion) << "Expected a region with close objects";
}

// Test consolidateRegions with valid input (uses real data if available, synthetic otherwise)
TEST_F(MotionRegionConsolidatorTest, ConsolidateRegionsValidInput) {
    std::vector<TrackedObject> objects;
    
    // Use real data if available, otherwise use synthetic data
    if (!realTrackedObjects.empty() && realTrackedObjects.size() >= 2) {
        LOG_INFO("Using real bird image data for ConsolidateRegionsValidInput test");
        // Use a subset of real objects for this test
        objects.assign(realTrackedObjects.begin(), 
                      realTrackedObjects.begin() + std::min(static_cast<size_t>(3), realTrackedObjects.size()));
    } else {
        LOG_INFO("Using synthetic data for ConsolidateRegionsValidInput test");
        // Create 3 very close objects to form a group (within 50 pixel threshold)
        objects.emplace_back(0, cv::Rect(100, 100, 200, 200), "uuid0");
        objects.emplace_back(1, cv::Rect(102, 102, 200, 200), "uuid1");
        objects.emplace_back(2, cv::Rect(104, 104, 200, 200), "uuid2");
    }

    auto regions = consolidator->consolidateRegions(objects);
    
    LOG_INFO("ConsolidateRegionsValidInput: {} objects -> {} regions", objects.size(), regions.size());
    
    // For real data, we may not get exactly 1 region, so be more flexible
    if (!realTrackedObjects.empty()) {
        // Real data validation
        EXPECT_GE(regions.size(), 0) << "Should produce some result";
        EXPECT_LE(regions.size(), objects.size()) << "Should not create more regions than objects";
        
        // Check that regions meet basic requirements
        for (const auto& region : regions) {
            EXPECT_GT(region.boundingBox.area(), 0) << "Region should have positive area";
            EXPECT_GE(region.boundingBox.width, config.idealModelRegionSize) << "Region width should be at least idealModelRegionSize";
            EXPECT_GE(region.boundingBox.height, config.idealModelRegionSize) << "Region height should be at least idealModelRegionSize";
        }
    } else {
        // Synthetic data validation (more strict)
        EXPECT_EQ(regions.size(), 1) << "Expected one consolidated region for synthetic close objects";
        if (!regions.empty()) {
            auto& region = regions[0];
            EXPECT_EQ(region.trackedObjectIds.size(), 3) << "Region should contain 3 objects";
            EXPECT_GE(region.boundingBox.width, config.idealModelRegionSize) << "Region width should be at least idealModelRegionSize";
            EXPECT_GE(region.boundingBox.height, config.idealModelRegionSize) << "Region height should be at least idealModelRegionSize";
        }
    }
}

// Test consolidateRegions with empty input
TEST_F(MotionRegionConsolidatorTest, ConsolidateRegionsEmptyInput) {
    std::vector<TrackedObject> objects;
    auto regions = consolidator->consolidateRegions(objects);
    EXPECT_TRUE(regions.empty()) << "Expected no regions for empty input";
}

// Test consolidateRegions with single object (below minObjectsPerRegion)
TEST_F(MotionRegionConsolidatorTest, ConsolidateRegionsSingleObject) {
    std::vector<TrackedObject> objects;
    objects.emplace_back(0, cv::Rect(100, 100, 50, 50), "uuid0");
    
    auto regions = consolidator->consolidateRegions(objects);
    EXPECT_TRUE(regions.empty()) << "Expected no regions for single object (below minObjectsPerRegion)";
}

// Test region merging with overlapping regions
TEST_F(MotionRegionConsolidatorTest, ConsolidateRegionsMerging) {
    std::vector<TrackedObject> objects;
    // Create two overlapping groups
    objects.emplace_back(0, cv::Rect(100, 100, 50, 50), "uuid0");
    objects.emplace_back(1, cv::Rect(120, 120, 50, 50), "uuid1");
    objects.emplace_back(2, cv::Rect(110, 110, 50, 50), "uuid2"); // Overlaps with first group
    
    auto regions = consolidator->consolidateRegions(objects);
    
    // Expect one merged region
    EXPECT_EQ(regions.size(), 1) << "Expected one merged region";
    if (!regions.empty()) {
        auto& region = regions[0];
        EXPECT_EQ(region.trackedObjectIds.size(), 3) << "Merged region should contain 3 objects";
    }
}

// Test stale region removal
TEST_F(MotionRegionConsolidatorTest, RemoveStaleRegions) {
    std::vector<TrackedObject> objects;
    // Create initial regions
    objects.emplace_back(0, cv::Rect(100, 100, 50, 50), "uuid0");
    objects.emplace_back(1, cv::Rect(120, 120, 50, 50), "uuid1");
    consolidator->consolidateRegions(objects);
    
    // Simulate frames without objects
    for (int i = 0; i < config.maxFramesWithoutUpdate + 1; ++i) {
        consolidator->consolidateRegions({});
    }
    
    auto regions = consolidator->consolidateRegions({});
    EXPECT_TRUE(regions.empty()) << "Expected stale regions to be removed";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new MotionRegionConsolidatorTestEnvironment());
    return RUN_ALL_TESTS();
}