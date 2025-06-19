#ifndef MOTION_TRACKER_HPP
#define MOTION_TRACKER_HPP

// System includes
#include <string>
#include <vector>
#include <deque>

// Third-party includes
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/bgsegm.hpp>
#include <yaml-cpp/yaml.h>

// Maximum number of points to store in trajectory (moved to config.yaml)
// const size_t MAX_TRAJECTORY_POINTS = 30;

struct TrackedObject {
    int id;                                     // Unique identifier for this object
    cv::Rect currentBounds;                     // Current bounding box
    std::deque<cv::Point> trajectory;           // Path history (recent positions)
    cv::Point smoothedCenter;  // Add smoothed center position
    double confidence;         // Add tracking confidence
    cv::Point getCenter() const {
        return cv::Point(currentBounds.x + currentBounds.width/2,
                        currentBounds.y + currentBounds.height/2);
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
    
    // Initialize background subtractor
    void initializeBackgroundSubtractor();

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

    // Configurable parameters (all loaded from config.yaml)
    double thresholdValue;
    int minContourArea;
    double maxTrackingDistance;
    int maxThreshold;
    static const int ESC_KEY = 27;  // Keep this as const since it's a key code

    // Smoothing parameters
    double smoothingFactor;
    double minTrackingConfidence;
    
    // Image Processing Technique parameters
    bool enableGaussianBlur;
    int gaussianBlurSize;
    
    bool enableMorphology;
    int morphologyKernelSize;
    bool enableMorphClose;
    bool enableMorphOpen;
    bool enableDilation;
    bool enableErosion;
    
    bool enableContrastEnhancement;
    double claheClipLimit;
    int claheTileSize;
    
    bool enableMedianBlur;
    int medianBlurSize;
    
    bool enableBilateralFilter;
    int bilateralD;
    double bilateralSigmaColor;
    double bilateralSigmaSpace;
    
    bool enableAdaptiveThreshold;
    int adaptiveBlockSize;
    int adaptiveC;
    
    // Background Subtraction parameters
    bool enableBackgroundSubtraction;
    int backgroundHistory;
    double backgroundThreshold;
    bool backgroundDetectShadows;
    cv::Ptr<cv::BackgroundSubtractorMOG2> bgSubtractor;
    
    // Convex Hull parameters
    bool enableConvexHull;
    bool convexHullFill;
    
    // HSV Color Filtering parameters
    bool enableHsvFiltering;
    cv::Scalar hsvLower;
    cv::Scalar hsvUpper;
    
    // Edge Detection parameters
    bool enableEdgeDetection;
    int cannyLowThreshold;
    int cannyHighThreshold;
    
    // Contour Processing parameters
    bool enableContourApproximation;
    double contourEpsilonFactor;
    bool enableContourFiltering;
    double minAspectRatio;
    double maxAspectRatio;
    double minSolidity;
    
    // Visualization parameters
    bool enableSplitScreen;
    std::string splitScreenWindowName;
    
    // Helper method for position smoothing
    cv::Point smoothPosition(const cv::Point& newPos, const cv::Point& smoothedPos);
};

#endif // MOTION_TRACKER_HPP 