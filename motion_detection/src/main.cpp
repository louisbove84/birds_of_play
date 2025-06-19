#include "motion_tracker.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

// Colors for different tracked objects (cycled through based on object ID)
const std::vector<cv::Scalar> COLORS = {
    cv::Scalar(0, 255, 0),    // Green
    cv::Scalar(255, 0, 0),    // Blue
    cv::Scalar(0, 0, 255),    // Red
    cv::Scalar(255, 255, 0),  // Cyan
    cv::Scalar(255, 0, 255),  // Magenta
    cv::Scalar(0, 255, 255)   // Yellow
};

cv::Scalar getColor(int objectId) {
    return COLORS[objectId % COLORS.size()];
}

int main() {
    // Use config file from build directory
    MotionTracker tracker("config.yaml");
    
    if (!tracker.initialize(0)) {
        std::cerr << "Error: Could not open video source" << std::endl;
        return -1;
    }
    
    cv::Mat frame;
    while (true) {
        // Get frame from video source
        if (!tracker.readFrame(frame)) {
            break;
        }
        
        // Process frame
        MotionResult result = tracker.processFrame(frame);
        
        // Draw tracked objects and their paths
        if (result.hasMotion) {
            for (const auto& obj : result.trackedObjects) {
                cv::Scalar color = getColor(obj.id);
                
                // Draw bounding box
                cv::rectangle(frame, obj.currentBounds, color, 2);
                
                // Draw trajectory
                if (obj.trajectory.size() > 1) {
                    for (size_t i = 1; i < obj.trajectory.size(); ++i) {
                        cv::line(frame, 
                                obj.trajectory[i-1],
                                obj.trajectory[i],
                                color, 2);
                    }
                }
                
                // Draw object ID
                cv::Point textPos(obj.currentBounds.x, obj.currentBounds.y - 10);
                cv::putText(frame, 
                           "Object " + std::to_string(obj.id),
                           textPos,
                           cv::FONT_HERSHEY_SIMPLEX,
                           0.5, color, 2);
            }
        }
        
        // Display frame
        cv::imshow("Motion Detection", frame);
        
        // Check for exit key
        if (cv::waitKey(1) == MotionTracker::getEscKey()) {
            break;
        }
    }
    
    // Cleanup
    cv::destroyAllWindows();
    tracker.stop();
    
    return 0;
} 