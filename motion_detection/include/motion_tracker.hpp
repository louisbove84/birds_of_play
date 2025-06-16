#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>

struct MotionResult {
    bool hasMotion;
    std::vector<cv::Rect> motionRegions;
};

class MotionTracker {
public:
    MotionTracker();
    MotionResult processFrame(const cv::Mat& frame);

private:
    cv::Mat previousFrame;
    bool isFirstFrame;
    
    // Motion detection parameters
    const double MOTION_THRESHOLD = 25.0;
    const int MIN_MOTION_AREA = 500;
}; 