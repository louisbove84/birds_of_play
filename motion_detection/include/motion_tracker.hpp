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
#include <opencv2/bgsegm.hpp>           // cv::createBackgroundSubtractorMOG2
#include <opencv2/video.hpp>            // cv::BackgroundSubtractor
#include <yaml-cpp/yaml.h>              // YAML::Node, YAML::LoadFile

// Local includes
#include "object_classifier.hpp"        // ObjectClassifier class, ClassificationResult struct

// Maximum number of points to store in trajectory (moved to config.yaml)
// const size_t MAX_TRAJECTORY_POINTS = 30;

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
    bool hasMotion;
    std::vector<TrackedObject> trackedObjects;  // List of tracked objects with their paths
};

class MotionTracker {
public:
    explicit MotionTracker(const std::string& configPath);
    ~MotionTracker();

    bool initialize(const std::string& videoSource);
    bool initialize(int deviceIndex);
    void processFrame();
    void stop();
    MotionResult processFrame(const cv::Mat& frame);
    
    // New public methods
    bool readFrame(cv::Mat& frame) {
        if (!isRunning || !cap.isOpened()) return false;
        return cap.read(frame);
    }
    static int getEscKey() { return ESC_KEY; }
    
    // Split-screen visualization method
    cv::Mat createSplitScreenVisualization(const cv::Mat& originalFrame, const cv::Mat& processedFrame, 
                                          const cv::Mat& frameDiff, const cv::Mat& thresholded, 
                                          const cv::Mat& finalProcessed);
    
    // Get split-screen visualization for main display
    cv::Mat getSplitScreenVisualization(const cv::Mat& originalFrame);
    
    // Check if split-screen is enabled
    bool isSplitScreenEnabled() const { return splitScreen; }
    
    // Draw motion overlays on a single frame
    cv::Mat drawMotionOverlays(const cv::Mat& frame);
    
    // Initialize background subtractor
    void initializeBackgroundSubtractor();

    size_t getMinTrajectoryLength() const { return minTrajectoryLength; }
    const std::vector<TrackedObject>& getTrackedObjects() const { return trackedObjects; }
    void setTrackedObjects(const std::vector<TrackedObject>& objs) { trackedObjects = objs; }
    
    // Get list of object IDs that were lost in the last frame
    std::vector<int> getLostObjectIds() const { return lostObjectIds; }
    void clearLostObjectIds() { lostObjectIds.clear(); }

    // Getters for configuration values
    const TrackedObject* findTrackedObjectById(int id) const;

    // Getters for internal state
    cv::VideoCapture& getCap() { return cap; }

    // Frame processing methods (made public for testing)
    cv::Mat preprocessFrame(const cv::Mat& frame);
    cv::Mat detectMotion(const cv::Mat& processedFrame, cv::Mat& frameDiff, cv::Mat& thresh);
    cv::Mat applyMorphologicalOps(const cv::Mat& thresh);
    std::vector<cv::Rect> extractContours(const cv::Mat& processed);
    void logTrackingResults();
    void setPrevFrame(const cv::Mat& frame);

private:
    void loadConfig(const std::string& configPath);
    TrackedObject* findNearestObject(const cv::Rect& newBounds);
    void updateTrajectories(std::vector<cv::Rect>& newBounds, const cv::Mat& currentFrame);

    // Spatial merging and motion clustering methods
    std::vector<cv::Rect> mergeSpatialOverlaps(const std::vector<cv::Rect>& bounds);
    std::vector<cv::Rect> clusterByMotion(const std::vector<cv::Rect>& bounds);
    double calculateOverlapRatio(const cv::Rect& rect1, const cv::Rect& rect2);
    double calculateDistance(const cv::Rect& rect1, const cv::Rect& rect2);
    cv::Point calculateMotionVector(const cv::Rect& current, const cv::Rect& previous);
    double calculateCosineSimilarity(const cv::Point& vec1, const cv::Point& vec2);
    cv::Rect findClosestPreviousRect(const cv::Rect& current, const std::vector<cv::Rect>& previous);
    
    // Object classification method
    ClassificationResult classifyDetectedObject(const cv::Mat& frame, const cv::Rect& bounds);

    cv::VideoCapture cap;
    cv::Mat prevFrame;
    cv::Mat currentFrame;
    bool isRunning;
    bool isFirstFrame;
    std::vector<TrackedObject> trackedObjects;
    int nextObjectId;
    size_t maxTrajectoryPoints;  // Moved from const to configurable
    size_t minTrajectoryLength;  // Minimum trajectory points required to record

    // Motion history for clustering
    std::deque<std::vector<cv::Rect>> previousBounds;

    // Configurable parameters (all loaded from config.yaml)
    int minContourArea;
    double maxTrackingDistance;
    int maxThreshold;
    static const int ESC_KEY = 27;  // Keep this as const since it's a key code

    // Smoothing parameters
    double smoothingFactor;
    double minTrackingConfidence;
    
    // ===============================
    // INPUT COLOR PROCESSING
    // ===============================
    std::string processingMode;  // "grayscale", "hsv", "rgb", "ycrcb"
    
    // ===============================
    // IMAGE PREPROCESSING
    // ===============================
    bool contrastEnhancement;
    std::string blurType;  // "none", "gaussian", "median", "bilateral"
    
    // ===============================
    // MOTION DETECTION METHODS
    // ===============================
    bool backgroundSubtraction;
    std::string backgroundSubtractionMethod;  // "none", "MOG2", "PBAS"
    std::string opticalFlowMode;  // "none", "farneback", "lucas-kanade"
    double motionHistoryDuration;  // Seconds (0 = disabled)
    
    // ===============================
    // THRESHOLDING (Otsu only)
    // ===============================
    
    // ===============================
    // MORPHOLOGICAL OPERATIONS
    // ===============================
    bool morphology;
    int morphKernelSize;
    bool morphClose;
    bool morphOpen;
    bool dilation;
    bool erosion;
    
    // ===============================
    // CONTOUR PROCESSING
    // ===============================
    bool convexHull;
    bool contourApproximation;
    bool contourFiltering;
    double maxContourAspectRatio;
    double minContourSolidity;
    double contourEpsilonFactor;
    
    // ===============================
    // OBJECT TRACKING
    // ===============================
    bool splitScreen;
    bool drawContours;
    bool dataCollection;
    bool saveOnMotion;
    std::string splitScreenWindowName;
    
    // Spatial Merging and Motion Clustering
    bool spatialMerging;
    double spatialMergeDistance;
    double spatialMergeOverlapThreshold;
    bool motionClustering;
    double motionSimilarityThreshold;
    int motionHistoryFrames;
    
    // ===============================
    // ADVANCED PARAMETERS
    // ===============================
    
    // Contrast Enhancement (CLAHE)
    double claheClipLimit;
    int claheTileSize;
    
    // Blur Parameters
    int gaussianBlurSize;
    int medianBlurSize;
    int bilateralD;
    double bilateralSigmaColor;
    double bilateralSigmaSpace;
    
    // Background Subtraction (MOG2)
    int backgroundHistory;
    double backgroundThreshold;
    bool backgroundDetectShadows;
    
    // Background Subtraction (PBAS)
    int pbasHistory;
    double pbasThreshold;
    double pbasLearningRate;
    bool pbasDetectShadows;
    
    // Background subtractor (can be MOG2 or PBAS)
    cv::Ptr<cv::BackgroundSubtractor> bgSubtractor;
    
    // Edge Detection (Canny)
    int cannyLowThreshold;
    int cannyHighThreshold;
    

    
    // HSV Color Filtering
    cv::Scalar hsvLower;
    cv::Scalar hsvUpper;
    
    // Motion History (for visualization)
    cv::Mat motionHistory;
    double motionHistoryFps;
    
    // Object Classifier
    ObjectClassifier classifier;
    bool enableClassification;
    std::string modelPath;
    std::string labelsPath;
    
    // Helper method for position smoothing
    cv::Point smoothPosition(const cv::Point& newPos, const cv::Point& smoothedPos);



    std::vector<int> lostObjectIds;
    std::string generateUUID();
};

#endif // MOTION_TRACKER_HPP 