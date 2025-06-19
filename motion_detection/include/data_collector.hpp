#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <uuid/uuid.h>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>

struct TrackingData {
    std::string uuid;
    std::chrono::system_clock::time_point first_seen;
    std::chrono::system_clock::time_point last_seen;
    cv::Mat first_image;
    std::vector<cv::Point> trajectory;
    cv::Rect initial_bounds;
    double confidence;
};

class DataCollector {
public:
    DataCollector(const std::string& config_path);
    ~DataCollector();

    // Initialize data collection
    bool initialize();
    
    // Add new tracking data
    void addTrackingData(int object_id, const cv::Mat& frame, const cv::Rect& bounds, 
                        const cv::Point& position, double confidence);
    
    // Handle objects that are no longer being tracked
    void handleObjectLost(int object_id);
    
    // Save current data to MongoDB
    void saveData();

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