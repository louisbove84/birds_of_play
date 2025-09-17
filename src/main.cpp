#include "motion_detection/include/motion_processor.hpp"         // MotionProcessor class
#include "motion_detection/include/motion_region_consolidator.hpp" // MotionRegionConsolidator class
#include "motion_detection/include/logger.hpp"                   // LOG_INFO, LOG_ERROR, LOG_DEBUG macros
#include "motion_detection/include/motion_pipeline.hpp"          // Unified processing pipeline
#include <opencv2/opencv.hpp>           // cv::Mat, cv::VideoCapture, cv::imshow, etc.
#include <iostream>                     // std::cout, std::cerr, std::endl
#include <filesystem>                   // std::filesystem (fs::path, fs::create_directories)
#include <string>                       // std::string
#include <ctime>                        // std::time for timestamp
#include <chrono>                       // std::chrono for timing
#include <yaml-cpp/yaml.h>              // YAML::Node, YAML::LoadFile
#include <pybind11/embed.h>             // Python embedding
#include <pybind11/numpy.h>             // NumPy array support
namespace py = pybind11;
namespace fs = std::filesystem;         // Shorthand for std::filesystem

// Function declarations for Python bindings
std::string save_frame_to_mongodb(const cv::Mat& frame, const std::string& metadata_json);
std::string save_frames_to_mongodb(const cv::Mat& original_frame, const cv::Mat& processed_frame, const std::string& metadata_json);

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

// Function to save frame to MongoDB (declaration)
std::string save_frame_to_mongodb(const cv::Mat& frame, const std::string& metadata_json);

int main(int argc, char** argv) {
    // Initialize Python interpreter for MongoDB integration
    py::scoped_interpreter guard{};
    
    // Parse command line arguments
    fs::path config_path = (argc > 1) ? argv[1] : "motion_detection/config.yaml";
    std::string video_source = (argc > 2) ? argv[2] : "";  // Video file path (empty = use webcam)
    
    if (!fs::exists(config_path)) {
        std::cerr << "Error: Configuration file not found: " << config_path << std::endl;
        std::cerr << "Usage: " << argv[0] << " [config_path] [video_path]" << std::endl;
        std::cerr << "  config_path: Path to YAML configuration file (default: motion_detection/config.yaml)" << std::endl;
        std::cerr << "  video_path:  Path to video file (optional, default: use webcam)" << std::endl;
        return -1;
    }
    
    // Debug: Print what we're using
    std::cout << "🔧 Config file: " << config_path << std::endl;
    if (!video_source.empty()) {
        std::cout << "🎬 Video file: " << video_source << std::endl;
    } else {
        std::cout << "📹 Using webcam" << std::endl;
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
    
    // Read MongoDB configuration
    bool saveOnlyConsolidatedRegions = config["save_only_consolidated_regions"] ? 
                                      config["save_only_consolidated_regions"].as<bool>() : false;
    LOG_INFO("MongoDB save mode: {} frames", 
             saveOnlyConsolidatedRegions ? "consolidated regions only" : "all motion frames");

    // Initialize motion processor and motion region consolidator
    MotionProcessor motionProcessor(config_path.string());
    motionProcessor.setVisualizationPath(""); // Disable visualization file saving
    
    // Configure region consolidation
    ConsolidationConfig consolidationConfig;
    consolidationConfig.maxDistanceThreshold = config["max_distance_threshold"] ? 
                                             config["max_distance_threshold"].as<double>() : 150.0;
    consolidationConfig.minObjectsPerRegion = config["min_objects_per_region"] ? 
                                            config["min_objects_per_region"].as<int>() : 1;
    consolidationConfig.overlapThreshold = config["overlap_threshold"] ? 
                                         config["overlap_threshold"].as<double>() : 0.2;
    consolidationConfig.gridCellSize = config["grid_cell_size"] ? 
                                      config["grid_cell_size"].as<double>() : 150.0;
    consolidationConfig.maxFramesWithoutUpdate = config["max_frames_without_update"] ? 
                                                config["max_frames_without_update"].as<int>() : 10;
    consolidationConfig.minRegionArea = config["min_region_area"] ? 
                                       config["min_region_area"].as<double>() : 200.0;
    consolidationConfig.maxRegionArea = config["max_region_area"] ? 
                                       config["max_region_area"].as<double>() : 100000.0;
    consolidationConfig.regionExpansionFactor = config["region_expansion_factor"] ? 
                                               config["region_expansion_factor"].as<double>() : 1.1;
    consolidationConfig.idealModelRegionSize = config["ideal_model_region_size"] ? 
                                              config["ideal_model_region_size"].as<int>() : 640;
    consolidationConfig.sizeTolerancePercent = config["size_tolerance_percent"] ? 
                                             config["size_tolerance_percent"].as<int>() : 30;
    
    // Set frame size (will be updated when we know the actual video dimensions)
    consolidationConfig.frameSize = cv::Size(1920, 1080); // Default, will be updated from video
    
    MotionRegionConsolidator regionConsolidator(consolidationConfig);
    LOG_INFO("Region consolidation configured: maxDistance={}, minObjects={}, overlap={}", 
             consolidationConfig.maxDistanceThreshold, consolidationConfig.minObjectsPerRegion, 
             consolidationConfig.overlapThreshold);
    
    // Initialize video source (camera or video file)
    cv::VideoCapture cap;
    if (!video_source.empty()) {
        // Use video file
        cap.open(video_source);
        if (!cap.isOpened()) {
            LOG_CRITICAL("Error: Could not open video file: " + video_source);
            std::cerr << "Error: Could not open video file: " << video_source << std::endl;
            return -1;
        }
        LOG_INFO("📹 Opened video file: " + video_source);
    } else {
        // Use webcam
        cap.open(0);
        if (!cap.isOpened()) {
            LOG_CRITICAL("Error: Could not open camera.");
            std::cerr << "Error: Could not open camera. Please check if your webcam is connected." << std::endl;
            return -1;
        }
        LOG_INFO("📹 Opened webcam");
    }
    
    // Update frame size in consolidation config based on actual video dimensions
    cv::Mat testFrame;
    cap >> testFrame;
    if (!testFrame.empty()) {
        consolidationConfig.frameSize = testFrame.size();
        regionConsolidator.updateConfig(consolidationConfig);
        LOG_INFO("Updated consolidation frame size to: {}x{}", 
                 consolidationConfig.frameSize.width, consolidationConfig.frameSize.height);
    }
    // Reset video position to beginning
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    LOG_INFO("🐦 Birds of Play Motion Detection System initialized successfully!");
    if (!video_source.empty()) {
        LOG_INFO("🎬 Processing video file: " + video_source);
    } else {
        LOG_INFO("📹 Using webcam for live motion detection and region consolidation");
    }
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
    auto lastSaveTime = std::chrono::steady_clock::now();
    const auto saveInterval = std::chrono::seconds(1); // Save every 1 second

    while (key != 'q' && key != 27) { // Loop until 'q' or ESC is pressed
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Could not read frame from camera." << std::endl;
            break;
        }

        frameCount++;
        auto currentTime = std::chrono::steady_clock::now();

        // Use unified function to process frame and consolidate regions
        // Disable visualization file saving to avoid disk spam
        std::string visualizationPath = "";
        
        auto [processingResult, consolidatedRegions] = processFrameAndConsolidate(
            motionProcessor, regionConsolidator, frame, visualizationPath);
        
        // Log consolidated regions for debugging
        if (!consolidatedRegions.empty()) {
            LOG_DEBUG("Frame {}: {} motion detections -> {} consolidated regions", 
                     frameCount, processingResult.detectedBounds.size(), consolidatedRegions.size());
            for (size_t i = 0; i < consolidatedRegions.size(); ++i) {
                const auto& region = consolidatedRegions[i];
                LOG_DEBUG("  Region {}: {}x{} at ({},{}) with {} objects", 
                         i, region.boundingBox.width, region.boundingBox.height,
                         region.boundingBox.x, region.boundingBox.y, region.trackedObjectIds.size());
            }
        }
        
        // Create live display frame
        cv::Mat displayFrame = frame.clone();
        
        // Draw individual motion detections in gray (lower priority)
        for (size_t i = 0; i < processingResult.detectedBounds.size(); ++i) {
            const auto& bounds = processingResult.detectedBounds[i];
            cv::Scalar color = cv::Scalar(200, 200, 200); // Light gray for individual motion detections
            cv::rectangle(displayFrame, bounds, color, 1);
            
            // Add motion detection label
            std::string info = "M:" + std::to_string(i);
            cv::putText(displayFrame, info, cv::Point(bounds.x, bounds.y - 5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
        }
        
        // Draw consolidated regions in red (higher priority - drawn on top)
        for (size_t i = 0; i < consolidatedRegions.size(); ++i) {
            const auto& region = consolidatedRegions[i];
            cv::Scalar regionColor = cv::Scalar(0, 0, 255); // Red for consolidated regions
            cv::rectangle(displayFrame, region.boundingBox, regionColor, 3);
            
            std::string regionInfo = "Region:" + std::to_string(i) + " (" + std::to_string(region.trackedObjectIds.size()) + " objs)";
            cv::putText(displayFrame, regionInfo, cv::Point(region.boundingBox.x, region.boundingBox.y - 30),
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, regionColor, 2);
        }
        
        // Create MongoDB frame (clean frame with individual motion detections and consolidated regions)
        cv::Mat mongoFrame;
        if (saveOnlyConsolidatedRegions && !consolidatedRegions.empty()) {
            // For MongoDB: clean frame with both individual motion detections and consolidated regions
            mongoFrame = frame.clone();
            
            // Draw individual motion detections in gray (lower priority)
            for (size_t i = 0; i < processingResult.detectedBounds.size(); ++i) {
                const auto& bounds = processingResult.detectedBounds[i];
                cv::Scalar color = cv::Scalar(200, 200, 200); // Light gray for individual motion detections (BGR format)
                cv::rectangle(mongoFrame, bounds, color, 1);
                
                // Add motion detection label
                std::string info = "M:" + std::to_string(i);
                cv::putText(mongoFrame, info, cv::Point(bounds.x, bounds.y - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
            }
            
            // Draw consolidated regions in red (higher priority - drawn on top)
            for (size_t i = 0; i < consolidatedRegions.size(); ++i) {
                const auto& region = consolidatedRegions[i];
                cv::Scalar regionColor = cv::Scalar(0, 0, 255); // Red for consolidated regions (BGR format)
                cv::rectangle(mongoFrame, region.boundingBox, regionColor, 3);
                
                std::string regionInfo = "Region:" + std::to_string(i) + " (" + std::to_string(region.trackedObjectIds.size()) + " objs)";
                cv::putText(mongoFrame, regionInfo, cv::Point(region.boundingBox.x, region.boundingBox.y - 30),
                           cv::FONT_HERSHEY_SIMPLEX, 0.7, regionColor, 2);
            }
        } else {
            // Use the display frame (with both types of regions)
            mongoFrame = displayFrame;
        }

        // Auto-save frame every 1 second
        if (currentTime - lastSaveTime >= saveInterval) {
            // Check if we should save this frame based on configuration
            bool shouldSaveFrame = !saveOnlyConsolidatedRegions || !consolidatedRegions.empty();
            
            // Debug output
            LOG_DEBUG("Frame {}: saveOnlyConsolidatedRegions={}, consolidatedRegions.size()={}, shouldSaveFrame={}", 
                     frameCount, saveOnlyConsolidatedRegions, consolidatedRegions.size(), shouldSaveFrame);
            
            if (shouldSaveFrame) {
                try {
                // Create metadata JSON with motion detection info and consolidated regions
                std::string metadata = "{\"source\":\"motion_detection_cpp\",\"frame_count\":" + 
                                     std::to_string(frameCount) + ",\"timestamp\":\"" + 
                                     std::to_string(std::time(nullptr)) + "\",\"auto_saved\":true," +
                                     "\"motion_detected\":" + (processingResult.detectedBounds.empty() ? "false" : "true") + "," +
                                     "\"motion_regions\":" + std::to_string(processingResult.detectedBounds.size()) + "," +
                                     "\"consolidated_regions_count\":" + std::to_string(consolidatedRegions.size()) + "," +
                                     "\"confidence\":" + std::to_string(processingResult.detectedBounds.empty() ? 0.0 : 0.8);
                
                // Add consolidated regions coordinates for YOLO11 processing
                if (!consolidatedRegions.empty()) {
                    metadata += ",\"consolidated_regions\":[";
                    for (size_t i = 0; i < consolidatedRegions.size(); ++i) {
                        const auto& region = consolidatedRegions[i];
                        metadata += "{\"x\":" + std::to_string(region.boundingBox.x) + 
                                   ",\"y\":" + std::to_string(region.boundingBox.y) +
                                   ",\"width\":" + std::to_string(region.boundingBox.width) +
                                   ",\"height\":" + std::to_string(region.boundingBox.height) +
                                   ",\"object_count\":" + std::to_string(region.trackedObjectIds.size()) + "}";
                        if (i < consolidatedRegions.size() - 1) metadata += ",";
                    }
                    metadata += "]";
                } else {
                    metadata += ",\"consolidated_regions\":[]";
                }
                
                metadata += "}";
                
                // Try to save both original and processed frames to MongoDB using Python bindings
                std::string result = save_frames_to_mongodb(frame, mongoFrame, metadata);
                if (!result.empty()) {
                    std::cout << "💾 Frame saved to MongoDB with UUID: " << result << std::endl;
                    LOG_INFO("Frame saved to MongoDB: {}", result);
                } else {
                    std::cout << "❌ Failed to save frame to MongoDB" << std::endl;
                    LOG_ERROR("Failed to save frame to MongoDB");
                }
                
                } catch (const std::exception& e) {
                    std::cerr << "Error saving frame: " << e.what() << std::endl;
                    LOG_ERROR("Error saving frame to MongoDB: {}", e.what());
                }
            } else {
                // Frame skipped because no consolidated regions and save_only_consolidated_regions is enabled
                LOG_DEBUG("Frame {} skipped - no consolidated regions (save_only_consolidated_regions=true)", frameCount);
                std::cout << "⏭️  Frame skipped - no consolidated regions" << std::endl;
            }
            
            lastSaveTime = currentTime;
        }

        // Add status overlay
        std::string status = "Frame: " + std::to_string(frameCount) + 
                           " | Motions: " + std::to_string(processingResult.detectedBounds.size()) +
                           " | Regions: " + std::to_string(consolidatedRegions.size()) +
                           " | Save: " + (saveOnlyConsolidatedRegions ? "Consolidated Only" : "All Motion");
        cv::putText(displayFrame, status, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        
        // Add legend
        cv::putText(displayFrame, "Gray: Individual Motion | Red: Consolidated Regions", 
                   cv::Point(10, displayFrame.rows - 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);

        // Show the live feed
        cv::imshow("🐦 Birds of Play - Motion Detection", displayFrame);
        key = cv::waitKey(1);
        
        // Handle 's' key to save current frame
        if (key == 's' || key == 'S') {
            // Create frames directory if it doesn't exist
            fs::create_directories("frames");
            std::string saveFileName = "frames/saved_detection_frame_" + std::to_string(frameCount) + ".jpg";
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

