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

    // Initialize motion processor and motion region consolidator
    MotionProcessor motionProcessor(config_path.string());
    motionProcessor.setVisualizationPath(""); // Disable visualization file saving
    MotionRegionConsolidator regionConsolidator;
    
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

        // Auto-save frame every 1 second
        if (currentTime - lastSaveTime >= saveInterval) {
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
                
                // Try to save directly to MongoDB using Python bindings
                std::string result = save_frame_to_mongodb(displayFrame, metadata);
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
            
            lastSaveTime = currentTime;
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

// Function to save frame to MongoDB (implementation)
std::string save_frame_to_mongodb(const cv::Mat& frame, const std::string& metadata_json) {
    try {
        // Import Python modules
        py::module sys = py::module::import("sys");
        py::module os = py::module::import("os");
        
        // Add the src directory to Python path
        std::string current_dir = os.attr("getcwd")().cast<std::string>();
        sys.attr("path").attr("insert")(0, current_dir + "/src");
        
        // Add virtual environment site-packages to Python path
        std::string venv_site_packages = current_dir + "/venv/lib/python3.13/site-packages";
        sys.attr("path").attr("insert")(0, venv_site_packages);
        
        // Import our MongoDB modules
        py::module db_manager_module = py::module::import("mongodb.database_manager");
        py::module frame_db_module = py::module::import("mongodb.frame_database");
        
        // Get the classes
        py::object DatabaseManager = db_manager_module.attr("DatabaseManager");
        py::object FrameDatabase = frame_db_module.attr("FrameDatabase");
        
        // Create database manager and connect
        py::object db_manager = DatabaseManager();
        db_manager.attr("connect")();
        
        // Create frame database
        py::object frame_db = FrameDatabase(db_manager);
        
        // Convert metadata JSON to Python dict
        py::module json = py::module::import("json");
        py::object metadata = json.attr("loads")(metadata_json);
        
        // Convert cv::Mat to numpy array for Python
        // Create a copy of the frame data
        cv::Mat frame_copy = frame.clone();
        
        // Convert BGR to RGB if needed
        cv::cvtColor(frame_copy, frame_copy, cv::COLOR_BGR2RGB);
        
        // Create numpy array from cv::Mat
        py::array_t<unsigned char> numpy_frame(
            {frame_copy.rows, frame_copy.cols, frame_copy.channels()},
            frame_copy.data
        );
        
        // Save frame
        py::object result = frame_db.attr("save_frame")(numpy_frame, metadata);
        
        // Disconnect
        db_manager.attr("disconnect")();
        
        return result.cast<std::string>();
        
    } catch (const py::error_already_set& e) {
        std::cerr << "Python error: " << e.what() << std::endl;
        return "";
    } catch (const std::exception& e) {
        std::cerr << "C++ error: " << e.what() << std::endl;
        return "";
    }
}
