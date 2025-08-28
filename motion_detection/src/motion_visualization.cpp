/**
 * @file motion_visualization.cpp
 * @brief Implementation of motion detection visualization utilities
 */

#include "motion_visualization.hpp"
#include "tracked_object.hpp" // For TrackedObject definition
#include "logger.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

MotionVisualization::MotionVisualization() 
    : visualizationEnabled(true),
      splitScreenEnabled(true),
      defaultWindowName("Motion Detection"),
      boundingBoxColor(0, 255, 0),    // Green
      trajectoryColor(255, 0, 0),     // Blue  
      textColor(0, 255, 0),           // Green
      backgroundColor(0, 0, 0),       // Black
      lineThickness(2),
      fontScale(0.7),
      fontType(cv::FONT_HERSHEY_SIMPLEX) {
    
    // Initialize layout configuration with default values
    layoutConfig.largePanelWidth = 640;
    layoutConfig.largePanelHeight = 480;
    layoutConfig.smallPanelWidth = 320;
    layoutConfig.smallPanelHeight = 240;
    layoutConfig.padding = 10;
}

void MotionVisualization::setVisualizationEnabled(bool enabled) {
    visualizationEnabled = enabled;
}

void MotionVisualization::setSplitScreenEnabled(bool enabled) {
    splitScreenEnabled = enabled;
}

void MotionVisualization::setWindowName(const std::string& windowName) {
    defaultWindowName = windowName;
}

void MotionVisualization::initializeLayoutConfig(int frameWidth, int frameHeight) {
    // Calculate panel sizes based on input frame dimensions
    layoutConfig.largePanelWidth = frameWidth;
    layoutConfig.largePanelHeight = frameHeight;
    layoutConfig.smallPanelWidth = frameWidth / 2;
    layoutConfig.smallPanelHeight = frameHeight / 2;
    
    // Calculate total dimensions for split screen
    layoutConfig.totalWidth = layoutConfig.largePanelWidth * 2;
    layoutConfig.totalHeight = layoutConfig.largePanelHeight * 2;
}

cv::Mat MotionVisualization::ensureThreeChannel(const cv::Mat& image) {
    if (image.channels() == 1) {
        cv::Mat threeChannel;
        cv::cvtColor(image, threeChannel, cv::COLOR_GRAY2BGR);
        return threeChannel;
    }
    return image.clone();
}

cv::Mat MotionVisualization::resizeForDisplay(const cv::Mat& image, cv::Size targetSize) {
    cv::Mat resized;
    cv::resize(image, resized, targetSize);
    return resized;
}

void MotionVisualization::addLabel(cv::Mat& image, const std::string& label, cv::Point position) {
    if (label.empty()) return;
    
    cv::putText(image, label, position, fontType, fontScale, textColor, lineThickness);
}

cv::Scalar MotionVisualization::getColorForObject(int objectId) {
    // Generate different colors for different objects
    static std::vector<cv::Scalar> colors = {
        cv::Scalar(0, 255, 0),    // Green
        cv::Scalar(255, 0, 0),    // Blue
        cv::Scalar(0, 0, 255),    // Red
        cv::Scalar(255, 255, 0),  // Cyan
        cv::Scalar(255, 0, 255),  // Magenta
        cv::Scalar(0, 255, 255),  // Yellow
    };
    
    return colors[objectId % colors.size()];
}

cv::Mat MotionVisualization::createSplitScreenVisualization(
    const cv::Mat& originalFrame, 
    const cv::Mat& processedFrame,
    const cv::Mat& frameDiff, 
    const cv::Mat& thresholded, 
    const cv::Mat& finalProcessed) {
    
    if (!visualizationEnabled || !splitScreenEnabled) {
        return originalFrame.clone();
    }
    
    // Initialize layout based on frame size
    initializeLayoutConfig(originalFrame.cols, originalFrame.rows);
    
    // Prepare all images for display (ensure 3-channel)
    cv::Mat origDisplay = ensureThreeChannel(originalFrame);
    cv::Mat procDisplay = ensureThreeChannel(processedFrame);
    cv::Mat diffDisplay = ensureThreeChannel(frameDiff);
    cv::Mat threshDisplay = ensureThreeChannel(thresholded);
    cv::Mat finalDisplay = ensureThreeChannel(finalProcessed);
    
    // Create visualization canvas
    cv::Mat visualization(layoutConfig.totalHeight, layoutConfig.totalWidth, CV_8UC3, backgroundColor);
    
    // Large panels (top row)
    cv::Mat resizedOriginal = resizeForDisplay(origDisplay, 
        cv::Size(layoutConfig.largePanelWidth, layoutConfig.largePanelHeight));
    cv::Mat resizedFinal = resizeForDisplay(finalDisplay, 
        cv::Size(layoutConfig.largePanelWidth, layoutConfig.largePanelHeight));
    
    // Place large panels
    resizedOriginal.copyTo(visualization(cv::Rect(0, 0, 
        layoutConfig.largePanelWidth, layoutConfig.largePanelHeight)));
    resizedFinal.copyTo(visualization(cv::Rect(layoutConfig.largePanelWidth, 0, 
        layoutConfig.largePanelWidth, layoutConfig.largePanelHeight)));
    
    // Add labels for large panels
    addLabel(visualization, "Original", cv::Point(10, 30));
    addLabel(visualization, "Final Result", cv::Point(layoutConfig.largePanelWidth + 10, 30));
    
    // Small panels (bottom row)
    std::vector<cv::Mat> smallPanels = {procDisplay, diffDisplay, threshDisplay};
    std::vector<std::string> smallLabels = {"Preprocessed", "Frame Diff", "Thresholded"};
    
    int panelIndex = 0;
    for (size_t i = 0; i < smallPanels.size() && panelIndex < 4; ++i) {
        int row = panelIndex / 2;
        int col = panelIndex % 2;
        
        int x = col * layoutConfig.smallPanelWidth;
        int y = layoutConfig.largePanelHeight + row * layoutConfig.smallPanelHeight;
        
        cv::Mat resizedSmall = resizeForDisplay(smallPanels[i], 
            cv::Size(layoutConfig.smallPanelWidth, layoutConfig.smallPanelHeight));
        
        resizedSmall.copyTo(visualization(cv::Rect(x, y, 
            layoutConfig.smallPanelWidth, layoutConfig.smallPanelHeight)));
        
        addLabel(visualization, smallLabels[i], cv::Point(x + 10, y + 30));
        panelIndex++;
    }
    
    return visualization;
}

cv::Mat MotionVisualization::drawMotionOverlays(
    const cv::Mat& frame, 
    const std::vector<TrackedObject>& trackedObjects,
    bool showBoundingBoxes,
    bool showTrajectories) {
    
    if (!visualizationEnabled) {
        return frame.clone();
    }
    
    cv::Mat result = frame.clone();
    
    if (showBoundingBoxes) {
        drawBoundingBoxes(result, trackedObjects);
    }
    
    if (showTrajectories) {
        drawTrajectories(result, trackedObjects);
    }
    
    return result;
}

void MotionVisualization::drawBoundingBoxes(cv::Mat& frame, const std::vector<TrackedObject>& trackedObjects) {
    for (size_t i = 0; i < trackedObjects.size(); ++i) {
        const auto& obj = trackedObjects[i];
        cv::Scalar color = getColorForObject(obj.id);
        
        // Draw bounding box
        cv::rectangle(frame, obj.currentBounds, color, lineThickness);
        
        // Draw object ID and confidence
        std::string label = "ID:" + std::to_string(obj.id);
        if (obj.confidence > 0) {
            label += " (" + std::to_string(static_cast<int>(obj.confidence * 100)) + "%)";
        }
        
        cv::Point labelPos(obj.currentBounds.x, obj.currentBounds.y - 10);
        addLabel(frame, label, labelPos);
    }
}

void MotionVisualization::drawTrajectories(cv::Mat& frame, const std::vector<TrackedObject>& trackedObjects) {
    for (const auto& obj : trackedObjects) {
        if (obj.trajectory.size() < 2) continue;
        
        cv::Scalar color = getColorForObject(obj.id);
        
        // Draw trajectory lines
        for (size_t i = 1; i < obj.trajectory.size(); ++i) {
            cv::line(frame, obj.trajectory[i-1], obj.trajectory[i], color, lineThickness - 1);
        }
        
        // Draw trajectory points
        for (const auto& point : obj.trajectory) {
            cv::circle(frame, point, 3, color, -1);
        }
    }
}

void MotionVisualization::addTextLabels(cv::Mat& frame, 
    const std::vector<std::pair<std::string, cv::Point>>& labels) {
    
    for (const auto& labelPair : labels) {
        addLabel(frame, labelPair.first, labelPair.second);
    }
}

void MotionVisualization::displayVisualization(const cv::Mat& image, const std::string& windowName) {
    if (!visualizationEnabled) return;
    
    std::string winName = windowName.empty() ? defaultWindowName : windowName;
    cv::imshow(winName, image);
}

bool MotionVisualization::saveVisualization(const cv::Mat& image, const std::string& filename) {
    try {
        return cv::imwrite(filename, image);
    } catch (const cv::Exception& e) {
        LOG_ERROR("Failed to save visualization to {}: {}", filename, e.what());
        return false;
    }
}

cv::Mat MotionVisualization::createGridLayout(const std::vector<cv::Mat>& images, 
                                             const std::vector<std::string>& labels,
                                             int cols) {
    if (images.empty()) return cv::Mat();
    
    int rows = (images.size() + cols - 1) / cols;
    
    // Get dimensions from first image
    cv::Size cellSize(images[0].cols, images[0].rows);
    cv::Mat grid(rows * cellSize.height, cols * cellSize.width, CV_8UC3, backgroundColor);
    
    for (size_t i = 0; i < images.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        
        int x = col * cellSize.width;
        int y = row * cellSize.height;
        
        cv::Mat cell = ensureThreeChannel(images[i]);
        cv::Mat resized = resizeForDisplay(cell, cellSize);
        
        resized.copyTo(grid(cv::Rect(x, y, cellSize.width, cellSize.height)));
        
        if (i < labels.size() && !labels[i].empty()) {
            addLabel(grid, labels[i], cv::Point(x + 10, y + 30));
        }
    }
    
    return grid;
}

cv::Mat MotionVisualization::createSideBySideView(const cv::Mat& left, const cv::Mat& right,
                                                 const std::string& leftLabel,
                                                 const std::string& rightLabel) {
    cv::Mat leftDisplay = ensureThreeChannel(left);
    cv::Mat rightDisplay = ensureThreeChannel(right);
    
    // Make sure both images have the same height
    int maxHeight = std::max(leftDisplay.rows, rightDisplay.rows);
    cv::Mat resizedLeft = resizeForDisplay(leftDisplay, cv::Size(leftDisplay.cols, maxHeight));
    cv::Mat resizedRight = resizeForDisplay(rightDisplay, cv::Size(rightDisplay.cols, maxHeight));
    
    cv::Mat sideBySide(maxHeight, resizedLeft.cols + resizedRight.cols, CV_8UC3);
    
    resizedLeft.copyTo(sideBySide(cv::Rect(0, 0, resizedLeft.cols, maxHeight)));
    resizedRight.copyTo(sideBySide(cv::Rect(resizedLeft.cols, 0, resizedRight.cols, maxHeight)));
    
    if (!leftLabel.empty()) {
        addLabel(sideBySide, leftLabel, cv::Point(10, 30));
    }
    if (!rightLabel.empty()) {
        addLabel(sideBySide, rightLabel, cv::Point(resizedLeft.cols + 10, 30));
    }
    
    return sideBySide;
}

// Utility functions
cv::Mat createBeforeAfterComparison(const cv::Mat& before, const cv::Mat& after, 
                                   const std::string& title) {
    MotionVisualization viz;
    cv::Mat comparison = viz.createSideBySideView(before, after, "Before", "After");
    
    if (!title.empty()) {
        cv::putText(comparison, title, cv::Point(10, comparison.rows - 20), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
    }
    
    return comparison;
}

cv::Mat createPipelineVisualization(const std::vector<std::pair<cv::Mat, std::string>>& stages) {
    std::vector<cv::Mat> images;
    std::vector<std::string> labels;
    
    for (const auto& stage : stages) {
        images.push_back(stage.first);
        labels.push_back(stage.second);
    }
    
    MotionVisualization viz;
    return viz.createGridLayout(images, labels, 3); // 3 columns
}
