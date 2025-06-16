#include "motion_tracker.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    MotionTracker tracker;
    
    if (!tracker.initialize()) {
        std::cerr << "Error: Could not open video source" << std::endl;
        return -1;
    }
    
    cv::Mat frame;
    while (true) {
        // Get frame from video source
        if (!tracker.cap.read(frame)) {
            break;
        }
        
        // Process frame
        MotionResult result = tracker.processFrame(frame);
        
        // Draw motion regions
        if (result.hasMotion) {
            for (const auto& rect : result.motionRegions) {
                cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2);
            }
        }
        
        // Display frame
        cv::imshow("Motion Detection", frame);
        
        // Check for exit key
        if (cv::waitKey(1) == tracker.ESC_KEY) {
            break;
        }
    }
    
    // Cleanup
    cv::destroyAllWindows();
    tracker.stop();
    
    return 0;
} 