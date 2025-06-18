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
#include <yaml-cpp/yaml.h>

// Project includes
// (none for now)

struct MotionResult {
    bool hasMotion;
    std::vector<cv::Rect> motionRegions;
};

class MotionTracker {
public:
    MotionTracker(const std::string& configPath = "config.yaml");
    ~MotionTracker();

    bool initialize(const std::string& videoSource = "0");
    bool initialize(int deviceIndex);
    void processFrame();
    void stop();
    MotionResult processFrame(const cv::Mat& frame);
    
    // New public methods
    bool readFrame(cv::Mat& frame) { return cap.read(frame); }
    static int getEscKey() { return ESC_KEY; }

private:
    void loadConfig(const std::string& configPath);
    cv::VideoCapture cap;
    cv::Mat prevFrame;
    cv::Mat currentFrame;
    bool isRunning;
    bool isFirstFrame;
    std::vector<cv::Rect> detectedObjects;
    
    // Configurable parameters
    double thresholdValue = 25.0;
    int minContourArea = 500;
    const int MAX_THRESHOLD = 255;
    static const int ESC_KEY = 27;
};

#endif // MOTION_TRACKER_HPP 