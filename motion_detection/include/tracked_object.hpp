#ifndef TRACKED_OBJECT_HPP
#define TRACKED_OBJECT_HPP

#include <opencv2/opencv.hpp>
#include <vector>

struct TrackedObject {
    int id;
    cv::Rect currentBounds;
    std::vector<cv::Point> trajectory;
    double confidence;
    cv::Ptr<cv::KalmanFilter> kalmanFilter;
    bool updated; // Flag to check if the object was updated in the current frame
};

struct TrackedObjectData {
    int id;
    std::vector<cv::Point> trajectory;
    cv::Rect bounds;
    double confidence;
};

#endif // TRACKED_OBJECT_HPP 