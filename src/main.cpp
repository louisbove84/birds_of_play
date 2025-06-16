#include "camera_manager.hpp"
#include "motion_tracker.hpp"
#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    // Create instances of our classes
    CameraManager camera;
    MotionTracker motionTracker;
    
    // Initialize camera
    if (!camera.initialize()) {
        std::cerr << "Failed to initialize camera!" << std::endl;
        return -1;
    }
    
    // Initialize motion tracker
    motionTracker.initialize();
    
    // Main loop
    cv::Mat frame, outputFrame;
    while (true) {
        // Get frame from camera
        if (!camera.getFrame(frame)) {
            std::cerr << "Failed to get frame from camera!" << std::endl;
            break;
        }
        
        // Process frame for motion detection
        if (!motionTracker.processFrame(frame, outputFrame)) {
            std::cerr << "Failed to process frame!" << std::endl;
            break;
        }
        
        // Display the result
        cv::imshow("Motion Tracking", outputFrame);
        
        // Check for exit key (ESC)
        char key = cv::waitKey(1);
        if (key == 27) { // ESC key
            break;
        }
    }
    
    // Cleanup
    camera.release();
    cv::destroyAllWindows();
    
    return 0;
} 