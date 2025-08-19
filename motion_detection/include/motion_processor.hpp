#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <memory>

/**
 * @brief Pure frame processing class - handles image preprocessing, motion detection,
 *        morphological operations, and contour extraction
 */
class MotionProcessor {
public:
    struct ProcessingResult {
        cv::Mat processedFrame;
        cv::Mat frameDiff;
        cv::Mat thresh;
        cv::Mat morphological;
        std::vector<cv::Rect> detectedBounds;
        bool hasMotion = false;
    };

    explicit MotionProcessor(const std::string& configPath);
    ~MotionProcessor() = default;

    // Main processing pipeline
    ProcessingResult processFrame(const cv::Mat& frame);
    
    // Individual processing steps (public for testing)
    cv::Mat preprocessFrame(const cv::Mat& frame);
    cv::Mat detectMotion(const cv::Mat& processedFrame, cv::Mat& frameDiff, cv::Mat& thresh);
    cv::Mat applyMorphologicalOps(const cv::Mat& thresh);
    std::vector<cv::Rect> extractContours(const cv::Mat& processed);
    
    // Frame management
    void setPrevFrame(const cv::Mat& frame);
    bool isFirstFrame() const { return firstFrame; }
    void setFirstFrame(bool first) { firstFrame = first; }

    // Configuration getters
    int getMinContourArea() const { return minContourArea; }
    int getMaxThreshold() const { return maxThreshold; }
    bool isBackgroundSubtractionEnabled() const { return backgroundSubtraction; }
    
    // Adaptive contour detection methods
    double calculateAdaptiveMinArea(const std::vector<std::vector<cv::Point>>& contours);
    double calculateAdaptiveMinSolidity(const std::vector<std::vector<cv::Point>>& contours);
    double calculateAdaptiveMaxAspectRatio(const std::vector<std::vector<cv::Point>>& contours);

private:
    // Configuration loading
    void loadConfig(const std::string& configPath);
    void initializeBackgroundSubtractor();

    // Frame state
    cv::Mat prevFrame;
    bool firstFrame = true;

    // Background subtraction
    cv::Ptr<cv::BackgroundSubtractor> bgSubtractor;

    // ===============================
    // CONFIGURATION PARAMETERS
    // ===============================
    
    // Basic parameters
    int minContourArea;
    int maxThreshold;
    
    // INPUT COLOR PROCESSING
    std::string processingMode;
    
    // IMAGE PREPROCESSING
    bool contrastEnhancement;
    std::string blurType;
    double claheClipLimit;
    int claheTileSize;
    int gaussianBlurSize;
    int medianBlurSize;
    int bilateralD;
    double bilateralSigmaColor;
    double bilateralSigmaSpace;
    
    // MOTION DETECTION METHODS
    bool backgroundSubtraction;
    
    // MORPHOLOGICAL OPERATIONS
    bool morphology;
    int morphKernelSize;
    bool morphClose;
    bool morphOpen;
    bool dilation;
    bool erosion;
    
    // CONTOUR PROCESSING
    bool convexHull;
    bool contourApproximation;
    bool contourFiltering;
    double maxContourAspectRatio;
    double minContourSolidity;
    double contourEpsilonFactor;
    std::string contourDetectionMode;
    

};
