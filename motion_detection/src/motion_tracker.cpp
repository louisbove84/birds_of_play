#include "motion_tracker.hpp"
#include <iostream>
#include <cmath>

MotionTracker::MotionTracker(const std::string& configPath) 
    : isRunning(false),
      isFirstFrame(true),
      nextObjectId(0),
      maxTrajectoryPoints(30),
      thresholdValue(25.0),
      minContourArea(500),
      maxTrackingDistance(100.0),
      maxThreshold(255),
      smoothingFactor(0.6),
      minTrackingConfidence(0.5),
      enableGaussianBlur(true),
      gaussianBlurSize(5),
      enableMorphology(true),
      morphologyKernelSize(5),
      enableMorphClose(true),
      enableMorphOpen(true),
      enableDilation(true),
      enableErosion(false),
      enableContrastEnhancement(false),
      claheClipLimit(2.0),
      claheTileSize(8),
      enableMedianBlur(false),
      medianBlurSize(5),
      enableBilateralFilter(false),
      bilateralD(15),
      bilateralSigmaColor(75.0),
      bilateralSigmaSpace(75.0),
      enableAdaptiveThreshold(false),
      adaptiveBlockSize(11),
      adaptiveC(2),
      enableBackgroundSubtraction(true),
      backgroundHistory(500),
      backgroundThreshold(16.0),
      backgroundDetectShadows(true),
      enableConvexHull(true),
      convexHullFill(true),
      enableHsvFiltering(false),
      hsvLower(0, 30, 60),
      hsvUpper(20, 150, 255),
      enableEdgeDetection(false),
      cannyLowThreshold(50),
      cannyHighThreshold(150),
      enableContourApproximation(true),
      contourEpsilonFactor(0.02),
      enableContourFiltering(true),
      minAspectRatio(0.3),
      maxAspectRatio(3.0),
      minSolidity(0.5),
      enableSplitScreen(true),
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
    isRunning = true;
    return true;
}

bool MotionTracker::initialize(int deviceIndex) {
    cap.open(deviceIndex);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video device index " << deviceIndex << std::endl;
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
        if (config["max_threshold"]) maxThreshold = config["max_threshold"].as<int>();
        if (config["smoothing_factor"]) smoothingFactor = config["smoothing_factor"].as<double>();
        if (config["min_tracking_confidence"]) minTrackingConfidence = config["min_tracking_confidence"].as<double>();
        if (config["gaussian_blur_size"]) gaussianBlurSize = config["gaussian_blur_size"].as<int>();
        if (config["morphology_kernel_size"]) morphologyKernelSize = config["morphology_kernel_size"].as<int>();
        if (config["enable_morphology"]) enableMorphology = config["enable_morphology"].as<bool>();
        if (config["enable_dilation"]) enableDilation = config["enable_dilation"].as<bool>();
        if (config["enable_morph_close"]) enableMorphClose = config["enable_morph_close"].as<bool>();
        if (config["enable_morph_open"]) enableMorphOpen = config["enable_morph_open"].as<bool>();
        if (config["enable_erosion"]) enableErosion = config["enable_erosion"].as<bool>();
        if (config["enable_contrast_enhancement"]) enableContrastEnhancement = config["enable_contrast_enhancement"].as<bool>();
        if (config["clahe_clip_limit"]) claheClipLimit = config["clahe_clip_limit"].as<double>();
        if (config["clahe_tile_size"]) claheTileSize = config["clahe_tile_size"].as<int>();
        if (config["enable_median_blur"]) enableMedianBlur = config["enable_median_blur"].as<bool>();
        if (config["median_blur_size"]) medianBlurSize = config["median_blur_size"].as<int>();
        if (config["enable_bilateral_filter"]) enableBilateralFilter = config["enable_bilateral_filter"].as<bool>();
        if (config["bilateral_d"]) bilateralD = config["bilateral_d"].as<int>();
        if (config["bilateral_sigma_color"]) bilateralSigmaColor = config["bilateral_sigma_color"].as<double>();
        if (config["bilateral_sigma_space"]) bilateralSigmaSpace = config["bilateral_sigma_space"].as<double>();
        if (config["enable_adaptive_threshold"]) enableAdaptiveThreshold = config["enable_adaptive_threshold"].as<bool>();
        if (config["adaptive_block_size"]) adaptiveBlockSize = config["adaptive_block_size"].as<int>();
        if (config["adaptive_c"]) adaptiveC = config["adaptive_c"].as<int>();
        
        // Visualization parameters
        if (config["enable_split_screen"]) enableSplitScreen = config["enable_split_screen"].as<bool>();
        if (config["split_screen_window_name"]) splitScreenWindowName = config["split_screen_window_name"].as<std::string>();
        
        // Background Subtraction parameters
        if (config["enable_background_subtraction"]) enableBackgroundSubtraction = config["enable_background_subtraction"].as<bool>();
        if (config["background_history"]) backgroundHistory = config["background_history"].as<int>();
        if (config["background_threshold"]) backgroundThreshold = config["background_threshold"].as<double>();
        if (config["background_detect_shadows"]) backgroundDetectShadows = config["background_detect_shadows"].as<bool>();
        
        // Convex Hull parameters
        if (config["enable_convex_hull"]) enableConvexHull = config["enable_convex_hull"].as<bool>();
        if (config["convex_hull_fill"]) convexHullFill = config["convex_hull_fill"].as<bool>();
        
        // HSV Color Filtering parameters
        if (config["enable_hsv_filtering"]) enableHsvFiltering = config["enable_hsv_filtering"].as<bool>();
        if (config["hsv_lower_h"]) hsvLower[0] = config["hsv_lower_h"].as<int>();
        if (config["hsv_lower_s"]) hsvLower[1] = config["hsv_lower_s"].as<int>();
        if (config["hsv_lower_v"]) hsvLower[2] = config["hsv_lower_v"].as<int>();
        if (config["hsv_upper_h"]) hsvUpper[0] = config["hsv_upper_h"].as<int>();
        if (config["hsv_upper_s"]) hsvUpper[1] = config["hsv_upper_s"].as<int>();
        if (config["hsv_upper_v"]) hsvUpper[2] = config["hsv_upper_v"].as<int>();
        
        // Edge Detection parameters
        if (config["enable_edge_detection"]) enableEdgeDetection = config["enable_edge_detection"].as<bool>();
        if (config["canny_low_threshold"]) cannyLowThreshold = config["canny_low_threshold"].as<int>();
        if (config["canny_high_threshold"]) cannyHighThreshold = config["canny_high_threshold"].as<int>();
        
        // Contour Processing parameters
        if (config["enable_contour_approximation"]) enableContourApproximation = config["enable_contour_approximation"].as<bool>();
        if (config["contour_epsilon_factor"]) contourEpsilonFactor = config["contour_epsilon_factor"].as<double>();
        if (config["enable_contour_filtering"]) enableContourFiltering = config["enable_contour_filtering"].as<bool>();
        if (config["min_aspect_ratio"]) minAspectRatio = config["min_aspect_ratio"].as<double>();
        if (config["max_aspect_ratio"]) maxAspectRatio = config["max_aspect_ratio"].as<double>();
        if (config["min_solidity"]) minSolidity = config["min_solidity"].as<double>();
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not load config file: " << e.what() << ". Using defaults." << std::endl;
    }
}

TrackedObject* MotionTracker::findNearestObject(const cv::Rect& newBounds) {
    cv::Point newCenter(newBounds.x + newBounds.width/2, 
                       newBounds.y + newBounds.height/2);
    
    TrackedObject* nearest = nullptr;
    double minDistance = maxTrackingDistance;
    
    for (auto& obj : trackedObjects) {
        cv::Point objCenter = obj.getCenter();
        double distance = std::sqrt(std::pow(newCenter.x - objCenter.x, 2) + 
                                  std::pow(newCenter.y - objCenter.y, 2));
        
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
                cv::Point newCenter = matchedObj->getCenter();
                
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
            TrackedObject newObj;
            newObj.id = nextObjectId++;
            newObj.currentBounds = bounds;
            newObj.smoothedCenter = newObj.getCenter();
            newObj.confidence = 0.5;  // Initial confidence
            newObj.trajectory.push_back(newObj.smoothedCenter);
            trackedObjects.push_back(newObj);
        }
    }
    
    // Remove objects that weren't matched or have low confidence
    for (int i = trackedObjects.size() - 1; i >= 0; --i) {
        if (i < static_cast<int>(objectMatched.size()) && 
            (!objectMatched[i] || trackedObjects[i].confidence < minTrackingConfidence)) {
            trackedObjects.erase(trackedObjects.begin() + i);
        }
    }
}

cv::Mat MotionTracker::createSplitScreenVisualization(const cv::Mat& originalFrame, const cv::Mat& processedFrame, 
                                                      const cv::Mat& frameDiff, const cv::Mat& thresholded, 
                                                      const cv::Mat& finalProcessed) {
    // Count how many views we need to show
    int numViews = 1; // Always show original
    std::vector<std::string> viewNames = {"Original"};
    std::vector<cv::Mat> viewFrames = {originalFrame};
    
    // Add HSV mask if enabled
    if (enableHsvFiltering) {
        numViews++;
        viewNames.push_back("HSV Mask");
        cv::Mat hsvFrame, hsvMask;
        cv::cvtColor(originalFrame, hsvFrame, cv::COLOR_BGR2HSV);
        cv::inRange(hsvFrame, hsvLower, hsvUpper, hsvMask);
        cv::Mat hsvColor;
        cv::cvtColor(hsvMask, hsvColor, cv::COLOR_GRAY2BGR);
        viewFrames.push_back(hsvColor);
    }
    
    // Add processed frame if any processing was applied
    bool anyProcessing = enableContrastEnhancement || enableGaussianBlur || enableMedianBlur || enableBilateralFilter;
    if (anyProcessing) {
        numViews++;
        viewNames.push_back("Processed");
        cv::Mat processedColor;
        cv::cvtColor(processedFrame, processedColor, cv::COLOR_GRAY2BGR);
        viewFrames.push_back(processedColor);
    }
    
    // Add edge detection if enabled
    if (enableEdgeDetection) {
        numViews++;
        viewNames.push_back("Edges");
        cv::Mat edgeMask;
        cv::Canny(processedFrame, edgeMask, cannyLowThreshold, cannyHighThreshold);
        cv::Mat edgeColor;
        cv::cvtColor(edgeMask, edgeColor, cv::COLOR_GRAY2BGR);
        viewFrames.push_back(edgeColor);
    }
    
    // Add background subtraction if enabled
    if (enableBackgroundSubtraction && !bgSubtractor.empty()) {
        numViews++;
        viewNames.push_back("BG Subtract");
        cv::Mat bgMask;
        bgSubtractor->apply(processedFrame, bgMask);
        cv::Mat bgColor;
        cv::cvtColor(bgMask, bgColor, cv::COLOR_GRAY2BGR);
        viewFrames.push_back(bgColor);
    }
    
    // Always show frame difference
    numViews++;
    viewNames.push_back("Frame Diff");
    cv::Mat diffColor;
    cv::cvtColor(frameDiff, diffColor, cv::COLOR_GRAY2BGR);
    viewFrames.push_back(diffColor);
    
    // Always show thresholded
    numViews++;
    viewNames.push_back("Thresholded");
    cv::Mat threshColor;
    cv::cvtColor(thresholded, threshColor, cv::COLOR_GRAY2BGR);
    viewFrames.push_back(threshColor);
    
    // Add final processed if morphology was applied
    if (enableMorphology) {
        numViews++;
        viewNames.push_back("Morphology");
        cv::Mat finalColor;
        cv::cvtColor(finalProcessed, finalColor, cv::COLOR_GRAY2BGR);
        viewFrames.push_back(finalColor);
    }
    
    // Calculate layout (try to make it roughly square)
    int cols = std::ceil(std::sqrt(numViews));
    int rows = std::ceil(static_cast<double>(numViews) / cols);
    
    // Calculate individual view size
    int viewWidth = originalFrame.cols / cols;
    int viewHeight = originalFrame.rows / rows;
    
    // Create the combined visualization
    cv::Mat visualization(rows * viewHeight, cols * viewWidth, CV_8UC3, cv::Scalar(0, 0, 0));
    
    // Place each view in the grid
    for (int i = 0; i < numViews; ++i) {
        int row = i / cols;
        int col = i % cols;
        
        // Resize the frame to fit in the grid
        cv::Mat resizedFrame;
        cv::resize(viewFrames[i], resizedFrame, cv::Size(viewWidth, viewHeight));
        
        // Copy to the correct position
        resizedFrame.copyTo(visualization(cv::Rect(col * viewWidth, row * viewHeight, viewWidth, viewHeight)));
        
        // Add label
        cv::putText(visualization, viewNames[i], 
                    cv::Point(col * viewWidth + 10, row * viewHeight + 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    }
    
    return visualization;
}

void MotionTracker::initializeBackgroundSubtractor() {
    if (enableBackgroundSubtraction) {
        bgSubtractor = cv::createBackgroundSubtractorMOG2(
            backgroundHistory, 
            backgroundThreshold, 
            backgroundDetectShadows
        );
    }
}

MotionResult MotionTracker::processFrame(const cv::Mat& frame) {
    MotionResult result;
    result.hasMotion = false;
    
    if (frame.empty()) {
        return result;
    }
    
    // Initialize background subtractor if needed
    if (enableBackgroundSubtraction && bgSubtractor.empty()) {
        initializeBackgroundSubtractor();
    }
    
    // Convert frame to grayscale for processing
    cv::Mat grayFrame;
    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
    
    if (isFirstFrame) {
        prevFrame = grayFrame.clone();
        isFirstFrame = false;
        return result;
    }
    
    // Start with the original frame for processing
    cv::Mat processedFrame = grayFrame.clone();
    
    // 1. HSV Color Filtering (if enabled, apply to original frame)
    cv::Mat hsvMask;
    if (enableHsvFiltering) {
        cv::Mat hsvFrame;
        cv::cvtColor(frame, hsvFrame, cv::COLOR_BGR2HSV);
        cv::inRange(hsvFrame, hsvLower, hsvUpper, hsvMask);
        // Apply HSV mask to grayscale frame
        cv::bitwise_and(processedFrame, hsvMask, processedFrame);
    }
    
    // 2. Contrast Enhancement (CLAHE)
    if (enableContrastEnhancement) {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(claheClipLimit, cv::Size(claheTileSize, claheTileSize));
        clahe->apply(processedFrame, processedFrame);
    }
    
    // 3. Gaussian Blur
    if (enableGaussianBlur) {
        cv::GaussianBlur(processedFrame, processedFrame, cv::Size(gaussianBlurSize, gaussianBlurSize), 0);
    }
    
    // 4. Median Blur (alternative to Gaussian)
    if (enableMedianBlur) {
        cv::medianBlur(processedFrame, processedFrame, medianBlurSize);
    }
    
    // 5. Bilateral Filter (edge-preserving smoothing)
    if (enableBilateralFilter) {
        cv::bilateralFilter(processedFrame, processedFrame, bilateralD, bilateralSigmaColor, bilateralSigmaSpace);
    }
    
    // 6. Edge Detection (Canny)
    cv::Mat edgeMask;
    if (enableEdgeDetection) {
        cv::Canny(processedFrame, edgeMask, cannyLowThreshold, cannyHighThreshold);
    }
    
    // 7. Background Subtraction (MOG2)
    cv::Mat bgMask;
    if (enableBackgroundSubtraction && !bgSubtractor.empty()) {
        bgSubtractor->apply(processedFrame, bgMask);
    }
    
    // 8. Frame Difference (traditional motion detection)
    cv::Mat frameDiff;
    cv::absdiff(prevFrame, processedFrame, frameDiff);
    
    // 9. Combine different motion detection methods
    cv::Mat motionMask;
    if (enableBackgroundSubtraction && !bgSubtractor.empty()) {
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
    
    // 10. Apply thresholding
    cv::Mat thresh;
    if (enableAdaptiveThreshold) {
        cv::adaptiveThreshold(motionMask, thresh, maxThreshold, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                             cv::THRESH_BINARY, adaptiveBlockSize, adaptiveC);
    } else {
        cv::threshold(motionMask, thresh, thresholdValue, maxThreshold, cv::THRESH_BINARY);
    }
    
    // 11. Apply morphological operations
    cv::Mat processed = thresh.clone();
    if (enableMorphology) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(morphologyKernelSize, morphologyKernelSize));
        
        // Close operation to fill gaps within objects
        if (enableMorphClose) {
            cv::morphologyEx(processed, processed, cv::MORPH_CLOSE, kernel);
        }
        
        // Open operation to remove small noise
        if (enableMorphOpen) {
            cv::morphologyEx(processed, processed, cv::MORPH_OPEN, kernel);
        }
        
        // Dilation to make objects more cohesive
        if (enableDilation) {
            cv::dilate(processed, processed, kernel, cv::Point(-1, -1), 1);
        }
        
        // Erosion to reduce object size (use with caution)
        if (enableErosion) {
            cv::erode(processed, processed, kernel, cv::Point(-1, -1), 1);
        }
    }
    
    // 12. Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(processed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // 13. Process contours with advanced filtering
    std::vector<cv::Rect> newBounds;
    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < minContourArea) continue;
        
        // Contour approximation to reduce noise
        std::vector<cv::Point> approxContour = contour;
        if (enableContourApproximation) {
            double epsilon = contourEpsilonFactor * cv::arcLength(contour, true);
            cv::approxPolyDP(contour, approxContour, epsilon, true);
        }
        
        // Convex hull processing
        std::vector<cv::Point> hull;
        if (enableConvexHull) {
            cv::convexHull(approxContour, hull);
            
            // Calculate solidity (area / convex hull area)
            double hullArea = cv::contourArea(hull);
            double solidity = (hullArea > 0) ? area / hullArea : 0;
            
            // Filter by solidity
            if (enableContourFiltering && solidity < minSolidity) continue;
            
            // Use convex hull for bounding box calculation
            cv::Rect bounds = cv::boundingRect(hull);
            
            // Filter by aspect ratio
            if (enableContourFiltering) {
                double aspectRatio = static_cast<double>(bounds.width) / bounds.height;
                if (aspectRatio < minAspectRatio || aspectRatio > maxAspectRatio) continue;
            }
            
            result.hasMotion = true;
            newBounds.push_back(bounds);
        } else {
            // Use original contour
            cv::Rect bounds = cv::boundingRect(approxContour);
            
            // Filter by aspect ratio
            if (enableContourFiltering) {
                double aspectRatio = static_cast<double>(bounds.width) / bounds.height;
                if (aspectRatio < minAspectRatio || aspectRatio > maxAspectRatio) continue;
            }
            
            result.hasMotion = true;
            newBounds.push_back(bounds);
        }
    }
    
    // Create and display split-screen visualization if enabled
    if (enableSplitScreen) {
        cv::Mat visualization = createSplitScreenVisualization(frame, processedFrame, frameDiff, thresh, processed);
        cv::imshow(splitScreenWindowName, visualization);
    }
    
    // Update object trajectories
    updateTrajectories(newBounds);
    result.trackedObjects = trackedObjects;
    
    // Debug output
    if (!trackedObjects.empty()) {
        std::cout << "Tracking " << trackedObjects.size() << " objects:" << std::endl;
        for (const auto& obj : trackedObjects) {
            std::cout << "  Object " << obj.id << ": confidence=" << obj.confidence 
                      << ", trajectory points=" << obj.trajectory.size() 
                      << ", bounds=(" << obj.currentBounds.x << "," << obj.currentBounds.y 
                      << "," << obj.currentBounds.width << "," << obj.currentBounds.height << ")" << std::endl;
        }
    }
    
    // Update previous frame
    prevFrame = processedFrame.clone();
    
    return result;
} 