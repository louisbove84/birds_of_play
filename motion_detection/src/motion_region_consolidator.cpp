#include "motion_region_consolidator.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

MotionRegionConsolidator::MotionRegionConsolidator(const ConsolidationConfig& config)
    : config_(config), frameCounter_(0) {
    LOG_INFO("MotionRegionConsolidator initialized with config");
}

std::vector<ConsolidatedRegion> MotionRegionConsolidator::consolidateRegions(
    const std::vector<TrackedObject>& trackedObjects) {
    
    frameCounter_++;
    
    if (trackedObjects.empty()) {
        removeStaleRegions();
        return consolidatedRegions_;
    }
    
    LOG_DEBUG("Consolidating {} tracked objects", trackedObjects.size());
    
    // Step 1: Group objects by spatial proximity
    auto proximityGroups = groupObjectsByProximity(trackedObjects);
    
    // Step 2: Further refine groups by motion similarity
    std::vector<std::vector<int>> finalGroups;
    for (const auto& group : proximityGroups) {
        if (group.size() >= static_cast<size_t>(config_.minObjectsPerRegion)) {
            auto motionGroups = groupObjectsByMotion(trackedObjects);
            // Intersect proximity and motion groups
            for (const auto& motionGroup : motionGroups) {
                std::vector<int> intersection;
                std::set_intersection(group.begin(), group.end(),
                                    motionGroup.begin(), motionGroup.end(),
                                    std::back_inserter(intersection));
                if (intersection.size() >= static_cast<size_t>(config_.minObjectsPerRegion)) {
                    finalGroups.push_back(intersection);
                }
            }
        }
    }
    
    // Step 3: Create consolidated regions from groups
    auto newRegions = createConsolidatedRegions(trackedObjects, finalGroups);
    
    // Step 4: Update existing regions and merge if necessary
    updateExistingRegions(trackedObjects);
    
    // Step 5: Merge overlapping regions
    for (const auto& newRegion : newRegions) {
        bool merged = false;
        for (auto& existingRegion : consolidatedRegions_) {
            if (areRegionsOverlapping(newRegion, existingRegion)) {
                existingRegion = mergeRegions(existingRegion, newRegion);
                merged = true;
                break;
            }
        }
        if (!merged) {
            consolidatedRegions_.push_back(newRegion);
        }
    }
    
    // Step 6: Remove stale regions
    removeStaleRegions();
    
    LOG_DEBUG("Created {} consolidated regions", consolidatedRegions_.size());
    return consolidatedRegions_;
}

std::vector<std::vector<int>> MotionRegionConsolidator::groupObjectsByProximity(
    const std::vector<TrackedObject>& objects) {
    
    std::vector<std::vector<int>> groups;
    std::vector<bool> assigned(objects.size(), false);
    
    for (size_t i = 0; i < objects.size(); ++i) {
        if (assigned[i]) continue;
        
        std::vector<int> group = {static_cast<int>(i)};
        assigned[i] = true;
        
        // Find all objects within distance threshold
        for (size_t j = i + 1; j < objects.size(); ++j) {
            if (assigned[j]) continue;
            
            double distance = calculateSpatialDistance(objects[i], objects[j]);
            if (distance <= config_.maxDistanceThreshold) {
                group.push_back(static_cast<int>(j));
                assigned[j] = true;
            }
        }
        
        if (group.size() >= static_cast<size_t>(config_.minObjectsPerRegion)) {
            groups.push_back(group);
        }
    }
    
    return groups;
}

std::vector<std::vector<int>> MotionRegionConsolidator::groupObjectsByMotion(
    const std::vector<TrackedObject>& objects) {
    
    std::vector<std::vector<int>> groups;
    std::vector<bool> assigned(objects.size(), false);
    
    for (size_t i = 0; i < objects.size(); ++i) {
        if (assigned[i]) continue;
        
        std::vector<int> group = {static_cast<int>(i)};
        assigned[i] = true;
        
        // Find all objects with similar motion
        for (size_t j = i + 1; j < objects.size(); ++j) {
            if (assigned[j]) continue;
            
            double similarity = calculateMotionSimilarity(objects[i], objects[j]);
            if (similarity > 0.7) { // High similarity threshold
                group.push_back(static_cast<int>(j));
                assigned[j] = true;
            }
        }
        
        groups.push_back(group);
    }
    
    return groups;
}

std::vector<ConsolidatedRegion> MotionRegionConsolidator::createConsolidatedRegions(
    const std::vector<TrackedObject>& objects,
    const std::vector<std::vector<int>>& groups) {
    
    std::vector<ConsolidatedRegion> regions;
    
    for (const auto& group : groups) {
        if (group.empty()) continue;
        
        cv::Rect bbox = calculateBoundingBox(objects, group);
        cv::Point2f avgVelocity = calculateAverageVelocity(objects, group);
        double confidence = calculateRegionConfidence(objects, group);
        
        // Expand bounding box for better YOLO detection
        bbox = expandBoundingBox(bbox, config_.regionExpansionFactor, cv::Size(1920, 1080));
        
        // Ensure minimum size for YOLO
        if (bbox.width < config_.minRegionWidth) {
            int expand = (config_.minRegionWidth - bbox.width) / 2;
            bbox.x -= expand;
            bbox.width = config_.minRegionWidth;
        }
        if (bbox.height < config_.minRegionHeight) {
            int expand = (config_.minRegionHeight - bbox.height) / 2;
            bbox.y -= expand;
            bbox.height = config_.minRegionHeight;
        }
        
        // Check area constraints
        double area = bbox.width * bbox.height;
        if (area >= config_.minRegionArea && area <= config_.maxRegionArea) {
            regions.emplace_back(bbox, group, avgVelocity, confidence);
            LOG_DEBUG("Created consolidated region: {}x{} at ({},{}) with {} objects",
                     bbox.width, bbox.height, bbox.x, bbox.y, group.size());
        }
    }
    
    return regions;
}

void MotionRegionConsolidator::updateExistingRegions(const std::vector<TrackedObject>& objects) {
    for (auto& region : consolidatedRegions_) {
        region.framesSinceLastUpdate++;
        
        // Try to update region with current objects
        std::vector<int> updatedIds;
        for (int id : region.trackedObjectIds) {
            auto it = std::find_if(objects.begin(), objects.end(),
                                 [id](const TrackedObject& obj) { return obj.id == id; });
            if (it != objects.end()) {
                updatedIds.push_back(id);
            }
        }
        
        if (!updatedIds.empty()) {
            region.trackedObjectIds = updatedIds;
            region.framesSinceLastUpdate = 0;
            
            // Recalculate bounding box and velocity
            region.boundingBox = calculateBoundingBox(objects, updatedIds);
            region.averageVelocity = calculateAverageVelocity(objects, updatedIds);
            region.confidence = calculateRegionConfidence(objects, updatedIds);
        }
    }
}

void MotionRegionConsolidator::removeStaleRegions() {
    consolidatedRegions_.erase(
        std::remove_if(consolidatedRegions_.begin(), consolidatedRegions_.end(),
                      [this](const ConsolidatedRegion& region) {
                          return region.framesSinceLastUpdate > config_.maxFramesWithoutUpdate;
                      }),
        consolidatedRegions_.end());
}

ConsolidatedRegion MotionRegionConsolidator::mergeRegions(
    const ConsolidatedRegion& region1, const ConsolidatedRegion& region2) {
    
    // Merge bounding boxes
    cv::Rect mergedBox = region1.boundingBox | region2.boundingBox;
    
    // Merge object IDs
    std::vector<int> mergedIds = region1.trackedObjectIds;
    mergedIds.insert(mergedIds.end(), region2.trackedObjectIds.begin(), 
                    region2.trackedObjectIds.end());
    
    // Average velocities weighted by confidence
    double totalConfidence = region1.confidence + region2.confidence;
    cv::Point2f mergedVelocity;
    if (totalConfidence > 0) {
        mergedVelocity.x = (region1.averageVelocity.x * region1.confidence + 
                           region2.averageVelocity.x * region2.confidence) / totalConfidence;
        mergedVelocity.y = (region1.averageVelocity.y * region1.confidence + 
                           region2.averageVelocity.y * region2.confidence) / totalConfidence;
    }
    
    double mergedConfidence = totalConfidence / 2.0;
    
    ConsolidatedRegion merged(mergedBox, mergedIds, mergedVelocity, mergedConfidence);
    merged.framesSinceLastUpdate = std::min(region1.framesSinceLastUpdate, 
                                           region2.framesSinceLastUpdate);
    
    return merged;
}

double MotionRegionConsolidator::calculateSpatialDistance(
    const TrackedObject& obj1, const TrackedObject& obj2) const {
    
    cv::Point center1 = obj1.getCenter();
    cv::Point center2 = obj2.getCenter();
    
    double dx = center1.x - center2.x;
    double dy = center1.y - center2.y;
    
    return std::sqrt(dx * dx + dy * dy);
}

double MotionRegionConsolidator::calculateMotionSimilarity(
    const TrackedObject& obj1, const TrackedObject& obj2) const {
    
    if (obj1.trajectory.size() < 2 || obj2.trajectory.size() < 2) {
        return 0.0; // No motion data available
    }
    
    cv::Point2f vel1 = calculateVelocity(obj1);
    cv::Point2f vel2 = calculateVelocity(obj2);
    
    // Calculate magnitude similarity
    double mag1 = std::sqrt(vel1.x * vel1.x + vel1.y * vel1.y);
    double mag2 = std::sqrt(vel2.x * vel2.x + vel2.y * vel2.y);
    
    if (mag1 == 0.0 || mag2 == 0.0) {
        return (mag1 == 0.0 && mag2 == 0.0) ? 1.0 : 0.0;
    }
    
    double magRatio = std::min(mag1, mag2) / std::max(mag1, mag2);
    
    // Calculate angle similarity
    double dot = vel1.x * vel2.x + vel1.y * vel2.y;
    double angle = std::acos(std::clamp(dot / (mag1 * mag2), -1.0, 1.0));
    double angleDegrees = angle * 180.0 / M_PI;
    
    double angleSimilarity = 1.0 - (angleDegrees / 180.0);
    
    // Combine similarities
    return (magRatio + angleSimilarity) / 2.0;
}

bool MotionRegionConsolidator::areRegionsOverlapping(
    const ConsolidatedRegion& region1, const ConsolidatedRegion& region2) const {
    
    cv::Rect intersection = region1.boundingBox & region2.boundingBox;
    if (intersection.area() == 0) {
        return false;
    }
    
    double overlapRatio1 = static_cast<double>(intersection.area()) / region1.boundingBox.area();
    double overlapRatio2 = static_cast<double>(intersection.area()) / region2.boundingBox.area();
    
    return std::max(overlapRatio1, overlapRatio2) >= config_.overlapThreshold;
}

cv::Rect MotionRegionConsolidator::calculateBoundingBox(
    const std::vector<TrackedObject>& objects, const std::vector<int>& indices) const {
    
    if (indices.empty()) {
        return cv::Rect();
    }
    
    cv::Rect combinedBox = objects[indices[0]].currentBounds;
    for (size_t i = 1; i < indices.size(); ++i) {
        combinedBox |= objects[indices[i]].currentBounds;
    }
    
    return combinedBox;
}

cv::Point2f MotionRegionConsolidator::calculateAverageVelocity(
    const std::vector<TrackedObject>& objects, const std::vector<int>& indices) const {
    
    if (indices.empty()) {
        return cv::Point2f(0, 0);
    }
    
    cv::Point2f totalVelocity(0, 0);
    int validObjects = 0;
    
    for (int idx : indices) {
        cv::Point2f velocity = calculateVelocity(objects[idx]);
        if (velocity.x != 0.0f || velocity.y != 0.0f) {
            totalVelocity += velocity;
            validObjects++;
        }
    }
    
    if (validObjects > 0) {
        return cv::Point2f(totalVelocity.x / validObjects, totalVelocity.y / validObjects);
    }
    
    return cv::Point2f(0, 0);
}

double MotionRegionConsolidator::calculateRegionConfidence(
    const std::vector<TrackedObject>& objects, const std::vector<int>& indices) const {
    
    if (indices.empty()) {
        return 0.0;
    }
    
    double totalConfidence = 0.0;
    for (int idx : indices) {
        totalConfidence += objects[idx].confidence;
    }
    
    return totalConfidence / indices.size();
}

cv::Rect MotionRegionConsolidator::expandBoundingBox(
    const cv::Rect& bbox, double expansionFactor, const cv::Size& frameSize) {
    
    int expandX = static_cast<int>((bbox.width * (expansionFactor - 1.0)) / 2.0);
    int expandY = static_cast<int>((bbox.height * (expansionFactor - 1.0)) / 2.0);
    
    cv::Rect expanded(
        std::max(0, bbox.x - expandX),
        std::max(0, bbox.y - expandY),
        std::min(frameSize.width - std::max(0, bbox.x - expandX), bbox.width + 2 * expandX),
        std::min(frameSize.height - std::max(0, bbox.y - expandY), bbox.height + 2 * expandY)
    );
    
    return expanded;
}

cv::Point2f MotionRegionConsolidator::calculateVelocity(const TrackedObject& obj) {
    if (obj.trajectory.size() < 2) {
        return cv::Point2f(0, 0);
    }
    
    // Use last few points for velocity calculation
    size_t numPoints = std::min(obj.trajectory.size(), static_cast<size_t>(5));
    cv::Point2f velocity(0, 0);
    
    for (size_t i = obj.trajectory.size() - numPoints; i < obj.trajectory.size() - 1; ++i) {
        velocity.x += obj.trajectory[i + 1].x - obj.trajectory[i].x;
        velocity.y += obj.trajectory[i + 1].y - obj.trajectory[i].y;
    }
    
    velocity.x /= (numPoints - 1);
    velocity.y /= (numPoints - 1);
    
    return velocity;
}

void MotionRegionConsolidator::updateConfig(const ConsolidationConfig& config) {
    config_ = config;
    LOG_INFO("MotionRegionConsolidator config updated");
}
