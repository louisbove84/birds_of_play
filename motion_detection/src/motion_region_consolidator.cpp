#include "motion_region_consolidator.hpp"
#include "logger.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

MotionRegionConsolidator::MotionRegionConsolidator(const ConsolidationConfig& config)
    : config_(config), frameCounter_(0) {
    LOG_INFO("MotionRegionConsolidator initialized with config");
}

std::vector<std::vector<int>> MotionRegionConsolidator::groupByProximityPairwise(
    const std::vector<TrackedObject>& objects) {
    std::vector<std::vector<int>> groups;
    const size_t n = objects.size();
    std::vector<bool> assigned(n, false);

    for (size_t i = 0; i < n; ++i) {
        if (assigned[i]) continue;
        
        std::vector<int> group = {static_cast<int>(i)};
        assigned[i] = true;
        
        for (size_t j = i + 1; j < n; ++j) {
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
    
    LOG_DEBUG("Used pairwise proximity grouping (O(n²)) for {} objects, created {} groups", n, groups.size());
    return groups;
}

std::vector<std::vector<int>> MotionRegionConsolidator::groupByProximityGrid(
    const std::vector<TrackedObject>& objects) {
    std::vector<std::vector<int>> groups;
    const size_t n = objects.size();
    std::map<std::pair<int, int>, std::vector<int>> grid;
    double cellSize = config_.gridCellSize;
    
    // Assign objects to grid cells
    for (size_t i = 0; i < n; ++i) {
        cv::Point center = objects[i].getCenter();
        int row = static_cast<int>(center.y / cellSize);
        int col = static_cast<int>(center.x / cellSize);
        grid[{row, col}].push_back(static_cast<int>(i));
    }
    
    std::vector<bool> assigned(n, false);
    
    // Process each cell and its neighbors
    for (const auto& cell : grid) {
        const auto& cellIndices = cell.second;
        for (int i : cellIndices) {
            if (assigned[i]) continue;
            
            std::vector<int> group = {i};
            assigned[i] = true;
            
            // Check objects in the same and neighboring cells
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    auto neighborCell = grid.find({cell.first.first + dr, cell.first.second + dc});
                    if (neighborCell == grid.end()) continue;
                    
                    for (int j : neighborCell->second) {
                        if (assigned[j] || i == j) continue;
                        
                        double distance = calculateSpatialDistance(objects[i], objects[j]);
                        if (distance <= config_.maxDistanceThreshold) {
                            group.push_back(j);
                            assigned[j] = true;
                        }
                    }
                }
            }
            
            if (group.size() >= static_cast<size_t>(config_.minObjectsPerRegion)) {
                groups.push_back(group);
            }
        }
    }
    
    LOG_DEBUG("Used grid-based proximity grouping for {} objects, created {} groups with cell size {}", 
              n, groups.size(), cellSize);
    return groups;
}

std::vector<std::vector<int>> MotionRegionConsolidator::groupObjectsByProximity(
    const std::vector<TrackedObject>& objects) {
    const size_t n = objects.size();
    
    // Use pairwise O(n²) for small n (<50) to avoid grid overhead
    if (n < 50) {
        return groupByProximityPairwise(objects);
    }
    // Use grid-based approach for larger n
    return groupByProximityGrid(objects);
}

std::vector<ConsolidatedRegion> MotionRegionConsolidator::consolidateRegions(
    const std::vector<TrackedObject>& trackedObjects) {
    
    frameCounter_++;
    
    if (trackedObjects.empty()) {
        removeStaleRegions();
        return consolidatedRegions_;
    }
    
    LOG_DEBUG("Consolidating {} tracked objects", trackedObjects.size());
    
    // Step 1: Group objects by spatial proximity only
    auto proximityGroups = groupObjectsByProximity(trackedObjects);
    
    // Step 2: Use proximity groups directly
    std::vector<std::vector<int>> finalGroups;
    for (const auto& group : proximityGroups) {
        if (group.size() >= static_cast<size_t>(config_.minObjectsPerRegion)) {
            finalGroups.push_back(group);
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

std::vector<ConsolidatedRegion> MotionRegionConsolidator::createConsolidatedRegions(
    const std::vector<TrackedObject>& objects,
    const std::vector<std::vector<int>>& groups) {
    
    std::vector<ConsolidatedRegion> regions;
    
    for (const auto& group : groups) {
        if (group.empty()) continue;
        
        cv::Rect bbox = calculateBoundingBox(objects, group);
        
        // Expand bounding box for better YOLO detection
        bbox = expandBoundingBox(bbox, config_.regionExpansionFactor, config_.frameSize);
        
        // Adjust to ideal model region size, preserving aspect ratio
        double aspectRatio = static_cast<double>(bbox.width) / bbox.height;
        int idealSize = config_.idealModelRegionSize;
        int newWidth, newHeight;
        if (aspectRatio >= 1.0) {
            newWidth = idealSize;
            newHeight = static_cast<int>(idealSize / aspectRatio);
        } else {
            newHeight = idealSize;
            newWidth = static_cast<int>(idealSize * aspectRatio);
        }
        
        // Center the adjusted box
        int expandX = (newWidth - bbox.width) / 2;
        int expandY = (newHeight - bbox.height) / 2;
        bbox.x -= expandX;
        bbox.y -= expandY;
        bbox.width = newWidth;
        bbox.height = newHeight;
        
        // Clamp to frame boundaries
        bbox.x = std::max(0, std::min(bbox.x, config_.frameSize.width - bbox.width));
        bbox.y = std::max(0, std::min(bbox.y, config_.frameSize.height - bbox.height));
        
        // Check area constraints
        double area = bbox.width * bbox.height;
        if (area >= config_.minRegionArea && area <= config_.maxRegionArea) {
            regions.emplace_back(bbox, group);
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
            
            // Recalculate bounding box
            region.boundingBox = calculateBoundingBox(objects, updatedIds);
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
    
    ConsolidatedRegion merged(mergedBox, mergedIds);
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

void MotionRegionConsolidator::updateConfig(const ConsolidationConfig& config) {
    config_ = config;
    LOG_INFO("MotionRegionConsolidator config updated");
}

// ============================================================================
// VISUALIZATION METHODS
// ============================================================================

std::vector<ConsolidatedRegion> MotionRegionConsolidator::consolidateRegionsWithVisualization(
    const std::vector<TrackedObject>& trackedObjects,
    const cv::Mat& inputImage,
    const std::string& outputImagePath) {
    
    // Perform normal consolidation
    std::vector<ConsolidatedRegion> regions = consolidateRegions(trackedObjects);
    
    // Create visualization if input image is provided
    if (!inputImage.empty()) {
        cv::Mat visualization = createVisualization(trackedObjects, regions, inputImage);
        
        // Save visualization if output path is provided
        if (!outputImagePath.empty()) {
            if (cv::imwrite(outputImagePath, visualization)) {
                LOG_INFO("Saved consolidation visualization to: {}", outputImagePath);
            } else {
                LOG_ERROR("Failed to save visualization to: {}", outputImagePath);
            }
        }
    }
    
    return regions;
}

std::vector<ConsolidatedRegion> MotionRegionConsolidator::consolidateRegionsStandalone(
    const std::vector<TrackedObject>& trackedObjects,
    const std::string& outputImagePath) {
    
    // Perform normal consolidation
    std::vector<ConsolidatedRegion> regions = consolidateRegions(trackedObjects);
    
    // Create a synthetic visualization image if no input image provided
    cv::Mat syntheticImage = cv::Mat::zeros(cv::Size(1920, 1080), CV_8UC3);
    
    // Draw a background grid to show the frame
    for (int i = 0; i < syntheticImage.cols; i += 100) {
        cv::line(syntheticImage, cv::Point(i, 0), cv::Point(i, syntheticImage.rows), 
                cv::Scalar(50, 50, 50), 1);
    }
    for (int i = 0; i < syntheticImage.rows; i += 100) {
        cv::line(syntheticImage, cv::Point(0, i), cv::Point(syntheticImage.cols, i), 
                cv::Scalar(50, 50, 50), 1);
    }
    
    // Create visualization with synthetic image
    cv::Mat visualization = createVisualization(trackedObjects, regions, syntheticImage);
    
    // Save visualization if output path is provided
    if (!outputImagePath.empty()) {
        // Ensure directory exists
        fs::path path(outputImagePath);
        fs::create_directories(path.parent_path());
        
        if (cv::imwrite(outputImagePath, visualization)) {
            LOG_INFO("Saved standalone consolidation visualization to: {}", outputImagePath);
        } else {
            LOG_ERROR("Failed to save standalone visualization to: {}", outputImagePath);
        }
    }
    
    return regions;
}

cv::Mat MotionRegionConsolidator::createVisualization(
    const std::vector<TrackedObject>& trackedObjects,
    const std::vector<ConsolidatedRegion>& regions,
    const cv::Mat& inputImage) const {
    
    // Clone input image for visualization
    cv::Mat visualization = inputImage.clone();
    
    // Draw motion boxes first (underneath consolidated regions)
    drawMotionBoxes(visualization, trackedObjects);
    
    // Draw consolidated regions on top
    drawConsolidatedRegions(visualization, regions);
    
    // Add legend/info
    int legendY = 30;
    cv::putText(visualization, "Motion Boxes: Green", cv::Point(20, legendY), 
               cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    cv::putText(visualization, "Consolidated Regions: Red", cv::Point(20, legendY + 30), 
               cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
    cv::putText(visualization, "Objects: " + std::to_string(trackedObjects.size()) + 
               " -> Regions: " + std::to_string(regions.size()), 
               cv::Point(20, legendY + 60), 
               cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    
    return visualization;
}

void MotionRegionConsolidator::drawMotionBoxes(cv::Mat& image, 
                                              const std::vector<TrackedObject>& trackedObjects) const {
    
    for (const auto& obj : trackedObjects) {
        // Draw motion box in green
        cv::rectangle(image, obj.currentBounds, cv::Scalar(0, 255, 0), 2);
        
        // Add object ID label
        std::string label = "M" + std::to_string(obj.id);
        cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
        cv::Point textOrigin(obj.currentBounds.x, obj.currentBounds.y - 5);
        
        // Background for text
        cv::rectangle(image, 
                     cv::Point(textOrigin.x, textOrigin.y - textSize.height),
                     cv::Point(textOrigin.x + textSize.width, textOrigin.y + 5),
                     cv::Scalar(0, 255, 0), -1);
        
        // Text
        cv::putText(image, label, textOrigin, cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                   cv::Scalar(0, 0, 0), 1);
    }
}

void MotionRegionConsolidator::drawConsolidatedRegions(cv::Mat& image, 
                                                      const std::vector<ConsolidatedRegion>& regions) const {
    
    for (size_t i = 0; i < regions.size(); ++i) {
        const auto& region = regions[i];
        
        // Draw consolidated region in red with thicker line
        cv::rectangle(image, region.boundingBox, cv::Scalar(0, 0, 255), 4);
        
        // Add region info label
        std::string label = "R" + std::to_string(i) + " (" + 
                           std::to_string(region.trackedObjectIds.size()) + " objs)";
        cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, nullptr);
        cv::Point textOrigin(region.boundingBox.x, region.boundingBox.y - 10);
        
        // Background for text
        cv::rectangle(image, 
                     cv::Point(textOrigin.x, textOrigin.y - textSize.height - 5),
                     cv::Point(textOrigin.x + textSize.width, textOrigin.y + 5),
                     cv::Scalar(0, 0, 255), -1);
        
        // Text
        cv::putText(image, label, textOrigin, cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                   cv::Scalar(255, 255, 255), 2);
        
        // Add size info
        std::string sizeInfo = std::to_string(region.boundingBox.width) + "x" + 
                              std::to_string(region.boundingBox.height);
        cv::putText(image, sizeInfo, 
                   cv::Point(region.boundingBox.x, region.boundingBox.y + region.boundingBox.height - 10),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
    }
}

