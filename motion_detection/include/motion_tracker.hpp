#ifndef MOTION_TRACKER_HPP
#define MOTION_TRACKER_HPP

// System includes
#include <string>
#include <vector>
#include <deque>
#include <chrono>

// Third-party includes
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/bgsegm.hpp>
#include <opencv2/video.hpp>
#include <yaml-cpp/yaml.h>

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

    cv::Point getCenter() const {
        return cv::Point(currentBounds.x + currentBounds.width / 2,
                         currentBounds.y + currentBounds.height / 2);
    }

    TrackedObject(int obj_id, const cv::Rect& bounds, std::string new_uuid) : 
        id(obj_id), currentBounds(bounds), confidence(1.0), 
        framesWithoutDetection(0), firstSeen(std::chrono::system_clock::now()), uuid(new_uuid) {
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

private:
    void loadConfig(const std::string& configPath);
    TrackedObject* findNearestObject(const cv::Rect& newBounds);
    void updateTrajectories(std::vector<cv::Rect>& newBounds);

    cv::VideoCapture cap;
    cv::Mat prevFrame;
    cv::Mat currentFrame;
    bool isRunning;
    bool isFirstFrame;
    std::vector<TrackedObject> trackedObjects;
    int nextObjectId;
    size_t maxTrajectoryPoints;  // Moved from const to configurable
    size_t minTrajectoryLength;  // Minimum trajectory points required to record

    // Configurable parameters (all loaded from config.yaml)
    double thresholdValue;
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
    std::string opticalFlowMode;  // "none", "farneback", "lucas-kanade"
    double motionHistoryDuration;  // Seconds (0 = disabled)
    
    // ===============================
    // THRESHOLDING
    // ===============================
    std::string thresholdType;  // "binary", "adaptive", "otsu"
    
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
    // VISUALIZATION & OUTPUT
    // ===============================
    bool splitScreen;
    bool drawContours;
    bool dataCollection;
    bool saveOnMotion;
    std::string splitScreenWindowName;
    
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
    cv::Ptr<cv::BackgroundSubtractorMOG2> bgSubtractor;
    
    // Edge Detection (Canny)
    int cannyLowThreshold;
    int cannyHighThreshold;
    
    // Adaptive Thresholding
    int adaptiveBlockSize;
    int adaptiveC;
    
    // HSV Color Filtering
    cv::Scalar hsvLower;
    cv::Scalar hsvUpper;
    
    // Motion History (for visualization)
    cv::Mat motionHistory;
    double motionHistoryFps;
    
    // Helper method for position smoothing
    cv::Point smoothPosition(const cv::Point& newPos, const cv::Point& smoothedPos);

    std::vector<int> lostObjectIds;
    std::string generateUUID();
};

#endif // MOTION_TRACKER_HPP 