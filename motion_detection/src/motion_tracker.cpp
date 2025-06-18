#include "motion_tracker.hpp"
#include <iostream>

MotionTracker::MotionTracker(const std::string& configPath) : isRunning(false), isFirstFrame(true) {
    loadConfig(configPath);
}

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

bool MotionTracker::initialize(int deviceIndex) {
    cap.open(deviceIndex);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video device index " << deviceIndex << std::endl;
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

void MotionTracker::loadConfig(const std::string& configPath) {
    try {
        YAML::Node config = YAML::LoadFile(configPath);
        if (config["threshold_value"]) thresholdValue = config["threshold_value"].as<double>();
        if (config["min_contour_area"]) minContourArea = config["min_contour_area"].as<int>();
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not load config file: " << e.what() << ". Using defaults." << std::endl;
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
    cv::threshold(frameDiff, thresh, thresholdValue, MAX_THRESHOLD, cv::THRESH_BINARY);
    
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Process contours
    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area > minContourArea) {
            result.hasMotion = true;
            result.motionRegions.push_back(cv::boundingRect(contour));
        }
    }
    
    // Update previous frame
    prevFrame = grayFrame.clone();
    
    return result;
} 