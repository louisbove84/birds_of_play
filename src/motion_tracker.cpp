#include "motion_tracker.hpp"

MotionTracker::MotionTracker() : threshold_(25.0), history_(500) {}

MotionTracker::~MotionTracker() {}

void MotionTracker::initialize(double threshold, int history) {
    threshold_ = threshold;
    history_ = history;
    bgSubtractor_ = cv::createBackgroundSubtractorMOG2(history, threshold, false);
}

bool MotionTracker::processFrame(const cv::Mat& frame, cv::Mat& outputFrame) {
    if (frame.empty()) {
        return false;
    }

    // Create a copy of the input frame for output
    frame.copyTo(outputFrame);
    
    // Detect motion
    detectMotion(frame);
    
    // Draw motion regions on the output frame
    drawMotionRegions(outputFrame);
    
    return true;
}

void MotionTracker::detectMotion(const cv::Mat& frame) {
    cv::Mat fgMask;
    bgSubtractor_->apply(frame, fgMask);
    
    // Apply morphological operations to remove noise
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_CLOSE, kernel);
    
    // Find contours of moving objects
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Clear previous motion regions
    motionRegions_.clear();
    
    // Filter and store significant motion regions
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > 500) { // Minimum area threshold
            motionRegions_.push_back(cv::boundingRect(contour));
        }
    }
}

void MotionTracker::drawMotionRegions(cv::Mat& frame) {
    for (const auto& rect : motionRegions_) {
        cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2);
    }
}

const std::vector<cv::Rect>& MotionTracker::getMotionRegions() const {
    return motionRegions_;
} 