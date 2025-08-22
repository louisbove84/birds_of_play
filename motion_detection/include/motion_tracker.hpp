#ifndef MOTION_TRACKER_HPP
#define MOTION_TRACKER_HPP

// System includes
#include <string>                       // std::string
#include <vector>                       // std::vector
#include <deque>                        // std::deque
#include <chrono>                       // std::chrono::system_clock

// Third-party includes
#include <opencv2/core.hpp>             // cv::Point, cv::Rect, cv::Scalar, cv::Mat
#include <opencv2/imgproc.hpp>          // cv::resize, cv::threshold, cv::findContours
#include <opencv2/highgui.hpp>          // cv::imshow, cv::waitKey, cv::destroyAllWindows
#include <opencv2/videoio.hpp>          // cv::VideoCapture, cv::VideoWriter
#include <yaml-cpp/yaml.h>              // YAML::Node, YAML::LoadFile

// Local includes
#include "object_classifier.hpp"        // ObjectClassifier class, ClassificationResult struct
#include "motion_visualization.hpp"     // Visualization utilities
#include "motion_processor.hpp"         // Frame processing pipeline
#include "object_tracker.hpp"           // Object tracking and trajectories

struct TrackedObject {
    int id;
    cv::Rect currentBounds;
    std::deque<cv::Point> trajectory; // Using deque for efficient pop_front
    cv::Point smoothedCenter;
    double confidence;
    int framesWithoutDetection;
    std::chrono::system_clock::time_point firstSeen;
    std::string uuid;
    cv::Mat initialFrame;
    
    // Classification results
    std::string classLabel;
    float classConfidence;
    int classId;

    cv::Point getCenter() const {
        return cv::Point(currentBounds.x + currentBounds.width / 2,
                         currentBounds.y + currentBounds.height / 2);
    }

    TrackedObject(int obj_id, const cv::Rect& bounds, std::string new_uuid) : 
        id(obj_id), currentBounds(bounds), confidence(1.0), 
        framesWithoutDetection(0), firstSeen(std::chrono::system_clock::now()), uuid(new_uuid),
        classLabel("unknown"), classConfidence(0.0f), classId(-1) {
        smoothedCenter = getCenter();
        trajectory.push_back(smoothedCenter);
    }
};

struct MotionResult {
    bool hasMotion = false;
    std::vector<TrackedObject> trackedObjects;
    
    // Intermediate processing results for visualization
    cv::Mat processedFrame;
    cv::Mat frameDiff;
    cv::Mat thresh;
    cv::Mat processed;
};

/**
 * @brief Main MotionTracker class that orchestrates frame processing and object tracking
 */
class MotionTracker {
public:
    // Constructor and destructor
    explicit MotionTracker(const std::string& configPath);
    ~MotionTracker();

    // Initialization and control
    bool initialize(const std::string& videoSource);
    bool initialize(int deviceIndex = 0);
    void stop();
    bool isRunning() const { return running; }

    // Main processing methods
    void processFrame(); // Process frame from camera
    MotionResult processFrame(const cv::Mat& frame); // Process provided frame
    
    // Access to individual processing steps (public for testing)
    cv::Mat preprocessFrame(const cv::Mat& frame);
    cv::Mat detectMotion(const cv::Mat& processedFrame, cv::Mat& frameDiff, cv::Mat& thresh);
    cv::Mat applyMorphologicalOps(const cv::Mat& thresh);
    std::vector<cv::Rect> extractContours(const cv::Mat& processed);
    void logTrackingResults();
    void setPrevFrame(const cv::Mat& frame);

    // Configuration getters
    bool isSplitScreenEnabled() const;
    const std::string& getSplitScreenWindowName() const;
    MotionVisualization& getVisualization();
    
    // Access to tracked objects
    const std::vector<TrackedObject>& getTrackedObjects() const;
    const TrackedObject* findTrackedObjectById(int id) const;
    void setTrackedObjects(const std::vector<TrackedObject>& objects);
    void clearLostObjectIds();
    const std::vector<int>& getLostObjectIds() const;
    size_t getMinTrajectoryLength() const;
    
    // Camera access
    cv::VideoCapture& getCap() { return cap; }

private:
    // Configuration loading
    void loadConfig(const std::string& configPath);

    // Core components
    MotionProcessor processor;      // Handles frame processing pipeline
    ObjectTracker tracker;         // Handles object tracking and trajectories  
    MotionVisualization visualization; // Handles visualization
    
    // Camera and state
    cv::VideoCapture cap;
    bool running;
    
    // Visualization configuration
    bool splitScreen;
    bool drawContours;
    bool dataCollection;
    bool saveOnMotion;
    std::string splitScreenWindowName;
};

#endif // MOTION_TRACKER_HPP
