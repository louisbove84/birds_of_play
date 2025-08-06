#pragma once

#include <opencv2/opencv.hpp>           // cv::Mat, cv::imencode
#include <string>                       // std::string
#include <vector>                       // std::vector
#include <unordered_map>                // std::unordered_map
#include <filesystem>                   // std::filesystem
#include <chrono>                       // std::chrono::system_clock
#include <uuid/uuid.h>                  // uuid_t, uuid_generate, uuid_unparse_lower
#include <yaml-cpp/yaml.h>              // YAML::Node, YAML::LoadFile
#include "motion_tracker.hpp"           // TrackedObject struct
#include <bsoncxx/v_noabi/bsoncxx/builder/basic/document.hpp>  // bsoncxx::builder::basic::document
#include <bsoncxx/v_noabi/bsoncxx/types.hpp>            // bsoncxx::types::b_date, bsoncxx::types::b_binary
#include <mongocxx/v_noabi/mongocxx/client.hpp>          // mongocxx::client
#include <mongocxx/v_noabi/mongocxx/instance.hpp>        // mongocxx::instance
#include <mongocxx/v_noabi/mongocxx/uri.hpp>             // mongocxx::uri
#include <mongocxx/v_noabi/mongocxx/database.hpp>        // mongocxx::database
#include <mongocxx/v_noabi/mongocxx/collection.hpp>      // mongocxx::collection

// Forward-declare TrackedObject to break the circular dependency
struct TrackedObject;

struct TrackingData {
    std::string uuid;
    std::chrono::system_clock::time_point first_seen;
    std::chrono::system_clock::time_point last_seen;
    cv::Mat first_image;
    std::vector<cv::Point> trajectory;
    cv::Rect initial_bounds;
    double confidence;
    
    // Classification data
    std::string classLabel;
    float classConfidence;
    int classId;
};

class DataCollector {
public:
    DataCollector(const std::string& config_path);
    ~DataCollector();

    // Initialize data collection
    bool initialize();
    
    // Add new tracking data
    void addTrackingData(int object_id, const cv::Mat& frame, const cv::Rect& bounds, 
                        const cv::Point& position, double confidence,
                        const std::string& classLabel = "unknown", 
                        float classConfidence = 0.0f, 
                        int classId = -1);
    
    // Handle objects that are no longer being tracked
    void handleObjectLost(int object_id);
    
    // Save current data to MongoDB
    void saveData();

    void addLostObject(const TrackedObject& object);

private:
    // Configuration
    bool enabled;
    bool shouldCleanupOldData;
    std::string mongoUri;
    std::string dbName;
    std::string collectionName;
    std::string imageFormat;
    int saveIntervalSeconds;
    double minTrackingConfidence;
    
    // MongoDB connection
    mongocxx::instance instance;  // Required for driver initialization
    mongocxx::client client;
    mongocxx::database db;
    mongocxx::collection tracking_collection;
    mongocxx::collection images_collection;
    
    // Data storage
    std::unordered_map<int, TrackingData> trackingData;
    std::chrono::system_clock::time_point lastSaveTime;
    
    // Helper methods
    std::string generateUUID();
    void cleanupOldData();
    bsoncxx::document::value createTrackingDocument(const TrackingData& data);
    bsoncxx::document::value createImageDocument(const std::string& uuid, const cv::Mat& image);
    void saveTrackingData(const TrackingData& data);
    std::string getTimestampStr(const std::chrono::system_clock::time_point& tp);
    std::vector<uint8_t> matToVector(const cv::Mat& image);
}; 