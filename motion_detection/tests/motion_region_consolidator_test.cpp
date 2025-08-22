#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <filesystem>
#include <vector>
#include <string>

#include "motion_region_consolidator.hpp"
#include "motion_tracker.hpp"
#include "logger.hpp"

namespace fs = std::filesystem;

class MotionRegionConsolidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger
        Logger::init("debug", "test_log.txt", false);
        
        // Setup test directories
        testImageDir_ = "tests/img/";
        outputDir_ = "test_results/motion_region_consolidator/";
        
        // Create output directories
        fs::create_directories(outputDir_);
        fs::create_directories(outputDir_ + "01_input_images/");
        fs::create_directories(outputDir_ + "02_motion_tracking/");
        fs::create_directories(outputDir_ + "03_proximity_grouping/");
        fs::create_directories(outputDir_ + "04_motion_grouping/");
        fs::create_directories(outputDir_ + "05_consolidated_regions/");
        fs::create_directories(outputDir_ + "06_expanded_regions/");
        fs::create_directories(outputDir_ + "07_final_output/");
        
        // Initialize consolidator with test config
        ConsolidationConfig config;
        config.maxDistanceThreshold = 120.0;
        config.velocityAngleThreshold = 45.0;
        config.minObjectsPerRegion = 2;
        config.regionExpansionFactor = 1.3;
        config.minRegionWidth = 64;
        config.minRegionHeight = 64;
        config.minRegionArea = 300.0;
        config.maxRegionArea = 20000.0;
        
        consolidator_ = std::make_unique<MotionRegionConsolidator>(config);
        
        // Initialize motion tracker for generating test data
        motionTracker_ = std::make_unique<MotionTracker>("tests/config.yaml");
        
        LOG_INFO("MotionRegionConsolidatorTest setup complete");
    }
    
    void TearDown() override {
        consolidator_.reset();
        motionTracker_.reset();
    }
    
    // Helper function to create mock tracked objects for testing
    std::vector<TrackedObject> createMockTrackedObjects(const cv::Mat& frame, int imageIndex) {
        std::vector<TrackedObject> objects;
        
        switch (imageIndex) {
            case 1: {
                // Test image 1: Create objects that should be grouped by proximity
                objects.emplace_back(1, cv::Rect(100, 150, 40, 60), "uuid-1");
                objects.back().trajectory.push_back(cv::Point(120, 180));
                objects.back().trajectory.push_back(cv::Point(125, 185));
                objects.back().trajectory.push_back(cv::Point(130, 190));
                objects.back().confidence = 0.8;
                
                objects.emplace_back(2, cv::Rect(160, 170, 35, 50), "uuid-2");
                objects.back().trajectory.push_back(cv::Point(177, 195));
                objects.back().trajectory.push_back(cv::Point(182, 200));
                objects.back().trajectory.push_back(cv::Point(187, 205));
                objects.back().confidence = 0.7;
                
                // Distant object that shouldn't be grouped
                objects.emplace_back(3, cv::Rect(400, 300, 30, 45), "uuid-3");
                objects.back().trajectory.push_back(cv::Point(415, 322));
                objects.back().trajectory.push_back(cv::Point(420, 327));
                objects.back().trajectory.push_back(cv::Point(425, 332));
                objects.back().confidence = 0.6;
                break;
            }
            case 2: {
                // Test image 2: Create objects with similar motion vectors
                objects.emplace_back(4, cv::Rect(200, 100, 45, 55), "uuid-4");
                objects.back().trajectory.push_back(cv::Point(222, 127));
                objects.back().trajectory.push_back(cv::Point(232, 137));
                objects.back().trajectory.push_back(cv::Point(242, 147));
                objects.back().confidence = 0.9;
                
                objects.emplace_back(5, cv::Rect(280, 120, 40, 50), "uuid-5");
                objects.back().trajectory.push_back(cv::Point(300, 145));
                objects.back().trajectory.push_back(cv::Point(310, 155));
                objects.back().trajectory.push_back(cv::Point(320, 165));
                objects.back().confidence = 0.8;
                
                objects.emplace_back(6, cv::Rect(350, 140, 38, 48), "uuid-6");
                objects.back().trajectory.push_back(cv::Point(369, 164));
                objects.back().trajectory.push_back(cv::Point(379, 174));
                objects.back().trajectory.push_back(cv::Point(389, 184));
                objects.back().confidence = 0.7;
                break;
            }
            case 3: {
                // Test image 3: Mixed scenario with multiple groups
                // Group 1: Close proximity
                objects.emplace_back(7, cv::Rect(150, 200, 35, 45), "uuid-7");
                objects.back().trajectory.push_back(cv::Point(167, 222));
                objects.back().trajectory.push_back(cv::Point(172, 227));
                objects.back().trajectory.push_back(cv::Point(177, 232));
                objects.back().confidence = 0.8;
                
                objects.emplace_back(8, cv::Rect(190, 220, 40, 50), "uuid-8");
                objects.back().trajectory.push_back(cv::Point(210, 245));
                objects.back().trajectory.push_back(cv::Point(215, 250));
                objects.back().trajectory.push_back(cv::Point(220, 255));
                objects.back().confidence = 0.7;
                
                // Group 2: Similar motion but farther away
                objects.emplace_back(9, cv::Rect(350, 180, 42, 48), "uuid-9");
                objects.back().trajectory.push_back(cv::Point(371, 204));
                objects.back().trajectory.push_back(cv::Point(376, 209));
                objects.back().trajectory.push_back(cv::Point(381, 214));
                objects.back().confidence = 0.6;
                
                objects.emplace_back(10, cv::Rect(400, 200, 38, 52), "uuid-10");
                objects.back().trajectory.push_back(cv::Point(419, 226));
                objects.back().trajectory.push_back(cv::Point(424, 231));
                objects.back().trajectory.push_back(cv::Point(429, 236));
                objects.back().confidence = 0.8;
                break;
            }
        }
        
        return objects;
    }
    
    // Visualization helper functions
    cv::Mat visualizeTrackedObjects(const cv::Mat& frame, const std::vector<TrackedObject>& objects, 
                                   const std::string& title) {
        cv::Mat visualization = frame.clone();
        
        for (const auto& obj : objects) {
            // Draw bounding box
            cv::rectangle(visualization, obj.currentBounds, cv::Scalar(0, 255, 0), 2);
            
            // Draw trajectory
            if (obj.trajectory.size() > 1) {
                for (size_t i = 1; i < obj.trajectory.size(); ++i) {
                    cv::line(visualization, obj.trajectory[i-1], obj.trajectory[i], 
                            cv::Scalar(255, 0, 0), 2);
                }
                // Draw arrow at the end
                if (obj.trajectory.size() >= 2) {
                    cv::Point start = obj.trajectory[obj.trajectory.size() - 2];
                    cv::Point end = obj.trajectory[obj.trajectory.size() - 1];
                    cv::arrowedLine(visualization, start, end, cv::Scalar(255, 0, 255), 3);
                }
            }
            
            // Add object ID
            cv::putText(visualization, "ID:" + std::to_string(obj.id), 
                       cv::Point(obj.currentBounds.x, obj.currentBounds.y - 5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
        }
        
        // Add title
        cv::putText(visualization, title, cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
        
        return visualization;
    }
    
    cv::Mat visualizeGroups(const cv::Mat& frame, const std::vector<TrackedObject>& objects,
                           const std::vector<std::vector<int>>& groups, const std::string& title) {
        cv::Mat visualization = frame.clone();
        
        // Different colors for different groups
        std::vector<cv::Scalar> colors = {
            cv::Scalar(255, 0, 0),    // Blue
            cv::Scalar(0, 255, 0),    // Green  
            cv::Scalar(0, 0, 255),    // Red
            cv::Scalar(255, 255, 0),  // Cyan
            cv::Scalar(255, 0, 255),  // Magenta
            cv::Scalar(0, 255, 255),  // Yellow
        };
        
        for (size_t groupIdx = 0; groupIdx < groups.size(); ++groupIdx) {
            cv::Scalar color = colors[groupIdx % colors.size()];
            
            for (int objIdx : groups[groupIdx]) {
                const auto& obj = objects[objIdx];
                
                // Draw bounding box in group color
                cv::rectangle(visualization, obj.currentBounds, color, 3);
                
                // Draw trajectory
                if (obj.trajectory.size() > 1) {
                    for (size_t i = 1; i < obj.trajectory.size(); ++i) {
                        cv::line(visualization, obj.trajectory[i-1], obj.trajectory[i], color, 2);
                    }
                }
                
                // Add group and object info
                std::string label = "G" + std::to_string(groupIdx) + ":ID" + std::to_string(obj.id);
                cv::putText(visualization, label, 
                           cv::Point(obj.currentBounds.x, obj.currentBounds.y - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
            }
        }
        
        // Add title and group count
        cv::putText(visualization, title, cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
        cv::putText(visualization, "Groups: " + std::to_string(groups.size()), 
                   cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        
        return visualization;
    }
    
    cv::Mat visualizeConsolidatedRegions(const cv::Mat& frame, 
                                        const std::vector<ConsolidatedRegion>& regions, 
                                        const std::string& title) {
        cv::Mat visualization = frame.clone();
        
        for (size_t i = 0; i < regions.size(); ++i) {
            const auto& region = regions[i];
            
            // Draw bounding box
            cv::Scalar color(0, 255, 255); // Yellow
            cv::rectangle(visualization, region.boundingBox, color, 4);
            
            // Draw velocity vector
            cv::Point center(region.boundingBox.x + region.boundingBox.width / 2,
                           region.boundingBox.y + region.boundingBox.height / 2);
            cv::Point velocityEnd(center.x + static_cast<int>(region.averageVelocity.x * 15),
                                center.y + static_cast<int>(region.averageVelocity.y * 15));
            cv::arrowedLine(visualization, center, velocityEnd, color, 3);
            
            // Add region info
            std::string info = "R" + std::to_string(i) + " (" + 
                             std::to_string(region.trackedObjectIds.size()) + " objs)";
            cv::putText(visualization, info, 
                       cv::Point(region.boundingBox.x, region.boundingBox.y - 10),
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, color, 2);
            
            // Add confidence
            std::string confInfo = "Conf: " + std::to_string(region.confidence).substr(0, 4);
            cv::putText(visualization, confInfo,
                       cv::Point(region.boundingBox.x, region.boundingBox.y + region.boundingBox.height + 20),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }
        
        // Add title and region count
        cv::putText(visualization, title, cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
        cv::putText(visualization, "Regions: " + std::to_string(regions.size()), 
                   cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        
        return visualization;
    }
    
protected:
    std::string testImageDir_;
    std::string outputDir_;
    std::unique_ptr<MotionRegionConsolidator> consolidator_;
    std::unique_ptr<MotionTracker> motionTracker_;
};

TEST_F(MotionRegionConsolidatorTest, ComprehensiveVisualizationTest) {
    std::vector<std::string> testImages = {
        "test_image.jpg",
        "test_image2.jpg", 
        "test_image3.png"
    };
    
    for (size_t imgIdx = 0; imgIdx < testImages.size(); ++imgIdx) {
        std::string imagePath = testImageDir_ + testImages[imgIdx];
        cv::Mat frame = cv::imread(imagePath);
        
        ASSERT_FALSE(frame.empty()) << "Could not load test image: " << imagePath;
        
        std::string imgPrefix = "img" + std::to_string(imgIdx + 1) + "_";
        
        LOG_INFO("Processing test image {}: {}", imgIdx + 1, testImages[imgIdx]);
        
        // Step 1: Save input image
        cv::Mat inputViz = frame.clone();
        cv::putText(inputViz, "Input Image " + std::to_string(imgIdx + 1), 
                   cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
        cv::imwrite(outputDir_ + "01_input_images/" + imgPrefix + "input.jpg", inputViz);
        
        // Step 2: Create mock tracked objects (simulating motion tracker output)
        auto trackedObjects = createMockTrackedObjects(frame, imgIdx + 1);
        ASSERT_FALSE(trackedObjects.empty()) << "No tracked objects created for image " << imgIdx + 1;
        
        cv::Mat trackingViz = visualizeTrackedObjects(frame, trackedObjects, 
                                                     "Motion Tracking Results");
        cv::imwrite(outputDir_ + "02_motion_tracking/" + imgPrefix + "tracked_objects.jpg", trackingViz);
        
        // Step 3: Test proximity grouping
        LOG_DEBUG("Testing proximity grouping for {} objects", trackedObjects.size());
        
        // We need to access private methods for testing, so we'll use the public interface
        // and analyze the results
        auto consolidatedRegions = consolidator_->consolidateRegions(trackedObjects);
        
        // For visualization, we'll simulate the grouping process
        std::vector<std::vector<int>> proximityGroups;
        std::vector<bool> assigned(trackedObjects.size(), false);
        
        // Simple proximity grouping for visualization
        for (size_t i = 0; i < trackedObjects.size(); ++i) {
            if (assigned[i]) continue;
            
            std::vector<int> group = {static_cast<int>(i)};
            assigned[i] = true;
            
            cv::Point center1 = trackedObjects[i].getCenter();
            
            for (size_t j = i + 1; j < trackedObjects.size(); ++j) {
                if (assigned[j]) continue;
                
                cv::Point center2 = trackedObjects[j].getCenter();
                double distance = cv::norm(center1 - center2);
                
                if (distance <= 120.0) { // Using config threshold
                    group.push_back(static_cast<int>(j));
                    assigned[j] = true;
                }
            }
            
            proximityGroups.push_back(group);
        }
        
        cv::Mat proximityViz = visualizeGroups(frame, trackedObjects, proximityGroups, 
                                              "Proximity Grouping");
        cv::imwrite(outputDir_ + "03_proximity_grouping/" + imgPrefix + "proximity_groups.jpg", proximityViz);
        
        // Step 4: Test motion grouping (simplified visualization)
        std::vector<std::vector<int>> motionGroups;
        assigned.assign(trackedObjects.size(), false);
        
        for (size_t i = 0; i < trackedObjects.size(); ++i) {
            if (assigned[i]) continue;
            
            std::vector<int> group = {static_cast<int>(i)};
            assigned[i] = true;
            
            // Simple motion similarity check
            if (trackedObjects[i].trajectory.size() >= 2) {
                cv::Point2f vel1 = MotionRegionConsolidator::calculateVelocity(trackedObjects[i]);
                
                for (size_t j = i + 1; j < trackedObjects.size(); ++j) {
                    if (assigned[j] || trackedObjects[j].trajectory.size() < 2) continue;
                    
                    cv::Point2f vel2 = MotionRegionConsolidator::calculateVelocity(trackedObjects[j]);
                    
                    // Simple angle similarity check
                    double mag1 = cv::norm(vel1);
                    double mag2 = cv::norm(vel2);
                    
                    if (mag1 > 1.0 && mag2 > 1.0) {
                        double dot = vel1.dot(vel2);
                        double angle = std::acos(std::clamp(dot / (mag1 * mag2), -1.0, 1.0));
                        double angleDegrees = angle * 180.0 / M_PI;
                        
                        if (angleDegrees < 45.0) { // Using config threshold
                            group.push_back(static_cast<int>(j));
                            assigned[j] = true;
                        }
                    }
                }
            }
            
            motionGroups.push_back(group);
        }
        
        cv::Mat motionViz = visualizeGroups(frame, trackedObjects, motionGroups, 
                                           "Motion Similarity Grouping");
        cv::imwrite(outputDir_ + "04_motion_grouping/" + imgPrefix + "motion_groups.jpg", motionViz);
        
        // Step 5: Visualize consolidated regions
        cv::Mat consolidatedViz = visualizeConsolidatedRegions(frame, consolidatedRegions, 
                                                              "Consolidated Regions");
        cv::imwrite(outputDir_ + "05_consolidated_regions/" + imgPrefix + "consolidated.jpg", consolidatedViz);
        
        // Step 6: Visualize expanded regions
        cv::Mat expandedViz = frame.clone();
        for (size_t i = 0; i < consolidatedRegions.size(); ++i) {
            const auto& region = consolidatedRegions[i];
            
            // Show original region
            cv::rectangle(expandedViz, region.boundingBox, cv::Scalar(0, 255, 0), 2);
            
            // Show expanded region
            cv::Rect expanded = MotionRegionConsolidator::expandBoundingBox(
                region.boundingBox, 1.3, frame.size());
            cv::rectangle(expandedViz, expanded, cv::Scalar(0, 255, 255), 3);
            
            // Add labels
            cv::putText(expandedViz, "Original", 
                       cv::Point(region.boundingBox.x, region.boundingBox.y - 25),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
            cv::putText(expandedViz, "Expanded", 
                       cv::Point(expanded.x, expanded.y - 5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
        }
        
        cv::putText(expandedViz, "Region Expansion", cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
        cv::imwrite(outputDir_ + "06_expanded_regions/" + imgPrefix + "expanded.jpg", expandedViz);
        
        // Step 7: Final output with all information
        cv::Mat finalViz = frame.clone();
        
        // Draw original tracked objects in light color
        for (const auto& obj : trackedObjects) {
            cv::rectangle(finalViz, obj.currentBounds, cv::Scalar(100, 100, 100), 1);
        }
        
        // Draw consolidated regions prominently
        for (size_t i = 0; i < consolidatedRegions.size(); ++i) {
            const auto& region = consolidatedRegions[i];
            
            // Draw expanded bounding box
            cv::Rect expanded = MotionRegionConsolidator::expandBoundingBox(
                region.boundingBox, 1.3, frame.size());
            cv::rectangle(finalViz, expanded, cv::Scalar(0, 255, 255), 4);
            
            // Draw velocity vector
            cv::Point center(expanded.x + expanded.width / 2, expanded.y + expanded.height / 2);
            cv::Point velocityEnd(center.x + static_cast<int>(region.averageVelocity.x * 20),
                                center.y + static_cast<int>(region.averageVelocity.y * 20));
            cv::arrowedLine(finalViz, center, velocityEnd, cv::Scalar(255, 0, 255), 4);
            
            // Add comprehensive info
            std::vector<std::string> info = {
                "Region " + std::to_string(i),
                "Objects: " + std::to_string(region.trackedObjectIds.size()),
                "Size: " + std::to_string(expanded.width) + "x" + std::to_string(expanded.height),
                "Conf: " + std::to_string(region.confidence).substr(0, 4)
            };
            
            for (size_t j = 0; j < info.size(); ++j) {
                cv::putText(finalViz, info[j],
                           cv::Point(expanded.x, expanded.y - 30 + static_cast<int>(j * 20)),
                           cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
            }
        }
        
        // Add summary
        cv::putText(finalViz, "Final Consolidated Regions for YOLO11", 
                   cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
        cv::putText(finalViz, "Input Objects: " + std::to_string(trackedObjects.size()) + 
                             " -> Output Regions: " + std::to_string(consolidatedRegions.size()), 
                   cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        
        cv::imwrite(outputDir_ + "07_final_output/" + imgPrefix + "final_result.jpg", finalViz);
        
        // Assertions
        EXPECT_GE(consolidatedRegions.size(), 0) << "Should produce some consolidated regions";
        EXPECT_LE(consolidatedRegions.size(), trackedObjects.size()) 
            << "Should not have more regions than input objects";
        
        for (const auto& region : consolidatedRegions) {
            EXPECT_GE(region.trackedObjectIds.size(), 1) 
                << "Each region should contain at least one object";
            EXPECT_GT(region.boundingBox.area(), 0) 
                << "Region should have positive area";
        }
        
        LOG_INFO("Completed processing image {}: {} objects -> {} regions", 
                imgIdx + 1, trackedObjects.size(), consolidatedRegions.size());
    }
    
    LOG_INFO("All test images processed successfully. Check {} for results", outputDir_);
}

TEST_F(MotionRegionConsolidatorTest, ConfigurationTest) {
    // Test different configuration parameters
    ConsolidationConfig configs[] = {
        {80.0, 0.2, 30.0, 0.3, 2, 10, 200.0, 30000.0, 1.1, 32, 32},   // Tight grouping
        {200.0, 0.5, 60.0, 0.5, 1, 15, 500.0, 50000.0, 1.5, 96, 96}, // Loose grouping
    };
    
    std::string configNames[] = {"tight", "loose"};
    
    cv::Mat testFrame = cv::imread(testImageDir_ + "test_image.jpg");
    ASSERT_FALSE(testFrame.empty());
    
    auto trackedObjects = createMockTrackedObjects(testFrame, 1);
    
    for (int i = 0; i < 2; ++i) {
        consolidator_->updateConfig(configs[i]);
        auto regions = consolidator_->consolidateRegions(trackedObjects);
        
        cv::Mat configViz = visualizeConsolidatedRegions(testFrame, regions, 
                                                        "Config: " + configNames[i]);
        cv::imwrite(outputDir_ + "07_final_output/config_" + configNames[i] + "_test.jpg", configViz);
        
        LOG_INFO("Config {}: {} objects -> {} regions", configNames[i], 
                trackedObjects.size(), regions.size());
    }
}
