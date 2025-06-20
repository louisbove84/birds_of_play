#include "motion_tracker.hpp"
#include "logger.hpp"
#include <iostream>
#include <cmath>
#include <random>
#include <sstream>

MotionTracker::MotionTracker(const std::string& configPath) 
    : isRunning(false),
      isFirstFrame(true),
      nextObjectId(0),
      maxTrajectoryPoints(30),
      minTrajectoryLength(10),
      thresholdValue(25.0),
      minContourArea(500),
      maxTrackingDistance(100.0),
      maxThreshold(255),
      smoothingFactor(0.6),
      minTrackingConfidence(0.5),
      
      // ===============================
      // INPUT COLOR PROCESSING
      // ===============================
      processingMode("grayscale"),
      
      // ===============================
      // IMAGE PREPROCESSING
      // ===============================
      contrastEnhancement(false),
      blurType("gaussian"),
      
      // ===============================
      // MOTION DETECTION METHODS
      // ===============================
      backgroundSubtraction(true),
      opticalFlowMode("none"),
      motionHistoryDuration(0.0),
      
      // ===============================
      // THRESHOLDING
      // ===============================
      thresholdType("binary"),
      
      // ===============================
      // MORPHOLOGICAL OPERATIONS
      // ===============================
      morphology(true),
      morphKernelSize(5),
      morphClose(true),
      morphOpen(true),
      dilation(true),
      erosion(false),
      
      // ===============================
      // CONTOUR PROCESSING
      // ===============================
      convexHull(true),
      contourApproximation(true),
      contourFiltering(true),
      maxContourAspectRatio(2.0),
      minContourSolidity(0.85),
      contourEpsilonFactor(0.03),
      
      // ===============================
      // VISUALIZATION & OUTPUT
      // ===============================
      splitScreen(true),
      drawContours(true),
      dataCollection(true),
      saveOnMotion(true),
      splitScreenWindowName("Motion Detection - Split Screen View"),
      
      // ===============================
      // ADVANCED PARAMETERS
      // ===============================
      
      // Contrast Enhancement (CLAHE)
      claheClipLimit(2.0),
      claheTileSize(8),
      
      // Blur Parameters
      gaussianBlurSize(5),
      medianBlurSize(5),
      bilateralD(15),
      bilateralSigmaColor(75.0),
      bilateralSigmaSpace(75.0),
      
      // Background Subtraction (MOG2)
      backgroundHistory(500),
      backgroundThreshold(16.0),
      backgroundDetectShadows(true),
      
      // Edge Detection (Canny)
      cannyLowThreshold(50),
      cannyHighThreshold(150),
      
      // Adaptive Thresholding
      adaptiveBlockSize(11),
      adaptiveC(2),
      
      // HSV Color Filtering
      hsvLower(0, 30, 60),
      hsvUpper(20, 150, 255),
      
      // Motion History
      motionHistoryFps(30.0) {
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
    isRunning = true;
    return true;
}

bool MotionTracker::initialize(int deviceIndex) {
    cap.open(deviceIndex);
    if (!cap.isOpened()) {
        Logger::getInstance()->critical("Error: Could not open video device index {}", deviceIndex);
        return false;
    }
    isRunning = true;
    return true;
}

void MotionTracker::stop() {
    isRunning = false;
    if (cap.isOpened()) {
        cap.release();
    }
}

void MotionTracker::loadConfig(const std::string& configPath) {
    try {
        YAML::Node config = YAML::LoadFile(configPath);
        if (config["threshold_value"]) thresholdValue = config["threshold_value"].as<double>();
        if (config["min_contour_area"]) minContourArea = config["min_contour_area"].as<int>();
        if (config["max_tracking_distance"]) maxTrackingDistance = config["max_tracking_distance"].as<double>();
        if (config["max_trajectory_points"]) maxTrajectoryPoints = config["max_trajectory_points"].as<size_t>();
        if (config["min_trajectory_length"]) minTrajectoryLength = config["min_trajectory_length"].as<size_t>();
        Logger::getInstance()->info("Loaded min_trajectory_length: {}", minTrajectoryLength);
        
        if (config["max_threshold"]) maxThreshold = config["max_threshold"].as<int>();
        if (config["smoothing_factor"]) smoothingFactor = config["smoothing_factor"].as<double>();
        if (config["min_tracking_confidence"]) minTrackingConfidence = config["min_tracking_confidence"].as<double>();
        if (config["gaussian_blur_size"]) gaussianBlurSize = config["gaussian_blur_size"].as<int>();
        if (config["morphology_kernel_size"]) morphKernelSize = config["morphology_kernel_size"].as<int>();
        if (config["enable_morphology"]) morphology = config["enable_morphology"].as<bool>();
        if (config["enable_dilation"]) dilation = config["enable_dilation"].as<bool>();
        if (config["enable_morph_close"]) morphClose = config["enable_morph_close"].as<bool>();
        if (config["enable_morph_open"]) morphOpen = config["enable_morph_open"].as<bool>();
        if (config["enable_erosion"]) erosion = config["enable_erosion"].as<bool>();
        if (config["enable_contrast_enhancement"]) contrastEnhancement = config["enable_contrast_enhancement"].as<bool>();
        if (config["clahe_clip_limit"]) claheClipLimit = config["clahe_clip_limit"].as<double>();
        if (config["clahe_tile_size"]) claheTileSize = config["clahe_tile_size"].as<int>();
        if (config["enable_median_blur"]) medianBlurSize = config["enable_median_blur"].as<int>();
        if (config["median_blur_size"]) medianBlurSize = config["median_blur_size"].as<int>();
        if (config["enable_bilateral_filter"]) bilateralD = config["enable_bilateral_filter"].as<int>();
        if (config["bilateral_d"]) bilateralD = config["bilateral_d"].as<int>();
        if (config["bilateral_sigma_color"]) bilateralSigmaColor = config["bilateral_sigma_color"].as<double>();
        if (config["bilateral_sigma_space"]) bilateralSigmaSpace = config["bilateral_sigma_space"].as<double>();
        if (config["enable_adaptive_threshold"]) adaptiveBlockSize = config["enable_adaptive_threshold"].as<int>();
        if (config["adaptive_block_size"]) adaptiveBlockSize = config["adaptive_block_size"].as<int>();
        if (config["adaptive_c"]) adaptiveC = config["adaptive_c"].as<int>();
        
        // Processing mode
        if (config["processing_mode"]) processingMode = config["processing_mode"].as<std::string>();
        
        // Visualization parameters
        if (config["enable_split_screen"]) splitScreen = config["enable_split_screen"].as<bool>();
        if (config["split_screen_window_name"]) splitScreenWindowName = config["split_screen_window_name"].as<std::string>();
        
        // Background Subtraction parameters
        if (config["enable_background_subtraction"]) backgroundSubtraction = config["enable_background_subtraction"].as<bool>();
        if (config["background_history"]) backgroundHistory = config["background_history"].as<int>();
        if (config["background_threshold"]) backgroundThreshold = config["background_threshold"].as<double>();
        if (config["background_detect_shadows"]) backgroundDetectShadows = config["background_detect_shadows"].as<bool>();
        
        // Convex Hull parameters
        if (config["enable_convex_hull"]) convexHull = config["enable_convex_hull"].as<bool>();
        if (config["max_contour_aspect_ratio"]) maxContourAspectRatio = config["max_contour_aspect_ratio"].as<double>();
        if (config["min_contour_solidity"]) minContourSolidity = config["min_contour_solidity"].as<double>();
        
        // HSV Color Filtering parameters
        if (config["hsv_lower_h"]) hsvLower[0] = config["hsv_lower_h"].as<int>();
        if (config["hsv_lower_s"]) hsvLower[1] = config["hsv_lower_s"].as<int>();
        if (config["hsv_lower_v"]) hsvLower[2] = config["hsv_lower_v"].as<int>();
        if (config["hsv_upper_h"]) hsvUpper[0] = config["hsv_upper_h"].as<int>();
        if (config["hsv_upper_s"]) hsvUpper[1] = config["hsv_upper_s"].as<int>();
        if (config["hsv_upper_v"]) hsvUpper[2] = config["hsv_upper_v"].as<int>();
        
        // Edge Detection parameters
        if (config["enable_edge_detection"]) cannyLowThreshold = config["enable_edge_detection"].as<int>();
        if (config["canny_low_threshold"]) cannyLowThreshold = config["canny_low_threshold"].as<int>();
        if (config["canny_high_threshold"]) cannyHighThreshold = config["canny_high_threshold"].as<int>();
        
        // Contour Processing parameters
        if (config["enable_contour_approximation"]) contourApproximation = config["enable_contour_approximation"].as<bool>();
        if (config["contour_epsilon_factor"]) contourEpsilonFactor = config["contour_epsilon_factor"].as<double>();
        if (config["enable_contour_filtering"]) contourFiltering = config["enable_contour_filtering"].as<bool>();
        if (config["max_contour_aspect_ratio"]) maxContourAspectRatio = config["max_contour_aspect_ratio"].as<double>();
        if (config["min_contour_solidity"]) minContourSolidity = config["min_contour_solidity"].as<double>();
        
        // Motion Detection parameters
        if (config["optical_flow_mode"]) opticalFlowMode = config["optical_flow_mode"].as<std::string>();
        if (config["motion_history_duration"]) motionHistoryDuration = config["motion_history_duration"].as<double>();
        if (config["motion_history_fps"]) motionHistoryFps = config["motion_history_fps"].as<double>();
        
        // Thresholding parameters
        if (config["threshold_type"]) thresholdType = config["threshold_type"].as<std::string>();
        
        // Morphological operations parameters
        if (config["enable_morphology"]) morphology = config["enable_morphology"].as<bool>();
        if (config["enable_morph_close"]) morphClose = config["enable_morph_close"].as<bool>();
        if (config["enable_morph_open"]) morphOpen = config["enable_morph_open"].as<bool>();
        if (config["enable_erosion"]) erosion = config["enable_erosion"].as<bool>();
        
        // Visualization parameters
        if (config["enable_draw_contours"]) drawContours = config["enable_draw_contours"].as<bool>();
        if (config["enable_data_collection"]) dataCollection = config["enable_data_collection"].as<bool>();
        if (config["enable_save_on_motion"]) saveOnMotion = config["enable_save_on_motion"].as<bool>();
        
        // Debug: Print loaded config values
        Logger::getInstance()->info("=== CONFIG LOADED ===");
        Logger::getInstance()->info("min_trajectory_length: {}", minTrajectoryLength);
        Logger::getInstance()->info("min_contour_area: {}", minContourArea);
        Logger::getInstance()->info("threshold_value: {}", thresholdValue);
        Logger::getInstance()->info("=====================");
    } catch (const YAML::Exception& e) {
        Logger::getInstance()->critical("Warning: Could not load config file: {}. Error: {}", configPath, e.what());
    }
}

TrackedObject* MotionTracker::findNearestObject(const cv::Rect& bounds) {
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

cv::Point MotionTracker::smoothPosition(const cv::Point& newPos, const cv::Point& smoothedPos) {
    return cv::Point(
        static_cast<int>(smoothedPos.x * smoothingFactor + newPos.x * (1 - smoothingFactor)),
        static_cast<int>(smoothedPos.y * smoothingFactor + newPos.y * (1 - smoothingFactor))
    );
}

void MotionTracker::updateTrajectories(std::vector<cv::Rect>& newBounds) {
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

cv::Mat MotionTracker::createSplitScreenVisualization(const cv::Mat& originalFrame, const cv::Mat& processedFrame, 
                                                      const cv::Mat& frameDiff, const cv::Mat& thresholded, 
                                                      const cv::Mat& finalProcessed) {
    // Prepare view names and frames
    std::vector<std::string> viewNames = {"Original"};
    std::vector<cv::Mat> viewFrames = {originalFrame};
    bool hasMorphology = false;
    int morphIndex = -1;

    // Add color space conversion view if not RGB
    if (processingMode != "rgb") {
        viewNames.push_back(processingMode.substr(0, 1).append(processingMode.substr(1)).append(" Convert"));
        cv::Mat convertedColor;
        if (processingMode == "grayscale") {
            cv::cvtColor(processedFrame, convertedColor, cv::COLOR_GRAY2BGR);
        } else if (processingMode == "hsv") {
            cv::Mat hsvFrame, hsvMask;
            cv::cvtColor(originalFrame, hsvFrame, cv::COLOR_BGR2HSV);
            cv::inRange(hsvFrame, hsvLower, hsvUpper, hsvMask);
            cv::cvtColor(hsvMask, convertedColor, cv::COLOR_GRAY2BGR);
        } else if (processingMode == "ycrcb") {
            cv::Mat ycrcbFrame;
            cv::cvtColor(originalFrame, ycrcbFrame, cv::COLOR_BGR2YCrCb);
            std::vector<cv::Mat> channels;
            cv::split(ycrcbFrame, channels);
            cv::cvtColor(channels[0], convertedColor, cv::COLOR_GRAY2BGR);
        }
        viewFrames.push_back(convertedColor);
    }
    // Add processed frame if any preprocessing was applied
    bool anyPreprocessing = contrastEnhancement || (blurType != "none");
    if (anyPreprocessing) {
        viewNames.push_back("Preprocessed");
        cv::Mat processedColor;
        cv::cvtColor(processedFrame, processedColor, cv::COLOR_GRAY2BGR);
        viewFrames.push_back(processedColor);
    }
    // Add edge detection if enabled
    if (cannyLowThreshold > 0 && cannyHighThreshold > 0) {
        viewNames.push_back("Edges");
        cv::Mat edgeMask;
        cv::Canny(processedFrame, edgeMask, cannyLowThreshold, cannyHighThreshold);
        cv::Mat edgeColor;
        cv::cvtColor(edgeMask, edgeColor, cv::COLOR_GRAY2BGR);
        viewFrames.push_back(edgeColor);
    }
    // Add background subtraction if enabled
    if (backgroundSubtraction && !bgSubtractor.empty()) {
        viewNames.push_back("BG Subtract");
        cv::Mat bgMask;
        bgSubtractor->apply(processedFrame, bgMask);
        cv::Mat bgColor;
        cv::cvtColor(bgMask, bgColor, cv::COLOR_GRAY2BGR);
        viewFrames.push_back(bgColor);
    }
    // Always show frame difference
    viewNames.push_back("Frame Diff");
    cv::Mat diffColor;
    cv::cvtColor(frameDiff, diffColor, cv::COLOR_GRAY2BGR);
    viewFrames.push_back(diffColor);
    // Always show thresholded
    viewNames.push_back("Thresholded");
    cv::Mat threshColor;
    cv::cvtColor(thresholded, threshColor, cv::COLOR_GRAY2BGR);
    viewFrames.push_back(threshColor);
    // Add final processed if morphology was applied
    if (morphology) {
        viewNames.push_back("Morphology");
        cv::Mat finalColor;
        cv::cvtColor(finalProcessed, finalColor, cv::COLOR_GRAY2BGR);
        viewFrames.push_back(finalColor);
        hasMorphology = true;
        morphIndex = static_cast<int>(viewFrames.size()) - 1;
    }
    // Draw overlays on full-size original and morphology
    cv::Mat originalWithOverlay = drawMotionOverlays(originalFrame.clone());
    cv::Mat morphWithOverlay;
    if (hasMorphology) {
        morphWithOverlay = drawMotionOverlays(viewFrames[morphIndex].clone());
    }
    // Layout: Large panels for Original and Morphology (if present), rest are small
    int smallPanelCount = static_cast<int>(viewFrames.size()) - 1 - (hasMorphology ? 1 : 0);
    int largePanelWidth = originalFrame.cols / 2;
    int largePanelHeight = originalFrame.rows / 2;
    int smallPanelWidth = originalFrame.cols / 4;
    int smallPanelHeight = originalFrame.rows / 4;
    int totalWidth = largePanelWidth * (hasMorphology ? 2 : 1);
    int totalHeight = largePanelHeight + (smallPanelCount > 0 ? smallPanelHeight : 0);
    
    // Create visualization canvas with black background
    cv::Mat visualization(totalHeight, totalWidth, CV_8UC3, cv::Scalar(0, 0, 0));
    
    // Place Original (large, top-left)
    cv::Mat resizedOriginal;
    cv::resize(originalWithOverlay, resizedOriginal, cv::Size(largePanelWidth, largePanelHeight));
    resizedOriginal.copyTo(visualization(cv::Rect(0, 0, largePanelWidth, largePanelHeight)));
    cv::putText(visualization, "Original", cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(0,255,0), 2);
    
    // Place Morphology (large, top-right) if present
    if (hasMorphology) {
        cv::Mat resizedMorph;
        cv::resize(morphWithOverlay, resizedMorph, cv::Size(largePanelWidth, largePanelHeight));
        resizedMorph.copyTo(visualization(cv::Rect(largePanelWidth, 0, largePanelWidth, largePanelHeight)));
        cv::putText(visualization, "Morphology", cv::Point(largePanelWidth+10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(0,255,0), 2);
    } else {
        // Fill the right half with black if no morphology
        cv::rectangle(visualization, cv::Rect(largePanelWidth, 0, largePanelWidth, largePanelHeight), cv::Scalar(0,0,0), -1);
    }
    
    // Place small panels in a row below
    int smallY = largePanelHeight;
    int smallX = 0;
    for (int i = 1; i < static_cast<int>(viewFrames.size()); ++i) {
        if (hasMorphology && i == morphIndex) continue; // skip, already placed
        if (viewNames[i] == "Original") continue; // skip, already placed
        
        cv::Mat resizedSmall;
        cv::resize(viewFrames[i], resizedSmall, cv::Size(smallPanelWidth, smallPanelHeight));
        resizedSmall.copyTo(visualization(cv::Rect(smallX, smallY, smallPanelWidth, smallPanelHeight)));
        cv::putText(visualization, viewNames[i], cv::Point(smallX+10, smallY+30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,0), 2);
        smallX += smallPanelWidth;
        if (smallX + smallPanelWidth > totalWidth) break; // only one row
    }
    
    // Fill remaining space in the small panels row with black rectangles
    while (smallX < totalWidth) {
        cv::rectangle(visualization, cv::Rect(smallX, smallY, smallPanelWidth, smallPanelHeight), cv::Scalar(0,0,0), -1);
        smallX += smallPanelWidth;
    }
    
    // Fill any remaining vertical space with black
    if (smallPanelCount == 0) {
        // If no small panels, fill the entire bottom half with black
        cv::rectangle(visualization, cv::Rect(0, largePanelHeight, totalWidth, largePanelHeight), cv::Scalar(0,0,0), -1);
    }
    
    return visualization;
}

cv::Mat MotionTracker::getSplitScreenVisualization(const cv::Mat& originalFrame) {
    // Process the frame to get all intermediate results
    cv::Mat processedFrame = originalFrame.clone();
    
    // Apply color space conversion
    if (processingMode == "grayscale") {
        cv::cvtColor(processedFrame, processedFrame, cv::COLOR_BGR2GRAY);
    } else if (processingMode == "hsv") {
        cv::cvtColor(processedFrame, processedFrame, cv::COLOR_BGR2HSV);
    } else if (processingMode == "ycrcb") {
        cv::cvtColor(processedFrame, processedFrame, cv::COLOR_BGR2YCrCb);
    }
    
    // Apply preprocessing
    if (contrastEnhancement) {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(claheClipLimit, cv::Size(claheTileSize, claheTileSize));
        clahe->apply(processedFrame, processedFrame);
    }
    
    if (blurType == "gaussian") {
        cv::GaussianBlur(processedFrame, processedFrame, cv::Size(gaussianBlurSize, gaussianBlurSize), 0);
    } else if (blurType == "median") {
        cv::medianBlur(processedFrame, processedFrame, medianBlurSize);
    } else if (blurType == "bilateral") {
        cv::bilateralFilter(processedFrame, processedFrame, bilateralD, bilateralSigmaColor, bilateralSigmaSpace);
    }
    
    // Calculate frame difference
    cv::Mat frameDiff;
    if (!prevFrame.empty()) {
        cv::absdiff(processedFrame, prevFrame, frameDiff);
    } else {
        frameDiff = cv::Mat::zeros(processedFrame.size(), processedFrame.type());
    }
    
    // Apply thresholding
    cv::Mat thresholded;
    if (thresholdType == "binary") {
        cv::threshold(frameDiff, thresholded, thresholdValue, maxThreshold, cv::THRESH_BINARY);
    } else if (thresholdType == "adaptive") {
        cv::adaptiveThreshold(frameDiff, thresholded, maxThreshold, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, adaptiveBlockSize, adaptiveC);
    } else if (thresholdType == "otsu") {
        cv::threshold(frameDiff, thresholded, 0, maxThreshold, cv::THRESH_BINARY | cv::THRESH_OTSU);
    } else {
        thresholded = frameDiff;
    }
    
    // Apply morphological operations
    cv::Mat finalProcessed = thresholded.clone();
    if (morphology) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(morphKernelSize, morphKernelSize));
        
        if (morphClose) {
            cv::morphologyEx(finalProcessed, finalProcessed, cv::MORPH_CLOSE, kernel);
        }
        if (morphOpen) {
            cv::morphologyEx(finalProcessed, finalProcessed, cv::MORPH_OPEN, kernel);
        }
        if (dilation) {
            cv::dilate(finalProcessed, finalProcessed, kernel);
        }
        if (erosion) {
            cv::erode(finalProcessed, finalProcessed, kernel);
        }
    }
    
    // Create and return the split-screen visualization
    return createSplitScreenVisualization(originalFrame, processedFrame, frameDiff, thresholded, finalProcessed);
}

cv::Mat MotionTracker::drawMotionOverlays(const cv::Mat& frame) {
    cv::Mat result = frame.clone();
    
    // Draw motion detection overlays
    for (const auto& obj : trackedObjects) {
        // Draw bounding box
        cv::Scalar boxColor;
        if (obj.confidence > 0.8) {
            boxColor = cv::Scalar(0, 255, 0);  // Green for high confidence
        } else if (obj.confidence > 0.6) {
            boxColor = cv::Scalar(0, 255, 255);  // Yellow for medium confidence
        } else {
            boxColor = cv::Scalar(0, 0, 255);  // Red for low confidence
        }
        
        cv::rectangle(result, obj.currentBounds, boxColor, 3);
        
        // Draw object ID and confidence
        std::string label = "ID:" + std::to_string(obj.id) + " (" + 
                           std::to_string(static_cast<int>(obj.confidence * 100)) + "%)";
        cv::putText(result, label, 
                    cv::Point(obj.currentBounds.x, obj.currentBounds.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, boxColor, 2);
        
        // Draw motion trail
        if (obj.trajectory.size() > 1) {
            // Draw trail with fading colors
            for (size_t j = 1; j < obj.trajectory.size(); ++j) {
                double alpha = static_cast<double>(j) / obj.trajectory.size();
                cv::Scalar trailColor(
                    static_cast<int>(boxColor[0] * alpha),
                    static_cast<int>(boxColor[1] * alpha),
                    static_cast<int>(boxColor[2] * alpha)
                );
                cv::line(result, obj.trajectory[j-1], obj.trajectory[j], trailColor, 3);
            }
            
            // Draw current position as a circle
            cv::circle(result, obj.trajectory.back(), 8, boxColor, -1);
        }
    }
    
    // Add status information
    std::string statusText = "Objects: " + std::to_string(trackedObjects.size());
    cv::putText(result, statusText, cv::Point(10, 30), 
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
    
    return result;
}

void MotionTracker::initializeBackgroundSubtractor() {
    if (backgroundSubtraction) {
        bgSubtractor = cv::createBackgroundSubtractorMOG2(
            backgroundHistory, 
            backgroundThreshold, 
            backgroundDetectShadows
        );
        Logger::getInstance()->info("Using Background Subtraction (MOG2)");
    }
}

MotionResult MotionTracker::processFrame(const cv::Mat& frame) {
    MotionResult result;
    result.hasMotion = false;
    
    if (frame.empty()) {
        return result;
    }
    
    // Initialize background subtractor if needed
    if (backgroundSubtraction && bgSubtractor.empty()) {
        initializeBackgroundSubtractor();
    }
    
    // Start with the original frame for processing
    cv::Mat processedFrame;
    
    // 1. Apply processing mode (HSV, Grayscale, or RGB)
    if (processingMode == "hsv") {
        // HSV Color Filtering
        cv::Mat hsvFrame;
        cv::cvtColor(frame, hsvFrame, cv::COLOR_BGR2HSV);
        cv::inRange(hsvFrame, hsvLower, hsvUpper, processedFrame);
    } else if (processingMode == "grayscale") {
        // Grayscale processing (default)
        cv::cvtColor(frame, processedFrame, cv::COLOR_BGR2GRAY);
    } else if (processingMode == "rgb") {
        // RGB processing (no color conversion)
        processedFrame = frame.clone();
    } else {
        // Default to grayscale if unknown mode
        cv::cvtColor(frame, processedFrame, cv::COLOR_BGR2GRAY);
    }
    
    if (isFirstFrame) {
        prevFrame = processedFrame.clone();
        isFirstFrame = false;
        return result;
    }
    
    // 2. Contrast Enhancement (CLAHE)
    if (contrastEnhancement) {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(claheClipLimit, cv::Size(claheTileSize, claheTileSize));
        clahe->apply(processedFrame, processedFrame);
    }
    
    // 3. Apply blur based on blur_type
    if (blurType == "gaussian") {
        cv::GaussianBlur(processedFrame, processedFrame, cv::Size(gaussianBlurSize, gaussianBlurSize), 0);
    } else if (blurType == "median") {
        cv::medianBlur(processedFrame, processedFrame, medianBlurSize);
    } else if (blurType == "bilateral") {
        // Ensure the image is 8-bit single channel for bilateral filter
        cv::Mat bilateralInput;
        if (processedFrame.type() != CV_8UC1) {
            processedFrame.convertTo(bilateralInput, CV_8UC1);
        } else {
            bilateralInput = processedFrame;
        }
        cv::bilateralFilter(bilateralInput, processedFrame, bilateralD, bilateralSigmaColor, bilateralSigmaSpace);
    }
    // If blurType is "none", skip blurring
    
    // 4. Edge Detection (Canny)
    cv::Mat edgeMask;
    if (cannyLowThreshold > 0 && cannyHighThreshold > 0) {
        cv::Canny(processedFrame, edgeMask, cannyLowThreshold, cannyHighThreshold);
    }
    
    // 5. Background Subtraction (MOG2)
    cv::Mat bgMask;
    if (backgroundSubtraction && !bgSubtractor.empty()) {
        bgSubtractor->apply(processedFrame, bgMask);
    }
    
    // 6. Frame Difference (traditional motion detection)
    cv::Mat frameDiff;
    cv::absdiff(prevFrame, processedFrame, frameDiff);
    
    // 7. Combine different motion detection methods
    cv::Mat motionMask;
    if (backgroundSubtraction && !bgSubtractor.empty()) {
        // Use background subtraction as primary method
        motionMask = bgMask.clone();
        
        // Optionally combine with frame difference
        cv::Mat combinedMask;
        cv::bitwise_or(motionMask, frameDiff, combinedMask);
        motionMask = combinedMask;
    } else {
        // Use frame difference as primary method
        motionMask = frameDiff.clone();
    }
    
    // 8. Apply thresholding
    cv::Mat thresh;
    if (thresholdType == "adaptive") {
        cv::adaptiveThreshold(motionMask, thresh, maxThreshold, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                             cv::THRESH_BINARY, adaptiveBlockSize, adaptiveC);
    } else {
        cv::threshold(motionMask, thresh, thresholdValue, maxThreshold, cv::THRESH_BINARY);
    }
    
    // 9. Apply morphological operations
    cv::Mat processed = thresh.clone();
    if (morphology) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(morphKernelSize, morphKernelSize));
        
        // Close operation to fill gaps within objects
        if (morphClose) {
            cv::morphologyEx(processed, processed, cv::MORPH_CLOSE, kernel);
        }
        
        // Open operation to remove small noise
        if (morphOpen) {
            cv::morphologyEx(processed, processed, cv::MORPH_OPEN, kernel);
        }
        
        // Dilation to make objects more cohesive
        if (dilation) {
            cv::dilate(processed, processed, kernel, cv::Point(-1, -1), 1);
        }
        
        // Erosion to reduce object size (use with caution)
        if (erosion) {
            cv::erode(processed, processed, kernel, cv::Point(-1, -1), 1);
        }
    }
    
    // 10. Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(processed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Debug: Show contour count
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 30 == 0) { // Show every 30 frames
        Logger::getInstance()->debug("Found {} contours", contours.size());
    }
    
    // 11. Process contours with advanced filtering
    std::vector<cv::Rect> newBounds;
    int contoursPassed = 0;
    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < minContourArea) continue;
        
        // Contour approximation to reduce noise
        std::vector<cv::Point> approxContour = contour;
        if (contourApproximation) {
            double epsilon = contourEpsilonFactor * cv::arcLength(contour, true);
            cv::approxPolyDP(contour, approxContour, epsilon, true);
        }
        
        // Convex hull processing
        std::vector<cv::Point> hull;
        if (convexHull) {
            cv::convexHull(approxContour, hull);
            
            // Calculate solidity (area / convex hull area)
            double hullArea = cv::contourArea(hull);
            double solidity = (hullArea > 0) ? area / hullArea : 0;
            
            // Filter by solidity
            if (contourFiltering && solidity < minContourSolidity) continue;
            
            // Use convex hull for bounding box calculation
            cv::Rect bounds = cv::boundingRect(hull);
            
            // Filter by aspect ratio
            if (contourFiltering) {
                double aspectRatio = static_cast<double>(bounds.width) / bounds.height;
                if (aspectRatio > maxContourAspectRatio) continue;
            }
            
            result.hasMotion = true;
            newBounds.push_back(bounds);
            contoursPassed++;
        } else {
            // Use original contour
            cv::Rect bounds = cv::boundingRect(approxContour);
            
            // Filter by aspect ratio
            if (contourFiltering) {
                double aspectRatio = static_cast<double>(bounds.width) / bounds.height;
                if (aspectRatio > maxContourAspectRatio) continue;
            }
            
            result.hasMotion = true;
            newBounds.push_back(bounds);
            contoursPassed++;
        }
    }
    
    // Debug: Show how many contours passed filtering
    if (frameCount % 30 == 0) {
        Logger::getInstance()->debug("Contours passed filtering: {} (min_area={}, max_aspect={}, min_solidity={})", 
                                   contoursPassed, minContourArea, maxContourAspectRatio, minContourSolidity);
    }
    
    // Create and display split-screen visualization if enabled
    if (splitScreen) {
        cv::Mat visualization = createSplitScreenVisualization(frame, processedFrame, frameDiff, thresh, processed);
        cv::imshow(splitScreenWindowName, visualization);
    }
    
    // Update object trajectories
    updateTrajectories(newBounds);
    result.trackedObjects = trackedObjects;
    
    // Debug output - only show objects that meet minimum trajectory length
    if (!trackedObjects.empty()) {
        std::vector<TrackedObject> filteredObjects;
        for (const auto& obj : trackedObjects) {
            // Add detailed debug output right before the check
            Logger::getInstance()->debug("Checking Object {}: trajectory.size()={}, minTrajectoryLength={}. Filter condition met? {}",
                                       obj.id, obj.trajectory.size(), minTrajectoryLength, (obj.trajectory.size() >= minTrajectoryLength ? "Yes" : "No"));

            if (obj.trajectory.size() >= minTrajectoryLength) {
                filteredObjects.push_back(obj);
            }
        }
        
        if (!filteredObjects.empty()) {
            Logger::getInstance()->info("Tracking {} objects (min trajectory length: {}):", filteredObjects.size(), minTrajectoryLength);
            for (const auto& obj : filteredObjects) {
                Logger::getInstance()->info("  Object {}: confidence={}, trajectory points={}, bounds=({},{},{},{})", 
                                          obj.id, obj.confidence, obj.trajectory.size(), 
                                          obj.currentBounds.x, obj.currentBounds.y, obj.currentBounds.width, obj.currentBounds.height);
            }
        }
    } else {
        // Show when no objects are being tracked
        static int noObjectCount = 0;
        noObjectCount++;
        if (noObjectCount % 30 == 0) { // Show every 30 frames (about 1 second)
            Logger::getInstance()->info("No objects currently being tracked. Move your hands or objects in front of the camera.");
        }
    }
    
    // Update previous frame
    prevFrame = processedFrame.clone();
    
    return result;
}

const TrackedObject* MotionTracker::findTrackedObjectById(int id) const {
    for (const auto& obj : trackedObjects) {
        if (obj.id == id) {
            return &obj;
        }
    }
    return nullptr;
}

std::string MotionTracker::generateUUID() {
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