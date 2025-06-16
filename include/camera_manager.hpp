#pragma once

#include <opencv2/opencv.hpp>
#include <memory>
#include <string>

class CameraManager {
public:
    CameraManager();
    ~CameraManager();

    // Initialize camera with given device ID
    bool initialize(int deviceId = 0);
    
    // Get the next frame from the camera
    bool getFrame(cv::Mat& frame);
    
    // Release camera resources
    void release();

private:
    cv::VideoCapture camera_;
    bool isInitialized_;
}; 