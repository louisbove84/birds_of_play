#ifndef MOTION_REGION_CONSOLIDATOR_HPP
#define MOTION_REGION_CONSOLIDATOR_HPP

#include <vector>
#include <opencv2/core.hpp>
#include "motion_tracker.hpp"

/**
 * @brief Consolidated motion region containing multiple tracked objects
 */
struct ConsolidatedRegion {
    cv::Rect boundingBox;                    // Combined bounding box
    std::vector<int> trackedObjectIds;       // IDs of objects in this region
    cv::Point2f averageVelocity;            // Average motion vector
    double confidence;                       // Confidence score
    int framesSinceLastUpdate;               // Tracking stability
    
    ConsolidatedRegion() : confidence(0.0), framesSinceLastUpdate(0) {}
    
    ConsolidatedRegion(const cv::Rect& bbox, const std::vector<int>& ids, 
                      const cv::Point2f& velocity, double conf = 1.0)
        : boundingBox(bbox), trackedObjectIds(ids), averageVelocity(velocity), 
          confidence(conf), framesSinceLastUpdate(0) {}
};

/**
 * @brief Configuration for motion region consolidation
 */
struct ConsolidationConfig {
    // Spatial proximity thresholds
    double maxDistanceThreshold = 100.0;     // Max distance between objects to group
    double overlapThreshold = 0.3;           // Min overlap ratio to merge regions
    
    // Motion similarity thresholds
    double velocityAngleThreshold = 30.0;    // Max angle difference (degrees)
    double velocityMagnitudeThreshold = 0.5; // Max velocity magnitude ratio difference
    
    // Region management
    int minObjectsPerRegion = 2;             // Min objects to form a region
    int maxFramesWithoutUpdate = 10;         // Max frames before removing region
    double minRegionArea = 400.0;            // Min area for consolidated region
    double maxRegionArea = 50000.0;          // Max area for consolidated region
    
    // Expansion parameters
    double regionExpansionFactor = 1.2;      // Factor to expand bounding box
    int minRegionWidth = 64;                 // Min width for YOLO processing
    int minRegionHeight = 64;                // Min height for YOLO processing
};

/**
 * @brief Consolidates tracked objects with similar motion into larger regions
 * 
 * This class analyzes tracked objects from MotionTracker and groups objects
 * that are spatially close and exhibit similar motion patterns into consolidated
 * regions suitable for object detection with YOLO11.
 */
class MotionRegionConsolidator {
public:
    explicit MotionRegionConsolidator(const ConsolidationConfig& config = ConsolidationConfig());
    ~MotionRegionConsolidator() = default;

    // Main processing method
    std::vector<ConsolidatedRegion> consolidateRegions(const std::vector<TrackedObject>& trackedObjects);
    
    // Configuration management
    void updateConfig(const ConsolidationConfig& config);
    const ConsolidationConfig& getConfig() const { return config_; }
    
    // Region management
    void clearRegions() { consolidatedRegions_.clear(); }
    const std::vector<ConsolidatedRegion>& getCurrentRegions() const { return consolidatedRegions_; }
    
    // Utility methods
    static cv::Rect expandBoundingBox(const cv::Rect& bbox, double expansionFactor, 
                                     const cv::Size& frameSize);
    static cv::Point2f calculateVelocity(const TrackedObject& obj);
    
private:
    // Core consolidation algorithms
    std::vector<std::vector<int>> groupObjectsByProximity(const std::vector<TrackedObject>& objects);
    std::vector<std::vector<int>> groupObjectsByMotion(const std::vector<TrackedObject>& objects);
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
    double calculateMotionSimilarity(const TrackedObject& obj1, const TrackedObject& obj2) const;
    bool areRegionsOverlapping(const ConsolidatedRegion& region1, 
                              const ConsolidatedRegion& region2) const;
    
    // Utility methods
    cv::Rect calculateBoundingBox(const std::vector<TrackedObject>& objects, 
                                 const std::vector<int>& indices) const;
    cv::Point2f calculateAverageVelocity(const std::vector<TrackedObject>& objects, 
                                        const std::vector<int>& indices) const;
    double calculateRegionConfidence(const std::vector<TrackedObject>& objects, 
                                    const std::vector<int>& indices) const;
    
private:
    ConsolidationConfig config_;
    std::vector<ConsolidatedRegion> consolidatedRegions_;
    int frameCounter_;
};

#endif // MOTION_REGION_CONSOLIDATOR_HPP
