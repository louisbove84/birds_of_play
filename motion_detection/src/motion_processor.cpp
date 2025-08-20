#include "motion_processor.hpp"
#include "logger.hpp"
#include <yaml-cpp/yaml.h>

MotionProcessor::MotionProcessor(const std::string& configPath) 
    : firstFrame(true),
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
      contourEpsilonFactor(0.03),
      contourDetectionMode("adaptive"),
      
      // Permissive mode defaults (will be overridden by config)
      permissiveMinArea(50),
      permissiveMinSolidity(0.1),
      permissiveMaxAspectRatio(10.0),
      
      // Adaptive calculation cache
      adaptiveUpdateInterval(150),  // recalculate every 150 frames (5 seconds at 30fps)
      lastAdaptiveUpdate(0),
      cachedAdaptiveMinArea(minContourArea),
      cachedAdaptiveMinSolidity(minContourSolidity),
      cachedAdaptiveMaxAspectRatio(maxContourAspectRatio) {
    loadConfig(configPath);
}

// ============================================================================
// MAIN PROCESSING PIPELINE
// ============================================================================

/**
 * Main processing pipeline for motion detection. Takes a raw frame and:
 * 1. Preprocesses it (grayscale, blur, contrast)
 * 2. Detects motion using frame differencing or background subtraction
 * 3. Cleans up the motion mask using morphological operations
 * 4. Finds and filters motion regions using contour detection
 * 
 * @param frame Raw input frame from camera/video
 * @return ProcessingResult containing all intermediate steps and final detections
 */
MotionProcessor::ProcessingResult MotionProcessor::processFrame(const cv::Mat& frame) {
    ProcessingResult result;
    
    // Safety check for valid input
    if (frame.empty()) {
        return result;
    }
    
    // Step 1: Preprocess the frame
    // Convert to grayscale, apply blur for noise reduction,
    // and optionally enhance contrast
    result.processedFrame = preprocessFrame(frame);
    
    // Special handling for the first frame:
    // Just store it as reference and wait for next frame
    if (firstFrame) {
        setPrevFrame(result.processedFrame);
        firstFrame = false;
        return result;
    }
    
    // Step 2: Detect motion
    // Either using frame differencing (comparing to previous frame)
    // or background subtraction (comparing to learned background)
    // Returns a binary mask where white pixels indicate motion
    result.thresh = detectMotion(result.processedFrame, result.frameDiff, result.thresh);
    
    // Step 3: Clean up the motion mask
    // Uses morphological operations to:
    // - Fill small holes (close)
    // - Remove noise (open)
    // - Connect nearby regions (dilate)
    // - Shrink expanded regions (erode)
    result.morphological = applyMorphologicalOps(result.thresh);
    
    // Step 4: Find motion regions
    // Detects contours in the cleaned mask and filters them based on:
    // - Size (remove tiny regions)
    // - Shape (remove irregular shapes)
    // - Aspect ratio (remove too elongated regions)
    // Uses either adaptive or permissive thresholds
    result.detectedBounds = extractContours(result.morphological);
    
    // Update motion detection status
    result.hasMotion = !result.detectedBounds.empty();
    
    // Store current frame for next comparison
    setPrevFrame(result.processedFrame);
    
    return result;
}

// ============================================================================
// INDIVIDUAL PROCESSING STEPS
// ============================================================================

/**
 * Prepares the input frame for motion detection by:
 * 1. Converting to appropriate color space (usually grayscale)
 * 2. Optionally enhancing contrast using CLAHE
 * 3. Applying blur to reduce noise
 * 
 * Each step is configurable via the config file:
 * - processingMode: Color space conversion
 * - contrastEnhancement: Whether to use CLAHE
 * - blurType: Type of blur (gaussian/median/bilateral)
 */
cv::Mat MotionProcessor::preprocessFrame(const cv::Mat& frame) {
    cv::Mat processedFrame;
    
    // Step 1: Color Space Conversion
    // Usually convert to grayscale for motion detection
    // RGB mode is available for color-based detection
    if (processingMode == "grayscale") {
        cv::cvtColor(frame, processedFrame, cv::COLOR_BGR2GRAY);
    } else if (processingMode == "rgb") {
        processedFrame = frame.clone();
    } else {
        cv::cvtColor(frame, processedFrame, cv::COLOR_BGR2GRAY);
    }
    
    // Step 2: Contrast Enhancement (optional)
    // Uses CLAHE (Contrast Limited Adaptive Histogram Equalization)
    // Helps with:
    // - Low contrast scenes
    // - Varying lighting conditions
    // - Shadow regions
    if (contrastEnhancement) {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(claheClipLimit, cv::Size(claheTileSize, claheTileSize));
        clahe->apply(processedFrame, processedFrame);
    }
    
    // Step 3: Noise Reduction
    // Three blur options:
    // - Gaussian: General purpose, balanced blur
    // - Median: Better for salt-and-pepper noise
    // - Bilateral: Edge-preserving blur
    if (blurType == "gaussian") {
        cv::GaussianBlur(processedFrame, processedFrame, cv::Size(gaussianBlurSize, gaussianBlurSize), 0);
    } else if (blurType == "median") {
        cv::medianBlur(processedFrame, processedFrame, medianBlurSize);
    } else if (blurType == "bilateral") {
        // Bilateral filter requires 8-bit input
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

/**
 * Detects motion by comparing the current frame with either:
 * 1. The previous frame (frame differencing)
 * 2. A learned background model (background subtraction)
 * 
 * The comparison produces a binary mask where white pixels indicate motion.
 * Uses Otsu's method for automatic threshold selection.
 */
cv::Mat MotionProcessor::detectMotion(const cv::Mat& processedFrame, cv::Mat& frameDiff, cv::Mat& thresh) {
    // Initialize background subtractor if needed
    if (backgroundSubtraction && bgSubtractor.empty()) {
        initializeBackgroundSubtractor();
    }
    
    // Step 1: Frame Differencing
    // Compare current frame with previous frame
    // White pixels show where the frames differ (motion)
    frameDiff = cv::Mat::zeros(processedFrame.size(), processedFrame.type());
    if (!prevFrame.empty()) {
        cv::absdiff(processedFrame, prevFrame, frameDiff);
    }
    
    // Step 2: Background Subtraction (optional)
    // Compare current frame with learned background model
    // Better for:
    // - Slow moving objects
    // - Removing dynamic backgrounds
    // - Continuous motion
    cv::Mat bgMask;
    if (backgroundSubtraction && !bgSubtractor.empty()) {
        bgSubtractor->apply(processedFrame, bgMask);
    }
    
    // Step 3: Combine Detection Methods
    // If using background subtraction, combine it with frame diff
    // This helps catch both:
    // - Sudden movements (frame diff)
    // - Slow movements (background subtraction)
    cv::Mat motionMask;
    if (backgroundSubtraction && !bgSubtractor.empty()) {
        motionMask = bgMask.clone();
        cv::Mat combinedMask;
        cv::bitwise_or(motionMask, frameDiff, combinedMask);
        motionMask = combinedMask;
    } else {
        motionMask = frameDiff.clone();
    }
    
    // Step 4: Threshold Selection
    // Use Otsu's method to automatically find the best threshold
    // This adapts to varying lighting and motion conditions
    cv::threshold(motionMask, thresh, 0, maxThreshold, cv::THRESH_BINARY | cv::THRESH_OTSU);
    
    return thresh;
}

/**
 * Cleans up the motion mask using morphological operations.
 * Operations are applied in this order:
 * 1. Close - Fills small holes in motion regions
 * 2. Open - Removes small noise blobs
 * 3. Dilate - Expands motion regions to connect nearby areas
 * 4. Erode - Optional shrinking to counter over-expansion
 * 
 * Uses an elliptical kernel for more natural shape preservation.
 * All operations are configurable via the config file.
 */
cv::Mat MotionProcessor::applyMorphologicalOps(const cv::Mat& thresh) {
    cv::Mat processed = thresh.clone();
    
    if (morphology) {
        // Create an elliptical kernel for all operations
        // Ellipse shape better matches natural motion shapes
        // Size affects how aggressive each operation is
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(morphKernelSize, morphKernelSize));
        
        // Step 1: Close Operation (optional)
        // Fills small holes within motion regions
        // Useful for:
        // - Connecting broken contours
        // - Filling internal gaps
        // - Making shapes more solid
        if (morphClose) {
            cv::morphologyEx(processed, processed, cv::MORPH_CLOSE, kernel);
        }
        
        // Step 2: Open Operation (optional)
        // Removes small noise blobs
        // Useful for:
        // - Eliminating small false detections
        // - Smoothing object boundaries
        // - Removing thin connections
        if (morphOpen) {
            cv::morphologyEx(processed, processed, cv::MORPH_OPEN, kernel);
        }
        
        // Step 3: Dilation (optional)
        // Expands motion regions
        // Useful for:
        // - Connecting nearby motion regions
        // - Compensating for under-detection
        // - Making objects more visible
        if (dilation) {
            cv::dilate(processed, processed, kernel, cv::Point(-1, -1), 1);
        }
        
        // Step 4: Erosion (optional)
        // Shrinks motion regions
        // Useful for:
        // - Countering over-expansion from dilation
        // - Separating barely-connected regions
        // - Reducing false detections
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
    
    // Debug visualization (only if enabled)
    cv::Mat debugViz;
    if (visualizationEnabled) {
        cv::cvtColor(processed, debugViz, cv::COLOR_GRAY2BGR);
    }
    
    // Debug counters
    int totalContours = contours.size();
    int areaFiltered = 0;
    int solidityFiltered = 0;
    int aspectRatioFiltered = 0;
    int finalAccepted = 0;
    
    // Initialize with permissive values as fallback
    double adaptiveMinArea = permissiveMinArea;
    double adaptiveMinSolidity = permissiveMinSolidity;
    double adaptiveMaxAspectRatio = permissiveMaxAspectRatio;
    
    if (contourDetectionMode == "adaptive") {
        // Only recalculate adaptive values periodically
        if (frameCount - lastAdaptiveUpdate >= adaptiveUpdateInterval) {
            cachedAdaptiveMinArea = calculateAdaptiveMinArea(contours);
            cachedAdaptiveMinSolidity = calculateAdaptiveMinSolidity(contours);
            cachedAdaptiveMaxAspectRatio = calculateAdaptiveMaxAspectRatio(contours);
            lastAdaptiveUpdate = frameCount;
            LOG_INFO("Updated adaptive values at frame {}", frameCount);
        }
        adaptiveMinArea = cachedAdaptiveMinArea;
        adaptiveMinSolidity = cachedAdaptiveMinSolidity;
        adaptiveMaxAspectRatio = cachedAdaptiveMaxAspectRatio;
    } else if (contourDetectionMode == "permissive") {
        adaptiveMinArea = permissiveMinArea;
        adaptiveMinSolidity = permissiveMinSolidity;
        adaptiveMaxAspectRatio = permissiveMaxAspectRatio;
    }
    
    if (frameCount % 30 == 0 || totalContours > 0) {
        LOG_INFO("=== CONTOUR EXTRACTION (Frame {}) ===", frameCount);
        LOG_INFO("Mode: {} | Area: {:.0f} | Aspect: {:.1f} | Solidity: {:.2f}", 
                 contourDetectionMode, adaptiveMinArea, adaptiveMaxAspectRatio, adaptiveMinSolidity);
        LOG_INFO("Summary: Found {} contours | Area: {} | Solidity: {} | Aspect: {} | Accepted: {}", 
                 totalContours, areaFiltered, solidityFiltered, aspectRatioFiltered, finalAccepted);
    }
    
    // Process each detected contour through a series of filters
    // Each filter helps eliminate false detections while keeping real motion
    for (size_t i = 0; i < contours.size(); ++i) {
        const auto& contour = contours[i];
        
        // Step 1: Area Filter
        // Calculate contour area and filter out tiny regions
        // Area = number of pixels in the region
        double area = cv::contourArea(contour);
        
        // Visualize if enabled: Draw all contours in red initially
        if (visualizationEnabled) {
            cv::drawContours(debugViz, contours, i, cv::Scalar(0, 0, 255), 2);
        }
        
        // Filter out small regions that are likely noise
        // Uses either adaptive or permissive threshold
        if (area < adaptiveMinArea) {
            areaFiltered++;
            continue;
        }
        
        // Step 2: Shape Simplification (optional)
        // Approximate the contour with fewer points
        // Helps smooth out noisy edges while keeping shape
        std::vector<cv::Point> approxContour = contour;
        if (contourApproximation) {
            // epsilon = how far points can deviate from original contour
            // Larger epsilon = more simplification
            double epsilon = contourEpsilonFactor * cv::arcLength(contour, true);
            cv::approxPolyDP(contour, approxContour, epsilon, true);
        }
        
        // Step 3: Convex Hull Analysis (optional)
        // Checks how well the shape fills its convex hull
        // Helps identify solid vs scattered motion regions
        cv::Rect bounds;
        double solidity = 1.0;  // Perfect solidity by default
        
        if (convexHull) {
            // Calculate convex hull (smallest convex shape containing contour)
            std::vector<cv::Point> hull;
            cv::convexHull(approxContour, hull);
            double hullArea = cv::contourArea(hull);
            
            // Solidity = contour area / hull area
            // 1.0 = perfectly solid, < 1.0 = more scattered
            solidity = (hullArea > 0) ? area / hullArea : 0;
            
            // Filter out scattered/irregular shapes
            if (contourFiltering && solidity < adaptiveMinSolidity) {
                solidityFiltered++;
                continue;  // Skip to next contour
            }
            bounds = cv::boundingRect(hull);
        } else {
            bounds = cv::boundingRect(approxContour);
        }
        
        // Step 4: Aspect Ratio Filter
        // Check if the shape is too elongated
        // Helps filter out things like shadows or reflections
        double aspectRatio = static_cast<double>(bounds.width) / bounds.height;
        if (contourFiltering && aspectRatio > adaptiveMaxAspectRatio) {
            aspectRatioFiltered++;
            continue;  // Skip to next contour
        }
        
        // This contour has passed all our quality filters!
        finalAccepted++;
        newBounds.push_back(bounds);
        
        // Update visualization if enabled
        if (visualizationEnabled) {
            // 1. Contour Outline (Green)
            // Redraw the contour in green to show it passed
            cv::drawContours(debugViz, contours, i, cv::Scalar(0, 255, 0), 3);
            
            // 2. Bounding Box (Blue)
            // Draw rectangle around the motion region
            cv::rectangle(debugViz, bounds, cv::Scalar(255, 0, 0), 2);
            
            // 3. Statistics Label (White)
            // Show key metrics for this region
            std::string label = "A:" + std::to_string((int)area) + " S:" + std::to_string((int)(solidity*100)) + "%";
            cv::putText(debugViz, label, cv::Point(bounds.x, bounds.y - 5), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
        }
    }
    
    // Save debug visualization if enabled
    if (visualizationEnabled && !debugViz.empty() && (frameCount % 10 == 0 || totalContours > 0)) {
        std::string debugPath = visualizationPath + "/debug_contours_frame_" + 
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
    if (contours.empty()) return permissiveMinArea;
    
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
    if (contours.empty()) return permissiveMinSolidity;
    
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
    if (contours.empty()) return permissiveMaxAspectRatio;
    
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

        if (config["contour_detection_mode"]) contourDetectionMode = config["contour_detection_mode"].as<std::string>();
        
        // Permissive mode settings
        if (config["permissive_min_area"]) permissiveMinArea = config["permissive_min_area"].as<double>();
        if (config["permissive_min_solidity"]) permissiveMinSolidity = config["permissive_min_solidity"].as<double>();
        if (config["permissive_max_aspect_ratio"]) permissiveMaxAspectRatio = config["permissive_max_aspect_ratio"].as<double>();
        
        // Adaptive calculation settings
        if (config["adaptive_update_interval"]) adaptiveUpdateInterval = config["adaptive_update_interval"].as<int>();
        
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
