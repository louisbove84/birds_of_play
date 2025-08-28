#include "motion_detection/include/motion_processor.hpp"         // MotionProcessor class
#include "motion_detection/include/motion_region_consolidator.hpp" // MotionRegionConsolidator class
#include "motion_detection/include/logger.hpp"                   // LOG_INFO, LOG_ERROR, LOG_DEBUG macros
#include "motion_detection/include/motion_pipeline.hpp"          // Unified processing pipeline
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
    // Default to motion_detection config if no path provided
    fs::path config_path = (argc > 1) ? argv[1] : "motion_detection/config.yaml";
    
    if (!fs::exists(config_path)) {
        std::cerr << "Error: Configuration file not found: " << config_path << std::endl;
        std::cerr << "Usage: " << argv[0] << " [config_path]" << std::endl;
        std::cerr << "Default config: motion_detection/config.yaml" << std::endl;
        return -1;
    }
    
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
    LOG_INFO("Birds of Play Motion Detection Demo - Logger initialized at {}", logFilePath);

    // Initialize motion processor and motion region consolidator
    MotionProcessor motionProcessor(config_path.string());
    MotionRegionConsolidator regionConsolidator;
    
    // Initialize camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        LOG_CRITICAL("Error: Could not open camera.");
        std::cerr << "Error: Could not open camera. Please check if your webcam is connected." << std::endl;
        return -1;
    }
    


    LOG_INFO("🐦 Birds of Play Motion Detection System initialized successfully!");
    LOG_INFO("📹 Using webcam for live motion detection and region consolidation");
    LOG_INFO("⌨️  Press 'q' or ESC to quit, 's' to save current frame");
    
    std::cout << "\n🐦 Birds of Play - Live Motion Detection Demo" << std::endl;
    std::cout << "📹 Camera initialized successfully!" << std::endl;
    std::cout << "⌨️  Controls:" << std::endl;
    std::cout << "   'q' or ESC - Quit application" << std::endl;
    std::cout << "   's' - Save current frame with detections" << std::endl;
    std::cout << "\n🔍 Watch for:" << std::endl;
    std::cout << "   🟢 Green/Blue/Red boxes - Individual motion detections" << std::endl;
    std::cout << "   ⬜ White boxes - Consolidated motion regions" << std::endl;
    std::cout << "\nStarting live detection..." << std::endl;

    cv::Mat frame;
    char key = 0;
    int frameCount = 0;

    while (key != 'q' && key != 27) { // Loop until 'q' or ESC is pressed
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Could not read frame from camera." << std::endl;
            break;
        }

        frameCount++;

        // Use unified function to process frame and consolidate regions
        // Only save visualization occasionally to avoid disk spam
        std::string visualizationPath = "";
        if (frameCount % 30 == 0) { // Save every 30 frames (~1 second at 30fps)
            visualizationPath = "motion_frame_" + std::to_string(frameCount) + ".jpg";
        }
        
        auto [processingResult, consolidatedRegions] = processFrameAndConsolidate(
            motionProcessor, regionConsolidator, frame, visualizationPath);
        
        // Log consolidated regions for debugging
        if (!consolidatedRegions.empty()) {
            LOG_DEBUG("Frame {}: {} motion detections -> {} consolidated regions", 
                     frameCount, processingResult.detectedBounds.size(), consolidatedRegions.size());
        }
        
        // Create live display frame
        cv::Mat displayFrame = frame.clone();
        
        // Draw individual motion detections
        for (size_t i = 0; i < processingResult.detectedBounds.size(); ++i) {
            const auto& bounds = processingResult.detectedBounds[i];
            cv::Scalar color = getColor(i);
            cv::rectangle(displayFrame, bounds, color, 2);
            
            // Add motion detection label
            std::string info = "Motion:" + std::to_string(i);
            cv::putText(displayFrame, info, cv::Point(bounds.x, bounds.y - 10),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }
        
        // Draw consolidated regions (higher priority - drawn on top)
        for (size_t i = 0; i < consolidatedRegions.size(); ++i) {
            const auto& region = consolidatedRegions[i];
            cv::Scalar regionColor = cv::Scalar(255, 255, 255); // White for consolidated regions
            cv::rectangle(displayFrame, region.boundingBox, regionColor, 3);
            
            std::string regionInfo = "Region:" + std::to_string(i) + " (" + std::to_string(region.trackedObjectIds.size()) + " objs)";
            cv::putText(displayFrame, regionInfo, cv::Point(region.boundingBox.x, region.boundingBox.y - 30),
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, regionColor, 2);
        }

        // Add status overlay
        std::string status = "Frame: " + std::to_string(frameCount) + 
                           " | Motions: " + std::to_string(processingResult.detectedBounds.size()) +
                           " | Regions: " + std::to_string(consolidatedRegions.size());
        cv::putText(displayFrame, status, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

        // Show the live feed
        cv::imshow("🐦 Birds of Play - Motion Detection", displayFrame);
        key = cv::waitKey(1);
        
        // Handle 's' key to save current frame
        if (key == 's' || key == 'S') {
            std::string saveFileName = "saved_detection_frame_" + std::to_string(frameCount) + ".jpg";
            cv::imwrite(saveFileName, displayFrame);
            std::cout << "💾 Saved current frame to: " << saveFileName << std::endl;
            LOG_INFO("User saved frame: {}", saveFileName);
        }
    }



    // Cleanup
    cap.release();
    cv::destroyAllWindows();
    
    std::cout << "\n👋 Birds of Play Motion Detection Demo ended." << std::endl;
    std::cout << "📊 Processed " << frameCount << " frames total." << std::endl;
    LOG_INFO("Application ended after processing {} frames", frameCount);

    return 0;
}
