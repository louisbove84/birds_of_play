#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <deque>
#include <string>
#include "object_classifier.hpp"

// Forward declaration
struct TrackedObject;

/**
 * @brief Object tracking class - handles object tracking, trajectories, 
 *        spatial merging, motion clustering, and classification
 */
class ObjectTracker {
public:
    struct TrackingResult {
        std::vector<TrackedObject> trackedObjects;
        std::vector<int> lostObjectIds;
        bool hasTrackedObjects = false;
    };

    explicit ObjectTracker(const std::string& configPath);
    ~ObjectTracker() = default;

    // Main tracking pipeline
    TrackingResult trackObjects(const std::vector<cv::Rect>& detectedBounds, const cv::Mat& currentFrame);
    
    // Individual tracking steps (public for testing)
    std::vector<cv::Rect> mergeSpatialOverlaps(const std::vector<cv::Rect>& bounds);
    std::vector<cv::Rect> clusterByMotion(const std::vector<cv::Rect>& bounds);
    void updateTrajectories(std::vector<cv::Rect>& newBounds, const cv::Mat& currentFrame);
    void logTrackingResults();

    // Object management
    TrackedObject* findNearestObject(const cv::Rect& bounds);
    const TrackedObject* findTrackedObjectById(int id) const;
    const std::vector<TrackedObject>& getTrackedObjects() const { return trackedObjects; }
    const std::vector<int>& getLostObjectIds() const { return lostObjectIds; }
    void setTrackedObjects(const std::vector<TrackedObject>& objects) { trackedObjects = objects; }
    void clearLostObjectIds() { lostObjectIds.clear(); }

    // Configuration getters
    size_t getMinTrajectoryLength() const { return minTrajectoryLength; }
    double getMaxTrackingDistance() const { return maxTrackingDistance; }
    bool isSpatialMergingEnabled() const { return spatialMerging; }
    bool isMotionClusteringEnabled() const { return motionClustering; }

private:
    // Configuration loading
    void loadConfig(const std::string& configPath);
    
    // Helper functions
    cv::Point smoothPosition(const cv::Point& newPos, const cv::Point& smoothedPos);
    std::string generateUUID();
    ClassificationResult classifyDetectedObject(const cv::Mat& frame, const cv::Rect& bounds);
    
    // Spatial merging helpers
    double calculateOverlapRatio(const cv::Rect& rect1, const cv::Rect& rect2);
    double calculateDistance(const cv::Rect& rect1, const cv::Rect& rect2);
    
    // Motion clustering helpers
    cv::Point calculateMotionVector(const cv::Rect& current, const cv::Rect& previous);
    double calculateCosineSimilarity(const cv::Point& vec1, const cv::Point& vec2);
    cv::Rect findClosestPreviousRect(const cv::Rect& current, const std::vector<cv::Rect>& previous);

    // Tracking state
    std::vector<TrackedObject> trackedObjects;
    std::vector<int> lostObjectIds;
    std::deque<std::vector<cv::Rect>> previousBounds;
    int nextObjectId = 0;

    // Object classifier
    ObjectClassifier classifier;

    // ===============================
    // CONFIGURATION PARAMETERS
    // ===============================
    
    // Basic tracking parameters
    size_t maxTrajectoryPoints;
    size_t minTrajectoryLength;
    double maxTrackingDistance;
    double smoothingFactor;
    double minTrackingConfidence;
    
    // Spatial Merging
    bool spatialMerging;
    double spatialMergeDistance;
    double spatialMergeOverlapThreshold;
    
    // Motion Clustering
    bool motionClustering;
    double motionSimilarityThreshold;
    int motionHistoryFrames;
    
    // Object Classification
    bool enableClassification;
    std::string modelPath;
    std::string labelsPath;
};
