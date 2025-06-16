#include "motion_tracker.hpp"

MotionTracker::MotionTracker() : isRunning(false), isFirstFrame(true) {}

MotionTracker::~MotionTracker() {
    stop();
}

bool MotionTracker::initialize(const std::string& videoSource) {
    cap.open(videoSource);
    if (!cap.isOpened()) {
        return false;
    }
    isRunning = true;
    return true;
}

void MotionTracker::stop() {
    isRunning = false;
    if (cap.isOpened()) {
        cap.release();
    }
}

MotionResult MotionTracker::processFrame(const cv::Mat& frame) {
    MotionResult result;
    result.hasMotion = false;
    
    // Convert frame to grayscale
    cv::Mat grayFrame;
    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
    
    if (isFirstFrame) {
        prevFrame = grayFrame.clone();
        isFirstFrame = false;
        return result;
    }
    
    // Calculate absolute difference between frames
    cv::Mat frameDiff;
    cv::absdiff(prevFrame, grayFrame, frameDiff);
    
    // Apply threshold
    cv::Mat thresh;
    cv::threshold(frameDiff, thresh, MOTION_THRESHOLD, MAX_THRESHOLD, cv::THRESH_BINARY);
    
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Process contours
    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area > MIN_MOTION_AREA) {
            result.hasMotion = true;
            result.motionRegions.push_back(cv::boundingRect(contour));
        }
    }
    
    // Update previous frame
    prevFrame = grayFrame.clone();
    
    return result;
} 