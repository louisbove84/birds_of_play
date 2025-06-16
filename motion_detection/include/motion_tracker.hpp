#ifndef MOTION_TRACKER_HPP
#define MOTION_TRACKER_HPP

// System includes
#include <string>

// Third-party includes
#include <opencv2/opencv.hpp>

// Project includes
// (none for now)

struct MotionResult {
    bool hasMotion;
    std::vector<cv::Rect> motionRegions;
};

class MotionTracker {
public:
    MotionTracker();
    ~MotionTracker();

    bool initialize(const std::string& videoSource = "0");
    void processFrame();
    void stop();

    MotionResult processFrame(const cv::Mat& frame);

private:
    cv::VideoCapture cap;
    cv::Mat prevFrame;
    cv::Mat currentFrame;
    bool isRunning;
    std::vector<cv::Rect> detectedObjects;
    
    // Motion detection parameters
    const double MOTION_THRESHOLD = 25.0;
    const int MIN_MOTION_AREA = 500;
};

#endif // MOTION_TRACKER_HPP 