#include "motion_processor.hpp"
#include "logger.hpp"
#include <yaml-cpp/yaml.h>

MotionProcessor::MotionProcessor(const std::string& configPath) 
    : firstFrame(true),
      minContourArea(500),
      maxThreshold(255),
      
      // INPUT COLOR PROCESSING
      processingMode("grayscale"),
      
      // IMAGE PREPROCESSING
      contrastEnhancement(false),
      blurType("gaussian"),
      claheClipLimit(2.0),
      claheTileSize(8),
      gaussianBlurSize(5),
      medianBlurSize(5),
      bilateralD(15),
      bilateralSigmaColor(75.0),
      bilateralSigmaSpace(75.0),
      
      // MOTION DETECTION METHODS
      backgroundSubtraction(false),
      
      // MORPHOLOGICAL OPERATIONS
      morphology(true),
      morphKernelSize(5),
      morphClose(true),
      morphOpen(true),
      dilation(true),
      erosion(false),
      
      // CONTOUR PROCESSING
      convexHull(true),
      contourApproximation(true),
      contourFiltering(true),
      maxContourAspectRatio(2.0),
      minContourSolidity(0.85),
      contourEpsilonFactor(0.03),
      contourDetectionMode("manual") {
    loadConfig(configPath);
}

// ============================================================================
// MAIN PROCESSING PIPELINE
// ============================================================================

MotionProcessor::ProcessingResult MotionProcessor::processFrame(const cv::Mat& frame) {
    ProcessingResult result;
    
    if (frame.empty()) {
        return result;
    }
    
    // Step 1: Preprocess the frame
    result.processedFrame = preprocessFrame(frame);
    
    if (firstFrame) {
        setPrevFrame(result.processedFrame);
        firstFrame = false;
        return result;
    }
    
    // Step 2: Detect motion
    result.thresh = detectMotion(result.processedFrame, result.frameDiff, result.thresh);
    
    // Step 3: Apply morphological operations
    result.morphological = applyMorphologicalOps(result.thresh);
    
    // Step 4: Extract contours
    result.detectedBounds = extractContours(result.morphological);
    
    // Set motion detection result
    result.hasMotion = !result.detectedBounds.empty();
    
    // Update previous frame
    setPrevFrame(result.processedFrame);
    
    return result;
}

// ============================================================================
// INDIVIDUAL PROCESSING STEPS
// ============================================================================

cv::Mat MotionProcessor::preprocessFrame(const cv::Mat& frame) {
    cv::Mat processedFrame;
    if (processingMode == "grayscale") {
        cv::cvtColor(frame, processedFrame, cv::COLOR_BGR2GRAY);
    } else if (processingMode == "rgb") {
        processedFrame = frame.clone();
    } else {
        cv::cvtColor(frame, processedFrame, cv::COLOR_BGR2GRAY);
    }
    
    if (contrastEnhancement) {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(claheClipLimit, cv::Size(claheTileSize, claheTileSize));
        clahe->apply(processedFrame, processedFrame);
    }
    
    if (blurType == "gaussian") {
        cv::GaussianBlur(processedFrame, processedFrame, cv::Size(gaussianBlurSize, gaussianBlurSize), 0);
    } else if (blurType == "median") {
        cv::medianBlur(processedFrame, processedFrame, medianBlurSize);
    } else if (blurType == "bilateral") {
        cv::Mat bilateralInput;
        if (processedFrame.type() != CV_8UC1) {
            processedFrame.convertTo(bilateralInput, CV_8UC1);
        } else {
            bilateralInput = processedFrame;
        }
        cv::bilateralFilter(bilateralInput, processedFrame, bilateralD, bilateralSigmaColor, bilateralSigmaSpace);
    }
    
    return processedFrame;
}

cv::Mat MotionProcessor::detectMotion(const cv::Mat& processedFrame, cv::Mat& frameDiff, cv::Mat& thresh) {
    if (backgroundSubtraction && bgSubtractor.empty()) {
        initializeBackgroundSubtractor();
    }
    
    frameDiff = cv::Mat::zeros(processedFrame.size(), processedFrame.type());
    if (!prevFrame.empty()) {
        cv::absdiff(processedFrame, prevFrame, frameDiff);
    }
    
    cv::Mat bgMask;
    if (backgroundSubtraction && !bgSubtractor.empty()) {
        bgSubtractor->apply(processedFrame, bgMask);
    }
    
    cv::Mat motionMask;
    if (backgroundSubtraction && !bgSubtractor.empty()) {
        motionMask = bgMask.clone();
        cv::Mat combinedMask;
        cv::bitwise_or(motionMask, frameDiff, combinedMask);
        motionMask = combinedMask;
    } else {
        motionMask = frameDiff.clone();
    }
    
    // Always use Otsu thresholding
    cv::threshold(motionMask, thresh, 0, maxThreshold, cv::THRESH_BINARY | cv::THRESH_OTSU);
    
    return thresh;
}

cv::Mat MotionProcessor::applyMorphologicalOps(const cv::Mat& thresh) {
    cv::Mat processed = thresh.clone();
    if (morphology) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(morphKernelSize, morphKernelSize));
        if (morphClose) {
            cv::morphologyEx(processed, processed, cv::MORPH_CLOSE, kernel);
        }
        if (morphOpen) {
            cv::morphologyEx(processed, processed, cv::MORPH_OPEN, kernel);
        }
        if (dilation) {
            cv::dilate(processed, processed, kernel, cv::Point(-1, -1), 1);
        }
        if (erosion) {
            cv::erode(processed, processed, kernel, cv::Point(-1, -1), 1);
        }
    }
    return processed;
}

std::vector<cv::Rect> MotionProcessor::extractContours(const cv::Mat& processed) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(processed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    std::vector<cv::Rect> newBounds;
    static int frameCount = 0;
    frameCount++;
    
    // Debug: Create visualization of all found contours
    cv::Mat debugViz;
    cv::cvtColor(processed, debugViz, cv::COLOR_GRAY2BGR);
    
    // Debug counters
    int totalContours = contours.size();
    int areaFiltered = 0;
    int solidityFiltered = 0;
    int aspectRatioFiltered = 0;
    int finalAccepted = 0;
    
    // ADAPTIVE CONTOUR DETECTION
    double adaptiveMinArea = minContourArea;
    double adaptiveMinSolidity = minContourSolidity;
    double adaptiveMaxAspectRatio = maxContourAspectRatio;
    
    if (contourDetectionMode == "adaptive") {
        adaptiveMinArea = calculateAdaptiveMinArea(contours);
        adaptiveMinSolidity = calculateAdaptiveMinSolidity(contours);
        adaptiveMaxAspectRatio = calculateAdaptiveMaxAspectRatio(contours);
    } else if (contourDetectionMode == "permissive") {
        adaptiveMinArea = 50;
        adaptiveMinSolidity = 0.1;
        adaptiveMaxAspectRatio = 10.0;
    }
    
    if (frameCount % 30 == 0 || totalContours > 0) {
        LOG_INFO("=== CONTOUR EXTRACTION (Frame {}) ===", frameCount);
        LOG_INFO("Mode: {} | Area: {:.0f} | Aspect: {:.1f} | Solidity: {:.2f}", 
                 contourDetectionMode, adaptiveMinArea, adaptiveMaxAspectRatio, adaptiveMinSolidity);
        LOG_INFO("Summary: Found {} contours | Area: {} | Solidity: {} | Aspect: {} | Accepted: {}", 
                 totalContours, areaFiltered, solidityFiltered, aspectRatioFiltered, finalAccepted);
    }
    
    for (size_t i = 0; i < contours.size(); ++i) {
        const auto& contour = contours[i];
        double area = cv::contourArea(contour);
        
        // Draw all contours in red initially
        cv::drawContours(debugViz, contours, i, cv::Scalar(0, 0, 255), 2);
        
        if (area < adaptiveMinArea) {
            areaFiltered++;
            continue;
        }
        
        std::vector<cv::Point> approxContour = contour;
        if (contourApproximation) {
            double epsilon = contourEpsilonFactor * cv::arcLength(contour, true);
            cv::approxPolyDP(contour, approxContour, epsilon, true);
        }
        
        cv::Rect bounds;
        double solidity = 1.0;
        
        if (convexHull) {
            std::vector<cv::Point> hull;
            cv::convexHull(approxContour, hull);
            double hullArea = cv::contourArea(hull);
            solidity = (hullArea > 0) ? area / hullArea : 0;
            
            if (contourFiltering && solidity < adaptiveMinSolidity) {
                solidityFiltered++;
                continue;
            }
            bounds = cv::boundingRect(hull);
        } else {
            bounds = cv::boundingRect(approxContour);
        }
        
        double aspectRatio = static_cast<double>(bounds.width) / bounds.height;
        if (contourFiltering && aspectRatio > adaptiveMaxAspectRatio) {
            aspectRatioFiltered++;
            continue;
        }
        
        // This contour passed all filters
        finalAccepted++;
        newBounds.push_back(bounds);
        
        // Draw accepted contours in green
        cv::drawContours(debugViz, contours, i, cv::Scalar(0, 255, 0), 3);
        
        // Draw bounding rectangle in blue
        cv::rectangle(debugViz, bounds, cv::Scalar(255, 0, 0), 2);
        
        // Add text label with info
        std::string label = "A:" + std::to_string((int)area) + " S:" + std::to_string((int)(solidity*100)) + "%";
        cv::putText(debugViz, label, cv::Point(bounds.x, bounds.y - 5), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
    }
    
    // Save debug visualization every 10 frames or when we have interesting results
    if (frameCount % 10 == 0 || totalContours > 0) {
        std::string debugPath = "test_results/motion_processor/04_extract_contours/debug_contours_frame_" + 
                               std::to_string(frameCount) + ".jpg";
        cv::imwrite(debugPath, debugViz);
        LOG_INFO("Saved contour debug visualization to: {}", debugPath);
    }
    
    return newBounds;
}

// ============================================================================
// ADAPTIVE CONTOUR DETECTION METHODS
// ============================================================================

double MotionProcessor::calculateAdaptiveMinArea(const std::vector<std::vector<cv::Point>>& contours) {
    if (contours.empty()) return minContourArea;
    
    // Calculate areas of all contours
    std::vector<double> areas;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > 0) areas.push_back(area);
    }
    
    if (areas.empty()) return minContourArea;
    
    // Sort areas and use 10th percentile as minimum (filters out tiny noise)
    std::sort(areas.begin(), areas.end());
    size_t percentileIndex = static_cast<size_t>(areas.size() * 0.1);
    double adaptiveMin = areas[percentileIndex];
    
    // Ensure reasonable bounds (between 50 and 1000)
    adaptiveMin = std::max(50.0, std::min(1000.0, adaptiveMin));
    
    return adaptiveMin;
}

double MotionProcessor::calculateAdaptiveMinSolidity(const std::vector<std::vector<cv::Point>>& contours) {
    if (contours.empty()) return minContourSolidity;
    
    // Calculate solidity of all reasonable-sized contours
    std::vector<double> solidities;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < 100) continue; // Skip tiny contours
        
        std::vector<cv::Point> hull;
        cv::convexHull(contour, hull);
        double hullArea = cv::contourArea(hull);
        
        if (hullArea > 0) {
            double solidity = area / hullArea;
            solidities.push_back(solidity);
        }
    }
    
    if (solidities.empty()) return minContourSolidity;
    
    // Use 25th percentile as minimum (allows some irregular shapes)
    std::sort(solidities.begin(), solidities.end());
    size_t percentileIndex = static_cast<size_t>(solidities.size() * 0.25);
    double adaptiveMin = solidities[percentileIndex];
    
    // Ensure reasonable bounds (between 0.2 and 0.8)
    adaptiveMin = std::max(0.2, std::min(0.8, adaptiveMin));
    
    return adaptiveMin;
}

double MotionProcessor::calculateAdaptiveMaxAspectRatio(const std::vector<std::vector<cv::Point>>& contours) {
    if (contours.empty()) return maxContourAspectRatio;
    
    // Calculate aspect ratios of all reasonable-sized contours
    std::vector<double> aspectRatios;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < 100) continue; // Skip tiny contours
        
        cv::Rect bounds = cv::boundingRect(contour);
        if (bounds.width > 0 && bounds.height > 0) {
            double aspectRatio = static_cast<double>(bounds.width) / bounds.height;
            aspectRatios.push_back(aspectRatio);
        }
    }
    
    if (aspectRatios.empty()) return maxContourAspectRatio;
    
    // Use 90th percentile as maximum (allows most shapes but filters extreme outliers)
    std::sort(aspectRatios.begin(), aspectRatios.end());
    size_t percentileIndex = static_cast<size_t>(aspectRatios.size() * 0.9);
    double adaptiveMax = aspectRatios[percentileIndex];
    
    // Ensure reasonable bounds (between 2.0 and 15.0)
    adaptiveMax = std::max(2.0, std::min(15.0, adaptiveMax));
    
    return adaptiveMax;
}

void MotionProcessor::setPrevFrame(const cv::Mat& frame) {
    prevFrame = frame.clone();
}

// ============================================================================
// CONFIGURATION AND SETUP
// ============================================================================

void MotionProcessor::loadConfig(const std::string& configPath) {
    try {
        YAML::Node config = YAML::LoadFile(configPath);

        if (config["min_contour_area"]) minContourArea = config["min_contour_area"].as<int>();
        if (config["max_threshold"]) maxThreshold = config["max_threshold"].as<int>();
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

        // Processing mode
        if (config["processing_mode"]) processingMode = config["processing_mode"].as<std::string>();
        
        // Background Subtraction
        if (config["enable_background_subtraction"]) backgroundSubtraction = config["enable_background_subtraction"].as<bool>();
        
        // Convex Hull parameters
        if (config["convex_hull"]) convexHull = config["convex_hull"].as<bool>();
        

        
        // Contour Processing parameters
        if (config["contour_approximation"]) contourApproximation = config["contour_approximation"].as<bool>();
        if (config["contour_epsilon_factor"]) contourEpsilonFactor = config["contour_epsilon_factor"].as<double>();
        if (config["contour_filtering"]) contourFiltering = config["contour_filtering"].as<bool>();
        if (config["max_contour_aspect_ratio"]) maxContourAspectRatio = config["max_contour_aspect_ratio"].as<double>();
        if (config["min_contour_solidity"]) minContourSolidity = config["min_contour_solidity"].as<double>();
        if (config["contour_detection_mode"]) contourDetectionMode = config["contour_detection_mode"].as<std::string>();
        
        LOG_INFO("MotionProcessor config loaded: min_contour_area={}, background_subtraction={}", 
                 minContourArea, backgroundSubtraction);
        
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Warning: Could not load config file: {}. Error: {}", configPath, e.what());
    }
}

void MotionProcessor::initializeBackgroundSubtractor() {
    if (backgroundSubtraction) {
        bgSubtractor = cv::createBackgroundSubtractorMOG2();
        LOG_INFO("Using Background Subtraction (MOG2)");
    }
}
