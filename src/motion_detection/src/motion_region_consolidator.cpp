#include "motion_region_consolidator.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <map>
#include <vector>

#include "logger.hpp"

namespace fs = std::filesystem;

MotionRegionConsolidator::MotionRegionConsolidator(const ConsolidationConfig& config)
    : config_(config), frameCounter_(0) {
    LOG_INFO("MotionRegionConsolidator initialized with config");
}

std::vector<std::vector<int>> MotionRegionConsolidator::dbscanClustering(
    const std::vector<TrackedObject>& objects) {
    const size_t n = objects.size();
    std::vector<int> labels(n, -1);  // -1 = unvisited, -2 = noise, >= 0 = cluster ID
    std::vector<std::vector<int>> clusters;
    int clusterId = 0;

    LOG_DEBUG("Starting DBSCAN clustering for {} objects with eps={}, minPts={}", n, config_.eps,
              config_.minPts);

    for (size_t i = 0; i < n; ++i) {
        if (labels[i] != -1) continue;  // Already processed

        // Find all neighbors within eps distance
        std::vector<int> neighbors = rangeQuery(objects, static_cast<int>(i), config_.eps);

        if (neighbors.size() < static_cast<size_t>(config_.minPts)) {
            labels[i] = -2;  // Mark as noise
            continue;
        }

        // Start a new cluster
        std::vector<int> cluster;
        labels[i] = clusterId;
        cluster.push_back(static_cast<int>(i));

        // Process all neighbors
        for (size_t j = 0; j < neighbors.size(); ++j) {
            int neighborIdx = neighbors[j];

            if (labels[neighborIdx] == -2) {
                // Change noise to border point
                labels[neighborIdx] = clusterId;
                cluster.push_back(neighborIdx);
            } else if (labels[neighborIdx] == -1) {
                // Unvisited point
                labels[neighborIdx] = clusterId;
                cluster.push_back(neighborIdx);

                // Find neighbors of this neighbor
                std::vector<int> neighborNeighbors = rangeQuery(objects, neighborIdx, config_.eps);
                if (neighborNeighbors.size() >= static_cast<size_t>(config_.minPts)) {
                    // Add new neighbors to the list for processing
                    for (int newNeighbor : neighborNeighbors) {
                        if (std::find(neighbors.begin(), neighbors.end(), newNeighbor) ==
                            neighbors.end()) {
                            neighbors.push_back(newNeighbor);
                        }
                    }
                }
            }
        }

        clusters.push_back(cluster);
        clusterId++;
    }

    LOG_DEBUG("DBSCAN clustering completed: {} clusters found", clusters.size());

    // Log cluster information
    for (size_t i = 0; i < clusters.size(); ++i) {
        LOG_DEBUG("Cluster {}: {} objects", i, clusters[i].size());
    }

    return clusters;
}

std::vector<int> MotionRegionConsolidator::rangeQuery(const std::vector<TrackedObject>& objects,
                                                      int pointIdx, double eps) {
    std::vector<int> neighbors;

    for (size_t i = 0; i < objects.size(); ++i) {
        if (static_cast<int>(i) == pointIdx) continue;

        double distance = calculateOverlapAwareDistance(objects[pointIdx], objects[i]);
        if (distance <= eps) {
            neighbors.push_back(static_cast<int>(i));
        }
    }

    return neighbors;
}

// Now implement the overlap-aware distance calculation methods
double MotionRegionConsolidator::calculateOverlapAwareDistance(const TrackedObject& obj1,
                                                               const TrackedObject& obj2) const {
    const cv::Rect& rect1 = obj1.currentBounds;
    const cv::Rect& rect2 = obj2.currentBounds;

    // Calculate overlap component (lower is better for overlapping boxes)
    double overlapComponent = calculateOverlapComponent(rect1, rect2);

    // Calculate edge distance component (distance between closest edges)
    double edgeComponent = calculateEdgeDistance(rect1, rect2);

    // Combine components using weights
    double combinedDistance =
        (config_.overlapWeight * overlapComponent) + (config_.edgeWeight * edgeComponent);

    LOG_DEBUG("Distance between objects {} and {}: overlap={:.2f}, edge={:.2f}, combined={:.2f}",
              obj1.id, obj2.id, overlapComponent, edgeComponent, combinedDistance);

    return combinedDistance;
}

double MotionRegionConsolidator::calculateOverlapComponent(const cv::Rect& rect1,
                                                           const cv::Rect& rect2) const {
    // Calculate intersection
    cv::Rect intersection = rect1 & rect2;

    if (intersection.area() == 0) {
        // No overlap - return high distance
        return config_.maxEdgeDistance;
    }

    // Calculate overlap ratio relative to smaller rectangle
    int area1 = rect1.area();
    int area2 = rect2.area();
    int minArea = std::min(area1, area2);

    if (minArea == 0) {
        return config_.maxEdgeDistance;
    }

    double overlapRatio = static_cast<double>(intersection.area()) / minArea;

    // Convert overlap ratio to distance (higher overlap = lower distance)
    // For perfect overlap (ratio = 1.0), distance = 0
    // For no overlap (ratio = 0.0), distance = maxEdgeDistance
    double overlapDistance = config_.maxEdgeDistance * (1.0 - overlapRatio);

    return overlapDistance;
}

double MotionRegionConsolidator::calculateEdgeDistance(const cv::Rect& rect1,
                                                       const cv::Rect& rect2) const {
    // Calculate minimum distance between rectangle edges

    // Get rectangle boundaries
    int left1 = rect1.x;
    int right1 = rect1.x + rect1.width;
    int top1 = rect1.y;
    int bottom1 = rect1.y + rect1.height;

    int left2 = rect2.x;
    int right2 = rect2.x + rect2.width;
    int top2 = rect2.y;
    int bottom2 = rect2.y + rect2.height;

    // Check if rectangles overlap
    if (!(right1 < left2 || right2 < left1 || bottom1 < top2 || bottom2 < top1)) {
        // Rectangles overlap - edge distance is 0
        return 0.0;
    }

    // Calculate minimum distance between edges
    double minDistance = std::numeric_limits<double>::max();

    // Horizontal distances
    if (right1 < left2) {
        // rect1 is to the left of rect2
        minDistance = std::min(minDistance, static_cast<double>(left2 - right1));
    } else if (right2 < left1) {
        // rect2 is to the left of rect1
        minDistance = std::min(minDistance, static_cast<double>(left1 - right2));
    }

    // Vertical distances
    if (bottom1 < top2) {
        // rect1 is above rect2
        minDistance = std::min(minDistance, static_cast<double>(top2 - bottom1));
    } else if (bottom2 < top1) {
        // rect2 is above rect1
        minDistance = std::min(minDistance, static_cast<double>(top1 - bottom2));
    }

    // If rectangles are diagonally separated, calculate corner-to-corner distance
    if (minDistance == std::numeric_limits<double>::max()) {
        // Calculate distances between all corner combinations
        std::vector<cv::Point> corners1 = {cv::Point(left1, top1), cv::Point(right1, top1),
                                           cv::Point(left1, bottom1), cv::Point(right1, bottom1)};
        std::vector<cv::Point> corners2 = {cv::Point(left2, top2), cv::Point(right2, top2),
                                           cv::Point(left2, bottom2), cv::Point(right2, bottom2)};

        for (const auto& corner1 : corners1) {
            for (const auto& corner2 : corners2) {
                double dx = corner1.x - corner2.x;
                double dy = corner1.y - corner2.y;
                double distance = std::sqrt(dx * dx + dy * dy);
                minDistance = std::min(minDistance, distance);
            }
        }
    }

    // Cap the distance at maxEdgeDistance
    return std::min(minDistance, config_.maxEdgeDistance);
}

std::vector<ConsolidatedRegion> MotionRegionConsolidator::consolidateRegions(
    const std::vector<TrackedObject>& trackedObjects) {
    frameCounter_++;

    if (trackedObjects.empty()) {
        removeStaleRegions();
        return consolidatedRegions_;
    }

    LOG_DEBUG("Consolidating {} tracked objects using DBSCAN", trackedObjects.size());

    // Step 1: Apply DBSCAN clustering to group objects
    auto clusters = dbscanClustering(trackedObjects);

    // Step 2: Create consolidated regions from clusters (no size filtering)
    auto newRegions = createConsolidatedRegions(trackedObjects, clusters);

    // Step 3: Update existing regions
    updateExistingRegions(trackedObjects);

    // Step 4: Merge overlapping regions if needed
    for (const auto& newRegion : newRegions) {
        bool merged = false;
        for (auto& existingRegion : consolidatedRegions_) {
            // Use simple overlap check for merging
            cv::Rect intersection = newRegion.boundingBox & existingRegion.boundingBox;
            if (intersection.area() > 0) {
                existingRegion = mergeRegions(existingRegion, newRegion);
                merged = true;
                break;
            }
        }
        if (!merged) {
            consolidatedRegions_.push_back(newRegion);
        }
    }

    // Step 5: Remove stale regions
    removeStaleRegions();

    LOG_INFO("DBSCAN consolidation completed: {} regions created", consolidatedRegions_.size());

    // Debug: Log region information
    for (size_t i = 0; i < consolidatedRegions_.size(); ++i) {
        const auto& region = consolidatedRegions_[i];
        LOG_DEBUG("Region {}: {}x{} at ({},{}) with {} objects", i, region.boundingBox.width,
                  region.boundingBox.height, region.boundingBox.x, region.boundingBox.y,
                  region.trackedObjectIds.size());
    }

    return consolidatedRegions_;
}

std::vector<ConsolidatedRegion> MotionRegionConsolidator::createConsolidatedRegions(
    const std::vector<TrackedObject>& objects, const std::vector<std::vector<int>>& clusters) {
    std::vector<ConsolidatedRegion> regions;

    for (const auto& cluster : clusters) {
        if (cluster.empty()) continue;

        // Calculate bounding box that encompasses all objects in the cluster
        cv::Rect bbox = calculateBoundingBox(objects, cluster);

        // Apply minimal expansion only
        bbox = expandBoundingBox(bbox, config_.regionExpansionFactor, config_.frameSize);

        // Clamp to frame boundaries
        bbox.x = std::max(0, std::min(bbox.x, config_.frameSize.width - bbox.width));
        bbox.y = std::max(0, std::min(bbox.y, config_.frameSize.height - bbox.height));
        bbox.width = std::min(bbox.width, config_.frameSize.width - bbox.x);
        bbox.height = std::min(bbox.height, config_.frameSize.height - bbox.y);

        regions.emplace_back(bbox, cluster);
        LOG_DEBUG("Created consolidated region: {}x{} at ({},{}) with {} objects", bbox.width,
                  bbox.height, bbox.x, bbox.y, cluster.size());
    }

    LOG_INFO("Created {} consolidated regions from {} clusters", regions.size(), clusters.size());
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

ConsolidatedRegion MotionRegionConsolidator::mergeRegions(const ConsolidatedRegion& region1,
                                                          const ConsolidatedRegion& region2) {
    // Merge bounding boxes
    cv::Rect mergedBox = region1.boundingBox | region2.boundingBox;

    // Merge object IDs
    std::vector<int> mergedIds = region1.trackedObjectIds;
    mergedIds.insert(mergedIds.end(), region2.trackedObjectIds.begin(),
                     region2.trackedObjectIds.end());

    ConsolidatedRegion merged(mergedBox, mergedIds);
    merged.framesSinceLastUpdate =
        std::min(region1.framesSinceLastUpdate, region2.framesSinceLastUpdate);

    return merged;
}

cv::Rect MotionRegionConsolidator::calculateBoundingBox(const std::vector<TrackedObject>& objects,
                                                        const std::vector<int>& indices) const {
    if (indices.empty()) {
        return cv::Rect();
    }

    cv::Rect combinedBox = objects[indices[0]].currentBounds;
    for (size_t i = 1; i < indices.size(); ++i) {
        combinedBox |= objects[indices[i]].currentBounds;
    }

    return combinedBox;
}

cv::Rect MotionRegionConsolidator::expandBoundingBox(const cv::Rect& bbox, double expansionFactor,
                                                     const cv::Size& frameSize) {
    int expandX = static_cast<int>((bbox.width * (expansionFactor - 1.0)) / 2.0);
    int expandY = static_cast<int>((bbox.height * (expansionFactor - 1.0)) / 2.0);

    cv::Rect expanded(
        std::max(0, bbox.x - expandX), std::max(0, bbox.y - expandY),
        std::min(frameSize.width - std::max(0, bbox.x - expandX), bbox.width + 2 * expandX),
        std::min(frameSize.height - std::max(0, bbox.y - expandY), bbox.height + 2 * expandY));

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
    const std::vector<TrackedObject>& trackedObjects, const cv::Mat& inputImage,
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
    const std::vector<TrackedObject>& trackedObjects, const std::string& outputImagePath) {
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
    const std::vector<ConsolidatedRegion>& regions, const cv::Mat& inputImage) const {
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
    cv::putText(visualization,
                "Objects: " + std::to_string(trackedObjects.size()) +
                    " -> Regions: " + std::to_string(regions.size()),
                cv::Point(20, legendY + 60), cv::FONT_HERSHEY_SIMPLEX, 0.7,
                cv::Scalar(255, 255, 255), 2);

    return visualization;
}

void MotionRegionConsolidator::drawMotionBoxes(
    cv::Mat& image, const std::vector<TrackedObject>& trackedObjects) const {
    for (const auto& obj : trackedObjects) {
        // Draw motion box in green
        cv::rectangle(image, obj.currentBounds, cv::Scalar(0, 255, 0), 2);

        // Add object ID label
        std::string label = "M" + std::to_string(obj.id);
        cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
        cv::Point textOrigin(obj.currentBounds.x, obj.currentBounds.y - 5);

        // Background for text
        cv::rectangle(image, cv::Point(textOrigin.x, textOrigin.y - textSize.height),
                      cv::Point(textOrigin.x + textSize.width, textOrigin.y + 5),
                      cv::Scalar(0, 255, 0), -1);

        // Text
        cv::putText(image, label, textOrigin, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0),
                    1);
    }
}

void MotionRegionConsolidator::drawConsolidatedRegions(
    cv::Mat& image, const std::vector<ConsolidatedRegion>& regions) const {
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
        cv::rectangle(image, cv::Point(textOrigin.x, textOrigin.y - textSize.height - 5),
                      cv::Point(textOrigin.x + textSize.width, textOrigin.y + 5),
                      cv::Scalar(0, 0, 255), -1);

        // Text
        cv::putText(image, label, textOrigin, cv::FONT_HERSHEY_SIMPLEX, 0.7,
                    cv::Scalar(255, 255, 255), 2);

        // Add size info
        std::string sizeInfo = std::to_string(region.boundingBox.width) + "x" +
                               std::to_string(region.boundingBox.height);
        cv::putText(
            image, sizeInfo,
            cv::Point(region.boundingBox.x, region.boundingBox.y + region.boundingBox.height - 10),
            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
    }
}
