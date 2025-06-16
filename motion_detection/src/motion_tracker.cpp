#include "../include/motion_tracker.hpp"

MotionTracker::MotionTracker() : isFirstFrame(true) {}

MotionResult MotionTracker::processFrame(const cv::Mat& frame) {
    MotionResult result;
    result.hasMotion = false;
    
    // Convert frame to grayscale
    cv::Mat grayFrame;
    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
    
    // If this is the first frame, store it and return
    if (isFirstFrame) {
        previousFrame = grayFrame.clone();
        isFirstFrame = false;
        return result;
    }
    
    // Calculate absolute difference between frames
    cv::Mat frameDiff;
    cv::absdiff(previousFrame, grayFrame, frameDiff);
    
    // Apply threshold to get binary image
    cv::Mat thresh;
    cv::threshold(frameDiff, thresh, MOTION_THRESHOLD, 255, cv::THRESH_BINARY);
    
    // Find contours of motion
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Filter and store motion regions
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > MIN_MOTION_AREA) {
            result.motionRegions.push_back(cv::boundingRect(contour));
            result.hasMotion = true;
        }
    }
    
    // Update previous frame
    previousFrame = grayFrame.clone();
    
    return result;
} 