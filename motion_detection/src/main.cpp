#include "../include/motion_tracker.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Create motion tracker
    MotionTracker tracker;
    
    // Open default camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera." << std::endl;
        return -1;
    }
    
    cv::Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Empty frame." << std::endl;
            break;
        }
        
        // Process frame
        auto result = tracker.processFrame(frame);
        
        // Draw motion regions
        if (result.hasMotion) {
            for (const auto& rect : result.motionRegions) {
                cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2);
            }
        }
        
        // Display result
        cv::imshow("Motion Detection", frame);
        
        // Break loop on 'ESC' key
        if (cv::waitKey(1) == 27) {
            break;
        }
    }
    
    cap.release();
    cv::destroyAllWindows();
    return 0;
} 