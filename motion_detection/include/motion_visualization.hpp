/**
 * @file motion_visualization.hpp
 * @brief Visualization utilities for motion detection results
 * 
 * This class handles all visualization-related functionality for the motion tracker,
 * providing clean separation between core motion detection logic and display/rendering.
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// Forward declaration
struct TrackedObject;

/**
 * @class MotionVisualization
 * @brief Handles all visualization aspects of motion detection
 * 
 * Separates visualization concerns from core motion detection logic,
 * making the codebase more maintainable and testable.
 */
class MotionVisualization {
public:
    /**
     * @brief Constructor
     */
    MotionVisualization();
    
    /**
     * @brief Destructor
     */
    ~MotionVisualization() = default;
    
    // Configuration methods
    void setVisualizationEnabled(bool enabled);
    void setSplitScreenEnabled(bool enabled);
    void setWindowName(const std::string& windowName);
    
    /**
     * @brief Create a split-screen visualization showing processing stages
     * @param originalFrame Original input frame
     * @param processedFrame Preprocessed frame
     * @param frameDiff Frame difference image
     * @param thresholded Thresholded motion mask
     * @param finalProcessed Final processed image after morphology
     * @return Combined visualization image
     */
    cv::Mat createSplitScreenVisualization(
        const cv::Mat& originalFrame, 
        const cv::Mat& processedFrame,
        const cv::Mat& frameDiff, 
        const cv::Mat& thresholded, 
        const cv::Mat& finalProcessed
    );
    
    /**
     * @brief Get split-screen visualization for a given frame
     * @param originalFrame Input frame to process and visualize
     * @param processFrame Function pointer to process the frame
     * @return Split-screen visualization
     */
    cv::Mat getSplitScreenVisualization(const cv::Mat& originalFrame);
    
    /**
     * @brief Draw motion detection overlays on a frame
     * @param frame Input frame
     * @param trackedObjects List of detected objects
     * @param showBoundingBoxes Whether to draw bounding boxes
     * @param showTrajectories Whether to draw object trajectories
     * @return Frame with overlays drawn
     */
    cv::Mat drawMotionOverlays(
        const cv::Mat& frame, 
        const std::vector<TrackedObject>& trackedObjects,
        bool showBoundingBoxes = true,
        bool showTrajectories = true
    );
    
    /**
     * @brief Draw bounding boxes for detected objects
     * @param frame Input/output frame
     * @param trackedObjects List of detected objects
     */
    void drawBoundingBoxes(cv::Mat& frame, const std::vector<TrackedObject>& trackedObjects);
    
    /**
     * @brief Draw object trajectories
     * @param frame Input/output frame
     * @param trackedObjects List of tracked objects with history
     */
    void drawTrajectories(cv::Mat& frame, const std::vector<TrackedObject>& trackedObjects);
    
    /**
     * @brief Add text labels to visualization
     * @param frame Input/output frame
     * @param labels Text labels with positions
     */
    void addTextLabels(cv::Mat& frame, const std::vector<std::pair<std::string, cv::Point>>& labels);
    
    /**
     * @brief Display visualization in a window
     * @param image Image to display
     * @param windowName Window name (optional, uses default if empty)
     */
    void displayVisualization(const cv::Mat& image, const std::string& windowName = "");
    
    /**
     * @brief Save visualization to file
     * @param image Image to save
     * @param filename Output filename
     * @return true if successful, false otherwise
     */
    bool saveVisualization(const cv::Mat& image, const std::string& filename);
    
    // Utility methods for creating composite views
    cv::Mat createGridLayout(const std::vector<cv::Mat>& images, 
                            const std::vector<std::string>& labels,
                            int cols = 2);
    
    cv::Mat createSideBySideView(const cv::Mat& left, const cv::Mat& right,
                                const std::string& leftLabel = "",
                                const std::string& rightLabel = "");

private:
    // Configuration
    bool visualizationEnabled;
    bool splitScreenEnabled;
    std::string defaultWindowName;
    
    // Visual styling
    cv::Scalar boundingBoxColor;
    cv::Scalar trajectoryColor;
    cv::Scalar textColor;
    cv::Scalar backgroundColor;
    int lineThickness;
    double fontScale;
    int fontType;
    
    // Layout parameters
    struct LayoutConfig {
        int largePanelWidth;
        int largePanelHeight;
        int smallPanelWidth;
        int smallPanelHeight;
        int totalWidth;
        int totalHeight;
        int padding;
    } layoutConfig;
    
    // Helper methods
    void initializeLayoutConfig(int frameWidth, int frameHeight);
    cv::Mat resizeForDisplay(const cv::Mat& image, cv::Size targetSize);
    void addLabel(cv::Mat& image, const std::string& label, cv::Point position);
    cv::Scalar getColorForObject(int objectId);
    
    // Convert single channel images to 3-channel for visualization
    cv::Mat ensureThreeChannel(const cv::Mat& image);
};

/**
 * @brief Utility function to create a simple before/after comparison
 * @param before Original image
 * @param after Processed image
 * @param title Optional title for the comparison
 * @return Side-by-side comparison image
 */
cv::Mat createBeforeAfterComparison(const cv::Mat& before, const cv::Mat& after, 
                                   const std::string& title = "");

/**
 * @brief Create a processing pipeline visualization
 * @param stages Vector of processing stages with labels
 * @return Grid layout showing all stages
 */
cv::Mat createPipelineVisualization(const std::vector<std::pair<cv::Mat, std::string>>& stages);
