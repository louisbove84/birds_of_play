/**
 * @file motion_region_consolidator_example.cpp
 * @brief Example integration of MotionRegionConsolidator with MotionTracker
 * 
 * This file shows how to integrate the new MotionRegionConsolidator with your
 * existing MotionTracker to create consolidated regions for YOLO11 processing.
 */

#include "motion_tracker.hpp"
#include "motion_region_consolidator.hpp"
#include "logger.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

/**
 * @brief Example class showing integration with MotionRegionConsolidator
 */
class EnhancedMotionTracker {
public:
    EnhancedMotionTracker(const std::string& configPath) 
        : motionTracker_(configPath), consolidator_() {
        
        // Configure consolidation parameters
        ConsolidationConfig config;
        config.maxDistanceThreshold = 150.0;     // Pixels - adjust based on your frame size
        config.velocityAngleThreshold = 45.0;    // Degrees - allow more variation for birds
        config.minObjectsPerRegion = 2;          // At least 2 motion areas to consolidate
        config.regionExpansionFactor = 1.3;      // Expand regions by 30% for YOLO
        config.minRegionWidth = 96;              // Minimum size for YOLO11
        config.minRegionHeight = 96;
        
        consolidator_.updateConfig(config);
        
        LOG_INFO("EnhancedMotionTracker initialized");
    }
    
    /**
     * @brief Process frame and return consolidated regions for YOLO processing
     */
    std::vector<ConsolidatedRegion> processFrameForYOLO(const cv::Mat& frame) {
        // Step 1: Run normal motion tracking
        MotionResult motionResult = motionTracker_.processFrame(frame);
        
        if (!motionResult.hasMotion || motionResult.trackedObjects.empty()) {
            return {}; // No motion detected
        }
        
        LOG_DEBUG("Processing {} tracked objects for consolidation", 
                 motionResult.trackedObjects.size());
        
        // Step 2: Consolidate tracked objects into regions
        auto consolidatedRegions = consolidator_.consolidateRegions(motionResult.trackedObjects);
        
        LOG_INFO("Created {} consolidated regions from {} tracked objects",
                consolidatedRegions.size(), motionResult.trackedObjects.size());
        
        return consolidatedRegions;
    }
    
    /**
     * @brief Extract image regions for YOLO11 processing
     */
    std::vector<cv::Mat> extractRegionsForYOLO(const cv::Mat& frame, 
                                              const std::vector<ConsolidatedRegion>& regions) {
        std::vector<cv::Mat> extractedRegions;
        
        for (const auto& region : regions) {
            // Ensure region is within frame bounds
            cv::Rect clampedRect = region.boundingBox & cv::Rect(0, 0, frame.cols, frame.rows);
            
            if (clampedRect.area() > 0) {
                cv::Mat regionImage = frame(clampedRect).clone();
                extractedRegions.push_back(regionImage);
                
                LOG_DEBUG("Extracted region: {}x{} at ({},{})", 
                         clampedRect.width, clampedRect.height, 
                         clampedRect.x, clampedRect.y);
            }
        }
        
        return extractedRegions;
    }
    
    /**
     * @brief Visualize consolidated regions on frame
     */
    cv::Mat visualizeConsolidatedRegions(const cv::Mat& frame, 
                                        const std::vector<ConsolidatedRegion>& regions) {
        cv::Mat visualization = frame.clone();
        
        for (size_t i = 0; i < regions.size(); ++i) {
            const auto& region = regions[i];
            
            // Draw bounding box in bright color
            cv::Scalar color(0, 255, 255); // Yellow
            cv::rectangle(visualization, region.boundingBox, color, 3);
            
            // Draw velocity vector
            cv::Point center(region.boundingBox.x + region.boundingBox.width / 2,
                           region.boundingBox.y + region.boundingBox.height / 2);
            cv::Point velocityEnd(center.x + static_cast<int>(region.averageVelocity.x * 10),
                                center.y + static_cast<int>(region.averageVelocity.y * 10));
            cv::arrowedLine(visualization, center, velocityEnd, color, 2);
            
            // Add region info text
            std::string info = "R" + std::to_string(i) + " (" + 
                             std::to_string(region.trackedObjectIds.size()) + " objs)";
            cv::putText(visualization, info, 
                       cv::Point(region.boundingBox.x, region.boundingBox.y - 10),
                       cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        }
        
        return visualization;
    }
    
    /**
     * @brief Complete processing pipeline example
     */
    void processVideoForBirdDetection(const std::string& videoPath) {
        cv::VideoCapture cap(videoPath);
        if (!cap.isOpened()) {
            LOG_ERROR("Could not open video: {}", videoPath);
            return;
        }
        
        cv::Mat frame;
        int frameCount = 0;
        
        while (cap.read(frame)) {
            frameCount++;
            
            // Step 1: Get consolidated regions
            auto regions = processFrameForYOLO(frame);
            
            if (!regions.empty()) {
                LOG_INFO("Frame {}: Found {} consolidated regions", frameCount, regions.size());
                
                // Step 2: Extract regions for YOLO11 processing
                auto regionImages = extractRegionsForYOLO(frame, regions);
                
                // Step 3: Here you would run YOLO11 on each region
                for (size_t i = 0; i < regionImages.size(); ++i) {
                    // TODO: Run YOLO11 inference on regionImages[i]
                    // Example: auto detections = yolo11.detect(regionImages[i]);
                    LOG_DEBUG("Region {} ready for YOLO11: {}x{}", 
                             i, regionImages[i].cols, regionImages[i].rows);
                }
                
                // Step 4: Visualize results
                cv::Mat visualization = visualizeConsolidatedRegions(frame, regions);
                cv::imshow("Consolidated Motion Regions", visualization);
            }
            
            if (cv::waitKey(1) == 27) break; // ESC to exit
        }
        
        cv::destroyAllWindows();
    }
    
    // Access to underlying components
    MotionTracker& getMotionTracker() { return motionTracker_; }
    MotionRegionConsolidator& getConsolidator() { return consolidator_; }
    
private:
    MotionTracker motionTracker_;
    MotionRegionConsolidator consolidator_;
};

// Example usage function
void exampleUsage() {
    try {
        EnhancedMotionTracker tracker("config.yaml");
        
        // Process video file
        tracker.processVideoForBirdDetection("bird_video.mp4");
        
        // Or process single frame
        cv::Mat frame = cv::imread("bird_frame.jpg");
        if (!frame.empty()) {
            auto regions = tracker.processFrameForYOLO(frame);
            auto regionImages = tracker.extractRegionsForYOLO(frame, regions);
            
            // Save extracted regions for inspection
            for (size_t i = 0; i < regionImages.size(); ++i) {
                cv::imwrite("region_" + std::to_string(i) + ".jpg", regionImages[i]);
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error in example usage: {}", e.what());
    }
}
