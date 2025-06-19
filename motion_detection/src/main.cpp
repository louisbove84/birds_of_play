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
    
    // Initialize motion tracker with camera
    if (!tracker.initialize(0)) {
        std::cerr << "Error: Could not initialize motion tracker with camera." << std::endl;
        return -1;
    }
    
    if (!collector.initialize()) {
        std::cerr << "Warning: Data collection disabled or failed to initialize" << std::endl;
    }

    std::cout << "Motion tracking system initialized successfully!" << std::endl;
    std::cout << "Press 'q' or ESC to quit." << std::endl;

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

        // Handle objects that were lost in this frame
        std::vector<int> lostObjectIds = tracker.getLostObjectIds();
        for (int lostId : lostObjectIds) {
            collector.handleObjectLost(lostId);
        }

        // Update data collection for each tracked object
        if (result.hasMotion) {
            for (const auto& obj : result.trackedObjects) {
                if (obj.trajectory.size() >= tracker.getMinTrajectoryLength()) {
                    collector.addTrackingData(
                        obj.id,
                        frame,
                        obj.currentBounds,
                        obj.trajectory.back(),  // Current position is the last point in trajectory
                        obj.confidence
                    );
                }
            }
        }

        // Draw enhanced motion overlays (only for objects with sufficient trajectory length)
        cv::Mat filteredFrame = frame.clone();
        std::vector<TrackedObject> filteredObjects;
        for (const auto& obj : result.trackedObjects) {
            if (obj.trajectory.size() >= tracker.getMinTrajectoryLength()) {
                filteredObjects.push_back(obj);
            }
        }
        // Temporarily replace trackedObjects for overlay drawing
        auto originalTrackedObjects = tracker.getTrackedObjects();
        tracker.setTrackedObjects(filteredObjects);
        
        // Use split-screen visualization as the main display
        cv::Mat displayFrame;
        if (tracker.isSplitScreenEnabled()) {
            // Get the split-screen visualization which includes black backgrounds
            displayFrame = tracker.getSplitScreenVisualization(frame);
        } else {
            // Fallback to original frame with overlays
            displayFrame = tracker.drawMotionOverlays(filteredFrame);
        }
        
        tracker.setTrackedObjects(originalTrackedObjects);

        // Show the frame with overlays
        cv::imshow("Motion Tracking", displayFrame);
        key = cv::waitKey(1);
    }

    // Save final data before exit
    collector.saveData();

    // Cleanup
    cap.release();
    cv::destroyAllWindows();

    return 0;
} 