#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <deque>
#include <string>
#include <chrono>

/**
 * @brief TrackedObject struct definition - represents a tracked object with motion history
 * 
 * This struct contains all the information needed to track an object across frames,
 * including its current position, trajectory history, and metadata.
 */
struct TrackedObject {
    int id;
    cv::Rect currentBounds;
    std::deque<cv::Point> trajectory; // Using deque for efficient pop_front
    cv::Point smoothedCenter;
    double confidence;
    int framesWithoutDetection;
    std::chrono::system_clock::time_point firstSeen;
    std::string uuid;
    cv::Mat initialFrame;
    
    // Classification results (if needed by future components)
    std::string classLabel;
    float classConfidence;
    int classId;

    cv::Point getCenter() const {
        return cv::Point(currentBounds.x + currentBounds.width / 2,
                         currentBounds.y + currentBounds.height / 2);
    }

    TrackedObject(int obj_id, const cv::Rect& bounds, std::string new_uuid) : 
        id(obj_id), currentBounds(bounds), confidence(1.0), 
        framesWithoutDetection(0), firstSeen(std::chrono::system_clock::now()), uuid(new_uuid),
        classLabel("unknown"), classConfidence(0.0f), classId(-1) {
        smoothedCenter = getCenter();
        trajectory.push_back(smoothedCenter);
    }
};
