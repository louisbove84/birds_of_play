#include "data_collector.hpp"
#include <yaml-cpp/yaml.h>
#include <iomanip>
#include <sstream>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/exception/operation_exception.hpp>

using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::array;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;

DataCollector::DataCollector(const std::string& config_path) 
    : instance{}, // Initialize mongocxx instance
      client{mongocxx::uri{}} // Default constructor, will be updated in initialize()
{
    // Load configuration
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        enabled = config["enable_data_collection"].as<bool>();
        shouldCleanupOldData = config["cleanup_old_data"].as<bool>();
        mongoUri = config["mongodb_uri"].as<std::string>();
        dbName = config["database_name"].as<std::string>();
        std::string prefix = config["collection_prefix"].as<std::string>();
        collectionName = prefix;
        imageFormat = config["image_format"].as<std::string>();
        saveIntervalSeconds = config["save_interval_seconds"].as<int>();
        minTrackingConfidence = config["min_tracking_confidence"].as<double>();
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not load data collection config: " << e.what() << std::endl;
        enabled = false;
        shouldCleanupOldData = true;
        minTrackingConfidence = 0.5;
        mongoUri = "mongodb://localhost:27017";
        dbName = "birds_of_play";
        collectionName = "motion_tracking";
    }
}

DataCollector::~DataCollector() {
    if (enabled) {
        saveData(); // Save any remaining data before shutting down
    }
}

bool DataCollector::initialize() {
    if (!enabled) return false;
    
    try {
        // Connect to MongoDB
        client = mongocxx::client{mongocxx::uri{mongoUri}};
        db = client[dbName];
        tracking_collection = db[collectionName + "_data"];
        images_collection = db[collectionName + "_images"];
        
        // Clean up old data if configured
        if (shouldCleanupOldData) {
            cleanupOldData();
        }
        
        lastSaveTime = std::chrono::system_clock::now();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing MongoDB connection: " << e.what() << std::endl;
        return false;
    }
}

void DataCollector::addTrackingData(int object_id, const cv::Mat& frame, const cv::Rect& bounds,
                                  const cv::Point& position, double confidence) {
    if (!enabled || confidence < minTrackingConfidence) return;
    
    auto now = std::chrono::system_clock::now();
    
    // Create new tracking data if this is a new object
    if (trackingData.find(object_id) == trackingData.end()) {
        TrackingData data;
        data.uuid = generateUUID();
        data.first_seen = now;
        data.initial_bounds = bounds;
        
        // Save cropped image
        cv::Mat cropped = frame(bounds).clone();
        data.first_image = cropped;
        
        trackingData[object_id] = data;
    }
    
    // Update existing tracking data
    auto& data = trackingData[object_id];
    data.last_seen = now;
    data.trajectory.push_back(position);
    data.confidence = confidence;
    
    // Check if it's time to save data
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastSaveTime).count();
    if (elapsed >= saveIntervalSeconds) {
        saveData();
        lastSaveTime = now;
    }
}

void DataCollector::saveData() {
    if (!enabled || trackingData.empty()) return;
    
    try {
        // Save each tracked object that meets confidence threshold
        for (const auto& pair : trackingData) {
            if (pair.second.confidence >= minTrackingConfidence) {
                saveTrackingData(pair.second);
            }
        }
    } catch (const mongocxx::operation_exception& e) {
        std::cerr << "MongoDB operation error: " << e.what() << std::endl;
    }
}

std::string DataCollector::generateUUID() {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

void DataCollector::cleanupOldData() {
    try {
        tracking_collection.drop();
        images_collection.drop();
    } catch (const mongocxx::operation_exception& e) {
        std::cerr << "Error cleaning up old data: " << e.what() << std::endl;
    }
}

bsoncxx::document::value DataCollector::createTrackingDocument(const TrackingData& data) {
    // Create trajectory array
    auto trajectory = array{};
    for (const auto& point : data.trajectory) {
        trajectory.append(make_document(
            kvp("x", point.x),
            kvp("y", point.y)
        ));
    }
    
    // Create main document
    auto doc = make_document(
        kvp("uuid", data.uuid),
        kvp("first_seen", bsoncxx::types::b_date(data.first_seen)),
        kvp("last_seen", bsoncxx::types::b_date(data.last_seen)),
        kvp("initial_bounds", make_document(
            kvp("x", data.initial_bounds.x),
            kvp("y", data.initial_bounds.y),
            kvp("width", data.initial_bounds.width),
            kvp("height", data.initial_bounds.height)
        )),
        kvp("confidence", data.confidence),
        kvp("trajectory", trajectory.view())
    );
    
    return doc;
}

bsoncxx::document::value DataCollector::createImageDocument(const std::string& uuid, const cv::Mat& image) {
    std::vector<uint8_t> buffer = matToVector(image);
    
    auto doc = make_document(
        kvp("uuid", uuid),
        kvp("image", bsoncxx::types::b_binary{
            bsoncxx::binary_sub_type::k_binary,
            static_cast<uint32_t>(buffer.size()),
            buffer.data()
        })
    );
    
    return doc;
}

void DataCollector::saveTrackingData(const TrackingData& data) {
    if (data.confidence < minTrackingConfidence) return;
    
    try {
        // Save tracking data
        auto tracking_doc = createTrackingDocument(data);
        tracking_collection.insert_one(tracking_doc.view());
        
        // Save image data
        auto image_doc = createImageDocument(data.uuid, data.first_image);
        images_collection.insert_one(image_doc.view());
    } catch (const mongocxx::operation_exception& e) {
        std::cerr << "Error saving data to MongoDB: " << e.what() << std::endl;
    }
}

std::string DataCollector::getTimestampStr(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::vector<uint8_t> DataCollector::matToVector(const cv::Mat& image) {
    std::vector<uint8_t> buffer;
    if (image.empty()) return buffer;
    
    std::vector<uint8_t> encoded;
    cv::imencode("." + imageFormat, image, encoded);
    return encoded;
} 