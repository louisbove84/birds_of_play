#ifndef MOTION_REGION_CONSOLIDATOR_HPP
#define MOTION_REGION_CONSOLIDATOR_HPP

#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include "object_tracker.hpp" // For TrackedObject

/**
 * @brief Consolidated motion region containing multiple tracked objects
 */
struct ConsolidatedRegion {
    cv::Rect boundingBox;                    // Combined bounding box
    std::vector<int> trackedObjectIds;       // IDs of objects in this region
    int framesSinceLastUpdate;               // Tracking stability
    
    ConsolidatedRegion(const cv::Rect& bbox, const std::vector<int>& ids)
        : boundingBox(bbox), trackedObjectIds(ids), framesSinceLastUpdate(0) {}
};

/**
 * @brief Configuration for motion region consolidation
 */
struct ConsolidationConfig {
    // Spatial proximity thresholds
    double maxDistanceThreshold = 100.0;     // Max distance between objects to group
    double overlapThreshold = 0.3;           // Min overlap ratio to merge regions
    
    // Region size configuration
    cv::Size frameSize = cv::Size(1920, 1080); // Frame size for boundary checking
    int idealModelRegionSize = 640;          // Ideal region size for YOLOv11 (e.g., 640x640)
    
    // Grid-based grouping
    double gridCellSize = 100.0;             // Cell size for grid-based grouping
    
    // Region management
    int minObjectsPerRegion = 2;             // Min objects to form a region
    int maxFramesWithoutUpdate = 10;         // Max frames before removing region
    double minRegionArea = 400.0;            // Min area for consolidated region
    double maxRegionArea = 50000.0;          // Max area for consolidated region
    
    // Expansion parameters
    double regionExpansionFactor = 1.2;      // Factor to expand bounding box
};

/**
 * @brief Consolidates tracked objects by spatial proximity into YOLOv11-optimized regions
 * 
 * This class analyzes tracked objects from MotionProcessor and groups objects
 * that are spatially close into consolidated regions optimized for YOLOv11 input
 * (e.g., 640x640). Regions are created based on proximity and merged based on overlap.
 */
class MotionRegionConsolidator {
public:
    explicit MotionRegionConsolidator(const ConsolidationConfig& config = ConsolidationConfig());
    ~MotionRegionConsolidator() = default;

    // Main processing method
    std::vector<ConsolidatedRegion> consolidateRegions(const std::vector<TrackedObject>& trackedObjects);
    
    // Processing with visualization
    std::vector<ConsolidatedRegion> consolidateRegionsWithVisualization(
        const std::vector<TrackedObject>& trackedObjects,
        const cv::Mat& inputImage,
        const std::string& outputImagePath = "");
    
    // Standalone consolidation with visualization (no input image required)
    std::vector<ConsolidatedRegion> consolidateRegionsStandalone(
        const std::vector<TrackedObject>& trackedObjects,
        const std::string& outputImagePath = "");
    
    // Configuration management
    void updateConfig(const ConsolidationConfig& config);
    const ConsolidationConfig& getConfig() const { return config_; }
    
    // Region management
    void clearRegions() { consolidatedRegions_.clear(); }
    const std::vector<ConsolidatedRegion>& getCurrentRegions() const { return consolidatedRegions_; }
    
private:
    // Core consolidation algorithms
    std::vector<std::vector<int>> groupObjectsByProximity(const std::vector<TrackedObject>& objects);
    std::vector<std::vector<int>> groupByProximityPairwise(const std::vector<TrackedObject>& objects);
    std::vector<std::vector<int>> groupByProximityGrid(const std::vector<TrackedObject>& objects);
    std::vector<ConsolidatedRegion> createConsolidatedRegions(
        const std::vector<TrackedObject>& objects,
        const std::vector<std::vector<int>>& groups);
    
    // Region update and management
    void updateExistingRegions(const std::vector<TrackedObject>& objects);
    void removeStaleRegions();
    ConsolidatedRegion mergeRegions(const ConsolidatedRegion& region1, 
                                   const ConsolidatedRegion& region2);
    
    // Similarity calculations
    double calculateSpatialDistance(const TrackedObject& obj1, const TrackedObject& obj2) const;
    bool areRegionsOverlapping(const ConsolidatedRegion& region1, 
                              const ConsolidatedRegion& region2) const;
    
    // Utility methods
    cv::Rect calculateBoundingBox(const std::vector<TrackedObject>& objects, 
                                 const std::vector<int>& indices) const;
    cv::Rect expandBoundingBox(const cv::Rect& bbox, double expansionFactor, 
                              const cv::Size& frameSize);
    
    // Visualization methods
    cv::Mat createVisualization(const std::vector<TrackedObject>& trackedObjects,
                               const std::vector<ConsolidatedRegion>& regions,
                               const cv::Mat& inputImage) const;
    void drawMotionBoxes(cv::Mat& image, const std::vector<TrackedObject>& trackedObjects) const;
    void drawConsolidatedRegions(cv::Mat& image, const std::vector<ConsolidatedRegion>& regions) const;

    ConsolidationConfig config_;
    std::vector<ConsolidatedRegion> consolidatedRegions_;
    int frameCounter_;
};

#endif // MOTION_REGION_CONSOLIDATOR_HPP