#include "motion_tracker.hpp"
#include <iostream>
#include <cmath>

MotionTracker::MotionTracker(const std::string& configPath) 
    : isRunning(false),
      isFirstFrame(true),
      nextObjectId(0),
      maxTrajectoryPoints(30),
      thresholdValue(25.0),
      minContourArea(500),
      maxTrackingDistance(100.0),
      maxThreshold(255) {
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
        if (config["max_tracking_distance"]) maxTrackingDistance = config["max_tracking_distance"].as<double>();
        if (config["max_trajectory_points"]) maxTrajectoryPoints = config["max_trajectory_points"].as<size_t>();
        if (config["max_threshold"]) maxThreshold = config["max_threshold"].as<int>();
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not load config file: " << e.what() << ". Using defaults." << std::endl;
    }
}

TrackedObject* MotionTracker::findNearestObject(const cv::Rect& newBounds) {
    cv::Point newCenter(newBounds.x + newBounds.width/2, 
                       newBounds.y + newBounds.height/2);
    
    TrackedObject* nearest = nullptr;
    double minDistance = maxTrackingDistance;
    
    for (auto& obj : trackedObjects) {
        cv::Point objCenter = obj.getCenter();
        double distance = std::sqrt(std::pow(newCenter.x - objCenter.x, 2) + 
                                  std::pow(newCenter.y - objCenter.y, 2));
        
        if (distance < minDistance) {
            minDistance = distance;
            nearest = &obj;
        }
    }
    
    return nearest;
}

void MotionTracker::updateTrajectories(std::vector<cv::Rect>& newBounds) {
    // Mark all current objects for potential removal
    std::vector<bool> objectMatched(trackedObjects.size(), false);
    
    // Try to match new bounds with existing objects
    for (const auto& bounds : newBounds) {
        TrackedObject* matchedObj = findNearestObject(bounds);
        
        if (matchedObj != nullptr) {
            // Update existing object
            size_t idx = matchedObj - &trackedObjects[0];
            if (idx < objectMatched.size()) {  // Add bounds check
                objectMatched[idx] = true;
                matchedObj->currentBounds = bounds;
                matchedObj->trajectory.push_back(matchedObj->getCenter());
                if (matchedObj->trajectory.size() > maxTrajectoryPoints) {
                    matchedObj->trajectory.pop_front();
                }
            }
        } else {
            // Create new tracked object
            TrackedObject newObj;
            newObj.id = nextObjectId++;
            newObj.currentBounds = bounds;
            newObj.trajectory.push_back(cv::Point(bounds.x + bounds.width/2,
                                                bounds.y + bounds.height/2));
            trackedObjects.push_back(newObj);
        }
    }
    
    // Remove objects that weren't matched
    for (int i = trackedObjects.size() - 1; i >= 0; --i) {
        if (i < static_cast<int>(objectMatched.size()) && !objectMatched[i]) {
            trackedObjects.erase(trackedObjects.begin() + i);
        }
    }
}

MotionResult MotionTracker::processFrame(const cv::Mat& frame) {
    MotionResult result;
    result.hasMotion = false;
    
    if (frame.empty()) {
        return result;
    }
    
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
    cv::threshold(frameDiff, thresh, thresholdValue, maxThreshold, cv::THRESH_BINARY);
    
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Process contours and update trajectories
    std::vector<cv::Rect> newBounds;
    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area > minContourArea) {
            result.hasMotion = true;
            newBounds.push_back(cv::boundingRect(contour));
        }
    }
    
    // Update object trajectories
    updateTrajectories(newBounds);
    result.trackedObjects = trackedObjects;
    
    // Update previous frame
    prevFrame = grayFrame.clone();
    
    return result;
} 