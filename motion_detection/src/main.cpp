#include "motion_tracker.hpp"           // MotionTracker class, TrackedObject struct
#include "data_collector.hpp"           // DataCollector class, MongoDB integration
#include "logger.hpp"                   // LOG_INFO, LOG_ERROR, LOG_DEBUG macros
#include <opencv2/opencv.hpp>           // cv::Mat, cv::VideoCapture, cv::imshow, etc.
#include <iostream>                     // std::cout, std::cerr, std::endl
#include <filesystem>                   // std::filesystem (fs::path, fs::create_directories)
#include <string>                       // std::string
#include <yaml-cpp/yaml.h>              // YAML::Node, YAML::LoadFile
namespace fs = std::filesystem;         // Shorthand for std::filesystem

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

int main(int argc, char** argv) {
    fs::path config_path = (argc > 1) ? argv[1] : "config.yaml";
    YAML::Node config = YAML::LoadFile(config_path.string());

    // Initialize logger first
    std::string logLevel = config["logging"]["log_level"] ? config["logging"]["log_level"].as<std::string>() : "info";
    bool logToFile = config["logging"]["log_to_file"] ? config["logging"]["log_to_file"].as<bool>() : false;
    std::string logFilePath = config["logging"]["log_file_path"] ? config["logging"]["log_file_path"].as<std::string>() : "birdsofplay.log";
    
    // Ensure log directory exists (e.g., "logs/")
    fs::path logDir = fs::path(logFilePath).parent_path();
    if (!logDir.empty()) {
        fs::create_directories(logDir);
    }
    Logger::init(logLevel, logFilePath, logToFile);
    LOG_INFO("Logger initialized at {}", logFilePath);

    // Initialize motion tracker and data collector
    MotionTracker tracker(config_path.string());
    DataCollector collector(config_path.string());
    
    // Initialize motion tracker with camera
    if (!tracker.initialize(0)) {
        LOG_CRITICAL("Error: Could not initialize motion tracker with camera.");
        return -1;
    }
    
    if (!collector.initialize()) {
        LOG_WARN("Data collection disabled or failed to initialize");
    }

    LOG_INFO("Motion tracking system initialized successfully!");
    LOG_INFO("Press 'q' or ESC to quit.");

    cv::VideoCapture cap; // Define cap here
    if (!tracker.getCap().isOpened()) {
        LOG_CRITICAL("Camera is not open after tracker initialization.");
        return -1;
    }
    cap = tracker.getCap(); // Get the already-opened capture from the tracker

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
                        obj.confidence,
                        obj.classLabel,
                        obj.classConfidence,
                        obj.classId
                    );
                }
            }
        }

        // Update data collection for lost objects
        for (const auto& id : tracker.getLostObjectIds()) {
            LOG_DEBUG("Object {} lost. Forwarding to data collector.", id);
            // We need the full object, not just the ID. This requires a small change in MotionTracker.
            // For now, let's assume a function getLostObjectById exists.
            // This will be fixed in the next step.
            const auto* obj = tracker.findTrackedObjectById(id);
            if (obj) {
                collector.addLostObject(*obj);
            }
        }
        tracker.clearLostObjectIds();

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