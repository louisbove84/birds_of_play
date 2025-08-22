#include "object_tracker.hpp"
#include "motion_tracker.hpp" // For TrackedObject struct
#include "logger.hpp"
#include <yaml-cpp/yaml.h>
#include <random>
#include <sstream>
#include <cmath>

ObjectTracker::ObjectTracker(const std::string& configPath)
    : nextObjectId(0),
      maxTrajectoryPoints(30),
      minTrajectoryLength(10),
      maxTrackingDistance(100.0),
      smoothingFactor(0.6),
      minTrackingConfidence(0.5),
      
      // Spatial Merging
      spatialMerging(true),
      spatialMergeDistance(50.0),
      spatialMergeOverlapThreshold(0.3),
      
      // Motion Clustering
      motionClustering(true),
      motionSimilarityThreshold(0.7),
      motionHistoryFrames(5),
      
      // Object Classification
      enableClassification(true),
      modelPath("models/squeezenet.onnx"),
      labelsPath("models/imagenet_labels.txt") {
    loadConfig(configPath);
}

// ============================================================================
// MAIN TRACKING PIPELINE
// ============================================================================

ObjectTracker::TrackingResult ObjectTracker::trackObjects(const std::vector<cv::Rect>& detectedBounds, const cv::Mat& currentFrame) {
    TrackingResult result;
    
    if (detectedBounds.empty()) {
        result.trackedObjects = trackedObjects;
        result.lostObjectIds = lostObjectIds;
        result.hasTrackedObjects = !trackedObjects.empty();
        return result;
    }
    
    std::vector<cv::Rect> processedBounds = detectedBounds;
    
    // Apply spatial merging if enabled
    if (spatialMerging) {
        processedBounds = mergeSpatialOverlaps(processedBounds);
    }
    
    // Apply motion clustering if enabled
    if (motionClustering) {
        processedBounds = clusterByMotion(processedBounds);
    }
    
    // Update motion history for clustering
    if (motionClustering) {
        previousBounds.push_back(processedBounds);
        if (previousBounds.size() > static_cast<size_t>(motionHistoryFrames)) {
            previousBounds.pop_front();
        }
    }
    
    // Update object trajectories
    updateTrajectories(processedBounds, currentFrame);
    
    // Log tracking results
    logTrackingResults();
    
    result.trackedObjects = trackedObjects;
    result.lostObjectIds = lostObjectIds;
    result.hasTrackedObjects = !trackedObjects.empty();
    
    return result;
}

// ============================================================================
// INDIVIDUAL TRACKING STEPS
// ============================================================================

std::vector<cv::Rect> ObjectTracker::mergeSpatialOverlaps(const std::vector<cv::Rect>& bounds) {
    if (bounds.empty()) return bounds;
    
    std::vector<cv::Rect> mergedBounds = bounds;
    bool merged = true;
    
    while (merged) {
        merged = false;
        std::vector<cv::Rect> newMergedBounds;
        std::vector<bool> used(mergedBounds.size(), false);
        
        for (size_t i = 0; i < mergedBounds.size(); ++i) {
            if (used[i]) continue;
            
            cv::Rect current = mergedBounds[i];
            used[i] = true;
            
            for (size_t j = i + 1; j < mergedBounds.size(); ++j) {
                if (used[j]) continue;
                
                cv::Rect other = mergedBounds[j];
                
                // Check if boxes should be merged based on distance or overlap
                double distance = calculateDistance(current, other);
                double overlap = calculateOverlapRatio(current, other);
                
                if (distance <= spatialMergeDistance || overlap >= spatialMergeOverlapThreshold) {
                    // Merge the rectangles
                    int x1 = std::min(current.x, other.x);
                    int y1 = std::min(current.y, other.y);
                    int x2 = std::max(current.x + current.width, other.x + other.width);
                    int y2 = std::max(current.y + current.height, other.y + other.height);
                    
                    current = cv::Rect(x1, y1, x2 - x1, y2 - y1);
                    used[j] = true;
                    merged = true;
                    
                    LOG_DEBUG("Merged bounding boxes: distance={}, overlap={}", distance, overlap);
                }
            }
            
            newMergedBounds.push_back(current);
        }
        
        mergedBounds = newMergedBounds;
    }
    
    LOG_DEBUG("Spatial merging: {} -> {} bounding boxes", bounds.size(), mergedBounds.size());
    return mergedBounds;
}

std::vector<cv::Rect> ObjectTracker::clusterByMotion(const std::vector<cv::Rect>& bounds) {
    if (bounds.empty() || previousBounds.empty()) return bounds;
    
    // Get the most recent previous bounds
    const std::vector<cv::Rect>& prevBounds = previousBounds.back();
    if (prevBounds.empty()) return bounds;
    
    std::vector<cv::Rect> clusteredBounds = bounds;
    std::vector<bool> used(bounds.size(), false);
    std::vector<cv::Rect> finalBounds;
    
    for (size_t i = 0; i < bounds.size(); ++i) {
        if (used[i]) continue;
        
        std::vector<size_t> cluster = {i};
        used[i] = true;
        
        // Find the closest previous bounding box for motion calculation
        cv::Rect closestPrev = findClosestPreviousRect(bounds[i], prevBounds);
        cv::Point currentMotion = calculateMotionVector(bounds[i], closestPrev);
        
        for (size_t j = i + 1; j < bounds.size(); ++j) {
            if (used[j]) continue;
            
            cv::Rect closestPrevJ = findClosestPreviousRect(bounds[j], prevBounds);
            cv::Point otherMotion = calculateMotionVector(bounds[j], closestPrevJ);
            double similarity = calculateCosineSimilarity(currentMotion, otherMotion);
            
            if (similarity >= motionSimilarityThreshold) {
                cluster.push_back(j);
                used[j] = true;
                LOG_DEBUG("Motion clustering: similarity={}", similarity);
            }
        }
        
        // Merge clustered bounding boxes
        if (cluster.size() > 1) {
            cv::Rect mergedRect = bounds[cluster[0]];
            for (size_t k = 1; k < cluster.size(); ++k) {
                cv::Rect other = bounds[cluster[k]];
                int x1 = std::min(mergedRect.x, other.x);
                int y1 = std::min(mergedRect.y, other.y);
                int x2 = std::max(mergedRect.x + mergedRect.width, other.x + other.width);
                int y2 = std::max(mergedRect.y + mergedRect.height, other.y + other.height);
                mergedRect = cv::Rect(x1, y1, x2 - x1, y2 - y1);
            }
            finalBounds.push_back(mergedRect);
        } else {
            finalBounds.push_back(bounds[cluster[0]]);
        }
    }
    
    LOG_DEBUG("Motion clustering: {} -> {} bounding boxes", bounds.size(), finalBounds.size());
    return finalBounds;
}

void ObjectTracker::updateTrajectories(std::vector<cv::Rect>& newBounds, const cv::Mat& currentFrame) {
    // Mark all current objects for potential removal
    std::vector<bool> objectMatched(trackedObjects.size(), false);
    
    // Try to match new bounds with existing objects
    for (const auto& bounds : newBounds) {
        TrackedObject* matchedObj = findNearestObject(bounds);
        
        if (matchedObj != nullptr) {
            // Update existing object
            size_t idx = matchedObj - &trackedObjects[0];
            if (idx < objectMatched.size()) {
                objectMatched[idx] = true;
                matchedObj->currentBounds = bounds;
                
                // Get new center position
                cv::Point newCenter = cv::Point(bounds.x + bounds.width/2, bounds.y + bounds.height/2);
                
                // Apply smoothing if we have a previous smoothed position
                if (!matchedObj->trajectory.empty()) {
                    matchedObj->smoothedCenter = smoothPosition(newCenter, matchedObj->smoothedCenter);
                    matchedObj->trajectory.push_back(matchedObj->smoothedCenter);
                } else {
                    matchedObj->smoothedCenter = newCenter;
                    matchedObj->trajectory.push_back(newCenter);
                }
                
                // Update confidence based on consistent motion
                if (matchedObj->trajectory.size() >= 2) {
                    cv::Point prevMotion = matchedObj->trajectory.back() - matchedObj->trajectory[matchedObj->trajectory.size()-2];
                    cv::Point currMotion = newCenter - matchedObj->trajectory.back();
                    double motionSimilarity = (prevMotion.x * currMotion.x + prevMotion.y * currMotion.y) /
                                            (sqrt(prevMotion.x * prevMotion.x + prevMotion.y * prevMotion.y + 1) *
                                             sqrt(currMotion.x * currMotion.x + currMotion.y * currMotion.y + 1));
                    matchedObj->confidence = 0.7 * matchedObj->confidence + 0.3 * (motionSimilarity + 1) / 2;
                } else {
                    matchedObj->confidence = 0.5;  // Initial confidence
                }
                
                // Limit trajectory length
                if (matchedObj->trajectory.size() > maxTrajectoryPoints) {
                    matchedObj->trajectory.pop_front();
                }
            }
        } else {
            // Create new tracked object
            std::string new_uuid = generateUUID();
            trackedObjects.emplace_back(nextObjectId++, bounds, new_uuid);
            
            // Classify the new object
            if (enableClassification) {
                ClassificationResult classification = classifyDetectedObject(currentFrame, bounds);
                trackedObjects.back().classLabel = classification.label;
                trackedObjects.back().classConfidence = classification.confidence;
                trackedObjects.back().classId = classification.class_id;
            }
        }
    }
    
    // Remove objects that weren't matched or have low confidence
    lostObjectIds.clear(); // Clear previous lost objects
    for (int i = trackedObjects.size() - 1; i >= 0; --i) {
        if (i < static_cast<int>(objectMatched.size()) && 
            (!objectMatched[i] || trackedObjects[i].confidence < minTrackingConfidence)) {
            // Add to lost objects list before removing
            lostObjectIds.push_back(trackedObjects[i].id);
            trackedObjects.erase(trackedObjects.begin() + i);
        }
    }
}

void ObjectTracker::logTrackingResults() {
    static int noObjectCount = 0;
    if (!trackedObjects.empty()) {
        std::vector<TrackedObject> filteredObjects;
        for (const auto& obj : trackedObjects) {
            LOG_DEBUG("Checking Object {}: trajectory.size()={}, minTrajectoryLength={}", 
                      obj.id, obj.trajectory.size(), minTrajectoryLength);
            if (obj.trajectory.size() >= minTrajectoryLength) {
                filteredObjects.push_back(obj);
            }
        }
        if (!filteredObjects.empty()) {
            LOG_INFO("Tracking {} objects (min trajectory length: {}):", filteredObjects.size(), minTrajectoryLength);
            for (const auto& obj : filteredObjects) {
                LOG_INFO("  Object {}: confidence={}, trajectory points={}, bounds=({},{},{},{})", 
                         obj.id, obj.confidence, obj.trajectory.size(), 
                         obj.currentBounds.x, obj.currentBounds.y, obj.currentBounds.width, obj.currentBounds.height);
            }
        }
    } else {
        noObjectCount++;
        if (noObjectCount % 30 == 0) {
            LOG_INFO("No objects currently being tracked.");
        }
    }
}

// ============================================================================
// OBJECT MANAGEMENT
// ============================================================================

TrackedObject* ObjectTracker::findNearestObject(const cv::Rect& bounds) {
    TrackedObject* nearest = nullptr;
    double minDistance = maxTrackingDistance;
    cv::Point center = cv::Point(bounds.x + bounds.width/2, bounds.y + bounds.height/2);

    for (auto& obj : trackedObjects) {
        double distance = cv::norm(center - obj.getCenter());
        
        if (distance < minDistance) {
            minDistance = distance;
            nearest = &obj;
        }
    }
    
    return nearest;
}

const TrackedObject* ObjectTracker::findTrackedObjectById(int id) const {
    for (const auto& obj : trackedObjects) {
        if (obj.id == id) {
            return &obj;
        }
    }
    return nullptr;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

cv::Point ObjectTracker::smoothPosition(const cv::Point& newPos, const cv::Point& smoothedPos) {
    return cv::Point(
        static_cast<int>(smoothedPos.x * smoothingFactor + newPos.x * (1 - smoothingFactor)),
        static_cast<int>(smoothedPos.y * smoothingFactor + newPos.y * (1 - smoothingFactor))
    );
}

std::string ObjectTracker::generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

ClassificationResult ObjectTracker::classifyDetectedObject(const cv::Mat& frame, const cv::Rect& bounds) {
    if (!enableClassification || !classifier.isModelLoaded()) {
        return ClassificationResult("unknown", 0.0f, -1);
    }
    
    try {
        // Ensure bounds are within frame
        cv::Rect safeBounds = bounds & cv::Rect(0, 0, frame.cols, frame.rows);
        if (safeBounds.width <= 0 || safeBounds.height <= 0) {
            return ClassificationResult("unknown", 0.0f, -1);
        }
        
        // Crop the object region
        cv::Mat croppedImage = frame(safeBounds);
        
        // Classify the cropped image
        ClassificationResult result = classifier.classifyObject(croppedImage);
        
        LOG_DEBUG("Object classification: {} (confidence: {:.2f})", result.label, result.confidence);
        return result;
        
    } catch (const cv::Exception& e) {
        LOG_ERROR("OpenCV error during object classification: {}", e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("Error during object classification: {}", e.what());
    }
    
    return ClassificationResult("unknown", 0.0f, -1);
}

// ============================================================================
// SPATIAL MERGING HELPERS
// ============================================================================

double ObjectTracker::calculateOverlapRatio(const cv::Rect& rect1, const cv::Rect& rect2) {
    // Calculate intersection
    int x1 = std::max(rect1.x, rect2.x);
    int y1 = std::max(rect1.y, rect2.y);
    int x2 = std::min(rect1.x + rect1.width, rect2.x + rect2.width);
    int y2 = std::min(rect1.y + rect1.height, rect2.y + rect2.height);
    
    if (x2 <= x1 || y2 <= y1) return 0.0; // No overlap
    
    int intersectionArea = (x2 - x1) * (y2 - y1);
    int unionArea = rect1.width * rect1.height + rect2.width * rect2.height - intersectionArea;
    
    return static_cast<double>(intersectionArea) / unionArea;
}

double ObjectTracker::calculateDistance(const cv::Rect& rect1, const cv::Rect& rect2) {
    cv::Point center1(rect1.x + rect1.width / 2, rect1.y + rect1.height / 2);
    cv::Point center2(rect2.x + rect2.width / 2, rect2.y + rect2.height / 2);
    
    return cv::norm(center1 - center2);
}

// ============================================================================
// MOTION CLUSTERING HELPERS
// ============================================================================

cv::Point ObjectTracker::calculateMotionVector(const cv::Rect& current, const cv::Rect& previous) {
    cv::Point currentCenter(current.x + current.width / 2, current.y + current.height / 2);
    cv::Point previousCenter(previous.x + previous.width / 2, previous.y + previous.height / 2);
    
    return currentCenter - previousCenter;
}

double ObjectTracker::calculateCosineSimilarity(const cv::Point& vec1, const cv::Point& vec2) {
    double dotProduct = vec1.x * vec2.x + vec1.y * vec2.y;
    double magnitude1 = std::sqrt(vec1.x * vec1.x + vec1.y * vec1.y);
    double magnitude2 = std::sqrt(vec2.x * vec2.x + vec2.y * vec2.y);
    
    if (magnitude1 == 0 || magnitude2 == 0) return 0.0;
    
    return dotProduct / (magnitude1 * magnitude2);
}

cv::Rect ObjectTracker::findClosestPreviousRect(const cv::Rect& current, const std::vector<cv::Rect>& previous) {
    if (previous.empty()) return current; // Return current if no previous bounds
    
    cv::Rect closest = previous[0];
    double minDistance = calculateDistance(current, closest);
    
    for (const auto& prev : previous) {
        double distance = calculateDistance(current, prev);
        if (distance < minDistance) {
            minDistance = distance;
            closest = prev;
        }
    }
    
    return closest;
}

// ============================================================================
// CONFIGURATION LOADING
// ============================================================================

void ObjectTracker::loadConfig(const std::string& configPath) {
    try {
        YAML::Node config = YAML::LoadFile(configPath);

        if (config["max_tracking_distance"]) maxTrackingDistance = config["max_tracking_distance"].as<double>();
        if (config["max_trajectory_points"]) maxTrajectoryPoints = config["max_trajectory_points"].as<size_t>();
        if (config["min_trajectory_length"]) minTrajectoryLength = config["min_trajectory_length"].as<size_t>();
        if (config["smoothing_factor"]) smoothingFactor = config["smoothing_factor"].as<double>();
        if (config["min_tracking_confidence"]) minTrackingConfidence = config["min_tracking_confidence"].as<double>();
        
        // Spatial Merging and Motion Clustering parameters
        if (config["spatial_merging"]) spatialMerging = config["spatial_merging"].as<bool>();
        if (config["spatial_merge_distance"]) spatialMergeDistance = config["spatial_merge_distance"].as<double>();
        if (config["spatial_merge_overlap_threshold"]) spatialMergeOverlapThreshold = config["spatial_merge_overlap_threshold"].as<double>();
        if (config["motion_clustering"]) motionClustering = config["motion_clustering"].as<bool>();
        if (config["motion_similarity_threshold"]) motionSimilarityThreshold = config["motion_similarity_threshold"].as<double>();
        if (config["motion_history_frames"]) motionHistoryFrames = config["motion_history_frames"].as<int>();
        
        // Object Classification parameters
        if (config["enable_classification"]) enableClassification = config["enable_classification"].as<bool>();
        if (config["model_path"]) modelPath = config["model_path"].as<std::string>();
        if (config["labels_path"]) labelsPath = config["labels_path"].as<std::string>();
        
        LOG_INFO("ObjectTracker config loaded: min_trajectory_length={}, spatial_merging={}, motion_clustering={}", 
                 minTrajectoryLength, spatialMerging, motionClustering);
        
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Warning: Could not load config file: {}. Error: {}", configPath, e.what());
    }
}
