#include "motion_processor.hpp"         // MotionProcessor class
#include "motion_region_consolidator.hpp" // MotionRegionConsolidator class
#include "data_collector.hpp"           // DataCollector class, MongoDB integration
#include "logger.hpp"                   // LOG_INFO, LOG_ERROR, LOG_DEBUG macros
#include <opencv2/opencv.hpp>           // cv::Mat, cv::VideoCapture, cv::imshow, etc.
#include <iostream>                     // std::cout, std::cerr, std::endl
#include <filesystem>                   // std::filesystem (fs::path, fs::create_directories)
#include <string>                       // std::string
#include <ctime>                        // std::time for timestamp
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

    // Initialize motion processor, motion region consolidator, and data collector
    MotionProcessor motionProcessor(config_path.string());
    MotionRegionConsolidator regionConsolidator;
    DataCollector collector(config_path.string());
    
    // Initialize camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        LOG_CRITICAL("Error: Could not open camera.");
        return -1;
    }
    
    if (!collector.initialize()) {
        LOG_WARN("Data collection disabled or failed to initialize");
    }

    LOG_INFO("Motion tracking system initialized successfully!");
    LOG_INFO("Press 'q' or ESC to quit.");



    cv::Mat frame;
    char key = 0;

    while (key != 'q' && key != 27) { // Loop until 'q' or ESC is pressed
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Could not read frame." << std::endl;
            break;
        }

        // Process frame using motion processor
        MotionProcessor::ProcessingResult processingResult = motionProcessor.processFrame(frame);
        
        // Create simple TrackedObjects from detected bounds for region consolidation
        std::vector<TrackedObject> trackedObjects;
        static int nextId = 0; // Simple ID assignment
        
        for (const auto& bounds : processingResult.detectedBounds) {
            int currentId = nextId++;
            trackedObjects.emplace_back(currentId, bounds, "uuid_" + std::to_string(currentId));
        }
        
        // Consolidate motion regions for YOLO input with visualization
        std::vector<ConsolidatedRegion> consolidatedRegions;
        if (!trackedObjects.empty()) {
            // Use the original frame from motion processor for visualization
            std::string visualizationPath = "motion_consolidation_frame_" + std::to_string(static_cast<int>(std::time(nullptr))) + ".jpg";
            consolidatedRegions = regionConsolidator.consolidateRegionsWithVisualization(
                trackedObjects, processingResult.originalFrame, visualizationPath);
            
            LOG_INFO("Motion detected: {} objects -> {} consolidated regions", 
                     trackedObjects.size(), consolidatedRegions.size());
                     
            // TODO: Pass consolidated regions to YOLO11 for object detection
            for (const auto& region : consolidatedRegions) {
                LOG_DEBUG("Consolidated region: {}x{} at ({},{}) with {} objects",
                         region.boundingBox.width, region.boundingBox.height,
                         region.boundingBox.x, region.boundingBox.y,
                         region.trackedObjectIds.size());
            }
        }
        
        // Simple visualization - draw detected motion bounds
        cv::Mat displayFrame = frame.clone();
        for (size_t i = 0; i < processingResult.detectedBounds.size(); ++i) {
            const auto& bounds = processingResult.detectedBounds[i];
            cv::Scalar color = getColor(i);
            cv::rectangle(displayFrame, bounds, color, 2);
            
            // Add motion info
            std::string info = "Motion:" + std::to_string(i);
            cv::putText(displayFrame, info, cv::Point(bounds.x, bounds.y - 10),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }
        
        // Draw consolidated regions
        for (size_t i = 0; i < consolidatedRegions.size(); ++i) {
            const auto& region = consolidatedRegions[i];
            cv::Scalar regionColor = cv::Scalar(255, 255, 255); // White for consolidated regions
            cv::rectangle(displayFrame, region.boundingBox, regionColor, 3);
            
            std::string regionInfo = "Region:" + std::to_string(i) + " (" + std::to_string(region.trackedObjectIds.size()) + " objs)";
            cv::putText(displayFrame, regionInfo, cv::Point(region.boundingBox.x, region.boundingBox.y - 30),
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, regionColor, 2);
        }

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
