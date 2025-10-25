#ifndef MOTION_REGION_CONSOLIDATOR_HPP
#define MOTION_REGION_CONSOLIDATOR_HPP

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

#include "tracked_object.hpp"  // For TrackedObject

/**
 * @brief Consolidated motion region containing multiple tracked objects
 */
struct ConsolidatedRegion {
    cv::Rect boundingBox;               // Combined bounding box
    std::vector<int> trackedObjectIds;  // IDs of objects in this region
    int framesSinceLastUpdate;          // Tracking stability

    ConsolidatedRegion(const cv::Rect& bbox, const std::vector<int>& ids)
        : boundingBox(bbox), trackedObjectIds(ids), framesSinceLastUpdate(0) {}
};

/**
 * @brief Configuration for motion region consolidation using DBSCAN
 */
struct ConsolidationConfig {
    // DBSCAN parameters
    double eps = 50.0;  // Maximum distance for DBSCAN neighborhood
    int minPts = 2;     // Minimum points to form a dense region in DBSCAN

    // Distance calculation parameters
    double overlapWeight = 0.7;  // Weight for overlap component in distance calculation (0.0-1.0)
    double edgeWeight =
        0.3;  // Weight for edge proximity component in distance calculation (0.0-1.0)
    double maxEdgeDistance = 100.0;  // Maximum edge-to-edge distance to consider

    // Region management
    cv::Size frameSize = cv::Size(1920, 1080);  // Frame size for boundary checking
    int maxFramesWithoutUpdate = 10;            // Max frames before removing region

    // Expansion parameters (minimal expansion only)
    double regionExpansionFactor = 1.1;  // Small factor to expand bounding box slightly
};

/**
 * @brief Consolidates tracked objects using DBSCAN clustering with overlap-aware distance
 *
 * This class analyzes tracked objects from MotionProcessor and groups objects
 * using DBSCAN clustering algorithm with a custom distance metric that considers
 * both bounding box overlap and edge proximity. No size constraints are applied.
 */
class MotionRegionConsolidator {
   public:
    explicit MotionRegionConsolidator(const ConsolidationConfig& config = ConsolidationConfig());
    ~MotionRegionConsolidator() = default;

    // Main processing method
    std::vector<ConsolidatedRegion> consolidateRegions(
        const std::vector<TrackedObject>& trackedObjects);

    // Processing with visualization
    std::vector<ConsolidatedRegion> consolidateRegionsWithVisualization(
        const std::vector<TrackedObject>& trackedObjects, const cv::Mat& inputImage,
        const std::string& outputImagePath = "");

    // Standalone consolidation with visualization (no input image required)
    std::vector<ConsolidatedRegion> consolidateRegionsStandalone(
        const std::vector<TrackedObject>& trackedObjects, const std::string& outputImagePath = "");

    // Configuration management
    void updateConfig(const ConsolidationConfig& config);
    const ConsolidationConfig& getConfig() const { return config_; }

    // Region management
    void clearRegions() { consolidatedRegions_.clear(); }
    const std::vector<ConsolidatedRegion>& getCurrentRegions() const {
        return consolidatedRegions_;
    }

   private:
    // DBSCAN clustering algorithm
    std::vector<std::vector<int>> dbscanClustering(const std::vector<TrackedObject>& objects);
    std::vector<int> rangeQuery(const std::vector<TrackedObject>& objects, int pointIdx,
                                double eps);
    std::vector<ConsolidatedRegion> createConsolidatedRegions(
        const std::vector<TrackedObject>& objects, const std::vector<std::vector<int>>& clusters);

    // Distance calculation with overlap and edge awareness
    double calculateOverlapAwareDistance(const TrackedObject& obj1,
                                         const TrackedObject& obj2) const;
    double calculateOverlapComponent(const cv::Rect& rect1, const cv::Rect& rect2) const;
    double calculateEdgeDistance(const cv::Rect& rect1, const cv::Rect& rect2) const;

    // Region update and management
    void updateExistingRegions(const std::vector<TrackedObject>& objects);
    void removeStaleRegions();
    ConsolidatedRegion mergeRegions(const ConsolidatedRegion& region1,
                                    const ConsolidatedRegion& region2);

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
    void drawConsolidatedRegions(cv::Mat& image,
                                 const std::vector<ConsolidatedRegion>& regions) const;

    ConsolidationConfig config_;
    std::vector<ConsolidatedRegion> consolidatedRegions_;
    int frameCounter_;
};

#endif  // MOTION_REGION_CONSOLIDATOR_HPP