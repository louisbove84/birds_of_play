#include "motion_tracker.hpp"
#include "data_collector.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>

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
    // Initialize video capture
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera." << std::endl;
        return -1;
    }

    // Get config file path
    std::filesystem::path config_path = std::filesystem::current_path() / "config.yaml";
    if (!std::filesystem::exists(config_path)) {
        std::cerr << "Error: Could not find config.yaml" << std::endl;
        return -1;
    }

    // Initialize motion tracker and data collector
    MotionTracker tracker(config_path.string());
    DataCollector collector(config_path.string());
    
    if (!collector.initialize()) {
        std::cerr << "Warning: Data collection disabled or failed to initialize" << std::endl;
    }

    cv::Mat frame;
    char key = 0;

    while (key != 'q' && key != 27) { // Loop until 'q' or ESC is pressed
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Could not read frame." << std::endl;
            break;
        }

        // Process frame and get tracked objects
        MotionResult result = tracker.processFrame(frame);

        // Update data collection for each tracked object
        if (result.hasMotion) {
            for (const auto& obj : result.trackedObjects) {
                collector.addTrackingData(
                    obj.id,
                    frame,
                    obj.currentBounds,
                    obj.trajectory.back(),  // Current position is the last point in trajectory
                    obj.confidence
                );
            }

            // Draw tracking visualization
            for (const auto& obj : result.trackedObjects) {
                cv::Scalar color(0, 255, 0);  // Green color for tracking
                
                // Draw bounding box
                cv::rectangle(frame, obj.currentBounds, color, 2);
                
                // Draw trajectory
                if (obj.trajectory.size() > 1) {
                    for (size_t i = 1; i < obj.trajectory.size(); ++i) {
                        cv::line(frame, obj.trajectory[i-1], obj.trajectory[i], color, 2);
                    }
                }
                
                // Draw object ID
                cv::Point textPos(obj.currentBounds.x, obj.currentBounds.y - 10);
                cv::putText(frame, "Object " + std::to_string(obj.id),
                           textPos, cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
            }
        }

        // Show the frame
        cv::imshow("Motion Tracking", frame);
        key = cv::waitKey(1);
    }

    // Save final data before exit
    collector.saveData();

    // Cleanup
    cap.release();
    cv::destroyAllWindows();

    return 0;
} 