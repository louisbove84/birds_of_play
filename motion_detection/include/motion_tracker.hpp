#ifndef MOTION_TRACKER_HPP
#define MOTION_TRACKER_HPP

// System includes
#include <string>
#include <vector>

// Third-party includes
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

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
    
    // New public methods
    bool readFrame(cv::Mat& frame) { return cap.read(frame); }
    static int getEscKey() { return ESC_KEY; }

private:
    cv::VideoCapture cap;
    cv::Mat prevFrame;
    cv::Mat currentFrame;
    bool isRunning;
    bool isFirstFrame;
    std::vector<cv::Rect> detectedObjects;
    
    // Motion detection parameters
    const double MOTION_THRESHOLD = 25.0;
    const int MIN_MOTION_AREA = 500;
    const int MAX_THRESHOLD = 255;
    static const int ESC_KEY = 27;
};

#endif // MOTION_TRACKER_HPP 