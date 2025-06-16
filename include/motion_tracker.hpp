#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

class MotionTracker {
public:
    MotionTracker();
    ~MotionTracker();

    // Initialize the motion tracker with parameters
    void initialize(double threshold = 25.0, int history = 500);
    
    // Process a frame and detect motion
    bool processFrame(const cv::Mat& frame, cv::Mat& outputFrame);
    
    // Get the current motion regions
    const std::vector<cv::Rect>& getMotionRegions() const;

private:
    cv::Ptr<cv::BackgroundSubtractor> bgSubtractor_;
    std::vector<cv::Rect> motionRegions_;
    double threshold_;
    int history_;
    
    // Helper methods
    void detectMotion(const cv::Mat& frame);
    void drawMotionRegions(cv::Mat& frame);
}; 