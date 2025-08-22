#include "motion_tracker.hpp"
#include "logger.hpp"
#include <iostream>

MotionTracker::MotionTracker(const std::string& configPath) 
    : processor(configPath),
      tracker(configPath),
      visualization(),
      running(false),
      splitScreen(true),
      drawContours(true),
      dataCollection(true),
      saveOnMotion(true),
      splitScreenWindowName("Motion Detection - Split Screen View") {
    loadConfig(configPath);
}

MotionTracker::~MotionTracker() {
    stop();
}

bool MotionTracker::initialize(const std::string& videoSource) {
    cap.open(videoSource);
    if (!cap.isOpened()) {
        return false;
    }
    running = true;
    return true;
}

bool MotionTracker::initialize(int deviceIndex) {
    cap.open(deviceIndex);
    if (!cap.isOpened()) {
        LOG_CRITICAL("Error: Could not open video device index {}", deviceIndex);
        return false;
    }
    
    running = true;
    return true;
}

void MotionTracker::stop() {
    running = false;
    if (cap.isOpened()) {
        cap.release();
    }
}

void MotionTracker::processFrame() {
    if (!cap.isOpened()) {
        LOG_ERROR("Camera not initialized");
        return;
    }
    
    cv::Mat frame;
    cap >> frame;
    
    if (frame.empty()) {
        LOG_ERROR("Failed to capture frame");
        return;
    }
    
    // Process the captured frame using the overloaded version
    MotionResult result = processFrame(frame);
    
    // Log results if motion detected
    if (result.hasMotion) {
        LOG_INFO("Motion detected with " + std::to_string(result.trackedObjects.size()) + " objects");
    }
}

// ============================================================================
// MAIN PROCESSING PIPELINE - Orchestrates MotionProcessor and ObjectTracker
// ============================================================================

MotionResult MotionTracker::processFrame(const cv::Mat& frame) {
    MotionResult result;
    result.hasMotion = false;
    
    if (frame.empty()) {
        return result;
    }
    
    // Step 1: Process the frame using MotionProcessor
    MotionProcessor::ProcessingResult processingResult = processor.processFrame(frame);
    
    // Store intermediate results for visualization
    result.processedFrame = processingResult.processedFrame;
    result.frameDiff = processingResult.frameDiff;
    result.thresh = processingResult.thresh;
    result.processed = processingResult.morphological;
    result.hasMotion = processingResult.hasMotion;
    
    // Step 2: Track objects using ObjectTracker
    ObjectTracker::TrackingResult trackingResult = tracker.trackObjects(processingResult.detectedBounds, frame);
    result.trackedObjects = trackingResult.trackedObjects;
    
    // Step 3: Create and display split-screen visualization if enabled
    if (splitScreen && result.hasMotion) {
        cv::Mat vizResult = visualization.createSplitScreenVisualization(
            frame, 
            result.processedFrame, 
            result.frameDiff, 
            result.thresh, 
            result.processed
        );
        cv::imshow(splitScreenWindowName, vizResult);
    }
    
    return result;
}

// ============================================================================
// PUBLIC ACCESS TO INDIVIDUAL PROCESSING STEPS (for testing)
// ============================================================================

cv::Mat MotionTracker::preprocessFrame(const cv::Mat& frame) {
    return processor.preprocessFrame(frame);
}

cv::Mat MotionTracker::detectMotion(const cv::Mat& processedFrame, cv::Mat& frameDiff, cv::Mat& thresh) {
    return processor.detectMotion(processedFrame, frameDiff, thresh);
}

cv::Mat MotionTracker::applyMorphologicalOps(const cv::Mat& thresh) {
    return processor.applyMorphologicalOps(thresh);
}

std::vector<cv::Rect> MotionTracker::extractContours(const cv::Mat& processed) {
    return processor.extractContours(processed);
}

void MotionTracker::logTrackingResults() {
    tracker.logTrackingResults();
}

void MotionTracker::setPrevFrame(const cv::Mat& frame) {
    processor.setPrevFrame(frame);
}

// ============================================================================
// CONFIGURATION AND GETTERS
// ============================================================================

void MotionTracker::loadConfig(const std::string& configPath) {
    try {
        YAML::Node config = YAML::LoadFile(configPath);
        
        // Visualization parameters
        if (config["enable_split_screen"]) splitScreen = config["enable_split_screen"].as<bool>();
        if (config["split_screen_window_name"]) splitScreenWindowName = config["split_screen_window_name"].as<std::string>();
        if (config["enable_draw_contours"]) drawContours = config["enable_draw_contours"].as<bool>();
        if (config["enable_data_collection"]) dataCollection = config["enable_data_collection"].as<bool>();
        if (config["enable_save_on_motion"]) saveOnMotion = config["enable_save_on_motion"].as<bool>();
        
        LOG_INFO("MotionTracker config loaded: split_screen={}", splitScreen);
        
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Warning: Could not load config file: {}. Error: {}", configPath, e.what());
    }
}

// Getters for configuration values
bool MotionTracker::isSplitScreenEnabled() const { return splitScreen; }
const std::string& MotionTracker::getSplitScreenWindowName() const { return splitScreenWindowName; }
MotionVisualization& MotionTracker::getVisualization() { return visualization; }

// Access to tracked objects
const std::vector<TrackedObject>& MotionTracker::getTrackedObjects() const { 
    return tracker.getTrackedObjects(); 
}

const TrackedObject* MotionTracker::findTrackedObjectById(int id) const {
    return tracker.findTrackedObjectById(id);
}

void MotionTracker::setTrackedObjects(const std::vector<TrackedObject>& objects) {
    tracker.setTrackedObjects(objects);
}

void MotionTracker::clearLostObjectIds() {
    tracker.clearLostObjectIds();
}

const std::vector<int>& MotionTracker::getLostObjectIds() const {
    return tracker.getLostObjectIds();
}

size_t MotionTracker::getMinTrajectoryLength() const {
    return tracker.getMinTrajectoryLength();
}
