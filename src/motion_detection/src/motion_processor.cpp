/**
 * Motion Processor Implementation
 * ================================
 * This file handles the initial motion detection pipeline that produces motion boxes
 * (TrackedObjects) which are then consolidated by motion_region_consolidator.cpp.
 * 
 * Pipeline Overview:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │ Raw Frame → Preprocessed → Motion Detection → Morphology → Motion Boxes │
 * └─────────────────────────────────────────────────────────────────────────┘
 * 
 * Key Concepts:
 * - Motion Detection: Identifies pixels that changed between frames
 * - Motion Boxes: Bounding rectangles around contiguous motion regions
 * - Adaptive vs Permissive: Two modes for contour filtering thresholds
 * - Morphological Operations: Clean up noise and connect motion regions
 * 
 * Output:
 * - Vector of cv::Rect (motion boxes) that feed into motion_region_consolidator
 */

#include "motion_processor.hpp"
#include "logger.hpp"
#include <yaml-cpp/yaml.h>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * Constructor: Initializes Motion Processor with Configuration
 * ============================================================
 * Sets default values for all parameters, then loads from config file.
 * 
 * Key Parameter Categories:
 * 1. Color Processing: How to convert input frames (grayscale/rgb)
 * 2. Preprocessing: Contrast enhancement and noise reduction
 * 3. Motion Detection: Frame differencing vs background subtraction
 * 4. Morphology: Operations to clean up motion masks
 * 5. Contour Detection: Filters to identify valid motion regions
 * 
 * @param configPath - Path to YAML configuration file
 */
MotionProcessor::MotionProcessor(const std::string& configPath) 
    : firstFrame(true),
      maxThreshold(255),
      
      // INPUT COLOR PROCESSING
      // "grayscale": Standard motion detection (faster, recommended)
      // "rgb": Color-based detection (slower, experimental)
      processingMode("grayscale"),
      
      // IMAGE PREPROCESSING
      // Contrast enhancement: Helps in low-light or varying conditions
      contrastEnhancement(false),
      blurType("gaussian"),  // Options: "gaussian", "median", "bilateral"
      claheClipLimit(2.0),   // CLAHE contrast limit (higher = more enhancement)
      claheTileSize(8),      // CLAHE tile size (smaller = more local adaptation)
      gaussianBlurSize(5),   // Gaussian kernel size (odd number, larger = more blur)
      medianBlurSize(5),     // Median filter size (odd number)
      bilateralD(15),        // Bilateral filter diameter
      bilateralSigmaColor(75.0),  // Bilateral color space sigma
      bilateralSigmaSpace(75.0),  // Bilateral coordinate space sigma
      
      // MOTION DETECTION METHODS
      // Frame differencing (always on) + optional background subtraction
      backgroundSubtraction(false),  // Enable for slow-moving objects
      
      // MORPHOLOGICAL OPERATIONS
      // Clean up motion masks by filling holes and removing noise
      morphology(true),       // Enable/disable all morphology
      morphKernelSize(5),     // Kernel size for all operations (larger = more aggressive)
      morphClose(true),       // Fill holes in motion regions
      morphOpen(true),        // Remove small noise blobs
      dilation(true),         // Expand motion regions
      erosion(false),         // Shrink motion regions (usually off)
      
      // CONTOUR PROCESSING
      // Analyze shapes to filter out false detections
      convexHull(true),            // Use convex hull for shape analysis
      contourApproximation(true),  // Simplify contour shapes
      contourFiltering(true),      // Apply quality filters
      contourEpsilonFactor(0.03),  // How much to simplify contours (0.01-0.05)
      contourDetectionMode("adaptive"),  // "adaptive" or "permissive"
      
      // PERMISSIVE MODE DEFAULTS
      // Used when contourDetectionMode = "permissive"
      // Very loose thresholds to catch all potential motion
      permissiveMinArea(50),           // Minimum pixels (50 = very small)
      permissiveMinSolidity(0.1),      // Shape solidity (0.1 = very loose)
      permissiveMaxAspectRatio(10.0),  // Width/height ratio (10.0 = very elongated allowed)
      
      // ADAPTIVE MODE CACHING
      // Adaptive mode recalculates thresholds periodically based on detected contours
      adaptiveUpdateInterval(150),  // Recalculate every 150 frames (5 sec @ 30fps)
      lastAdaptiveUpdate(0),        // Frame number of last update
      cachedAdaptiveMinArea(minContourArea),
      cachedAdaptiveMinSolidity(minContourSolidity),
      cachedAdaptiveMaxAspectRatio(maxContourAspectRatio) {
    loadConfig(configPath);  // Override defaults with config file values
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
    
    // Store the original frame for downstream processing
    result.originalFrame = frame.clone();
    
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
    
    // Output overall motion detection summary
    if (result.hasMotion) {
        LOG_INFO("=== MOTION DETECTION SUMMARY ===");
        LOG_INFO("Motion detected: {} regions", result.detectedBounds.size());
        LOG_INFO("Frame size: {}x{}", result.processedFrame.cols, result.processedFrame.rows);
        LOG_INFO("Processing mode: {}", processingMode);
        LOG_INFO("Background subtraction: {}", backgroundSubtraction ? "enabled" : "disabled");
        LOG_INFO("=== END MOTION DETECTION SUMMARY ===");
    }
    
    // Store current frame for next comparison
    setPrevFrame(result.processedFrame);
    
    return result;
}

// ============================================================================
// VISUALIZATION METHODS
// ============================================================================

void MotionProcessor::saveProcessingVisualization(const ProcessingResult& result, const std::string& outputPath) {
    if (result.originalFrame.empty()) {
        LOG_WARN("Cannot create visualization: original frame is empty");
        return;
    }
    
    cv::Mat visualization = createProcessingVisualization(result);
    
    std::string finalPath = outputPath.empty() ? 
        visualizationPath + "/motion_processor_output.jpg" : outputPath;
    
    // Ensure directory exists
    fs::path path(finalPath);
    fs::create_directories(path.parent_path());
    
    if (cv::imwrite(finalPath, visualization)) {
        LOG_INFO("Saved MotionProcessor visualization to: {}", finalPath);
    } else {
        LOG_ERROR("Failed to save MotionProcessor visualization to: {}", finalPath);
    }
}

cv::Mat MotionProcessor::createProcessingVisualization(const ProcessingResult& result) const {
    // Create a comprehensive visualization showing all processing steps
    cv::Mat visualization;
    
    // Create a 2x3 grid layout for all processing steps
    std::vector<cv::Mat> steps;
    
    // Step 1: Original frame
    cv::Mat originalWithBoxes = result.originalFrame.clone();
    drawMotionBoxes(originalWithBoxes, result.detectedBounds);
    steps.push_back(originalWithBoxes);
    
    // Step 2: Processed frame (grayscale/blurred)
    cv::Mat processedDisplay;
    if (result.processedFrame.channels() == 1) {
        cv::cvtColor(result.processedFrame, processedDisplay, cv::COLOR_GRAY2BGR);
    } else {
        processedDisplay = result.processedFrame.clone();
    }
    steps.push_back(processedDisplay);
    
    // Step 3: Frame difference
    cv::Mat diffDisplay;
    if (!result.frameDiff.empty()) {
        cv::cvtColor(result.frameDiff, diffDisplay, cv::COLOR_GRAY2BGR);
    } else {
        diffDisplay = cv::Mat::zeros(result.originalFrame.size(), CV_8UC3);
    }
    steps.push_back(diffDisplay);
    
    // Step 4: Threshold
    cv::Mat threshDisplay;
    if (!result.thresh.empty()) {
        cv::cvtColor(result.thresh, threshDisplay, cv::COLOR_GRAY2BGR);
    } else {
        threshDisplay = cv::Mat::zeros(result.originalFrame.size(), CV_8UC3);
    }
    steps.push_back(threshDisplay);
    
    // Step 5: Morphological operations
    cv::Mat morphDisplay;
    if (!result.morphological.empty()) {
        cv::cvtColor(result.morphological, morphDisplay, cv::COLOR_GRAY2BGR);
    } else {
        morphDisplay = cv::Mat::zeros(result.originalFrame.size(), CV_8UC3);
    }
    steps.push_back(morphDisplay);
    
    // Step 6: Final result (original with motion boxes)
    cv::Mat finalResult = result.originalFrame.clone();
    drawMotionBoxes(finalResult, result.detectedBounds);
    steps.push_back(finalResult);
    
    // Create grid layout
    int cols = 3;
    int rows = 2;
    int cellWidth = result.originalFrame.cols / cols;
    int cellHeight = result.originalFrame.rows / rows;
    
    visualization = cv::Mat(cellHeight * rows, cellWidth * cols, CV_8UC3);
    
    std::vector<std::string> labels = {
        "Original + Motion Boxes", "Processed Frame", "Frame Difference",
        "Threshold", "Morphological", "Final Result"
    };
    
    for (int i = 0; i < steps.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        
        cv::Mat cell = visualization(cv::Rect(col * cellWidth, row * cellHeight, cellWidth, cellHeight));
        
        // Resize step to fit cell
        cv::Mat resizedStep;
        cv::resize(steps[i], resizedStep, cv::Size(cellWidth, cellHeight));
        resizedStep.copyTo(cell);
        
        // Add label
        cv::putText(visualization, labels[i], 
                   cv::Point(col * cellWidth + 10, row * cellHeight + 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
    }
    
    // Add summary info
    std::string summary = "Motion Detected: " + std::to_string(result.detectedBounds.size()) + 
                         " regions | Has Motion: " + (result.hasMotion ? "YES" : "NO");
    cv::putText(visualization, summary, cv::Point(10, visualization.rows - 20),
               cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    
    return visualization;
}

void MotionProcessor::drawMotionBoxes(cv::Mat& image, const std::vector<cv::Rect>& detectedBounds) const {
    for (size_t i = 0; i < detectedBounds.size(); ++i) {
        const auto& bounds = detectedBounds[i];
        
        // Draw motion box in bright green
        cv::rectangle(image, bounds, cv::Scalar(0, 255, 0), 2);
        
        // Add motion box ID label
        std::string label = "M" + std::to_string(i);
        cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
        cv::Point textOrigin(bounds.x, bounds.y - 5);
        
        // Background for text
        cv::rectangle(image, 
                     cv::Point(textOrigin.x, textOrigin.y - textSize.height),
                     cv::Point(textOrigin.x + textSize.width, textOrigin.y + 5),
                     cv::Scalar(0, 255, 0), -1);
        
        // Text
        cv::putText(image, label, textOrigin, cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                   cv::Scalar(0, 0, 0), 1);
        
        // Add size info
        std::string sizeInfo = std::to_string(bounds.width) + "x" + std::to_string(bounds.height);
        cv::putText(image, sizeInfo, 
                   cv::Point(bounds.x, bounds.y + bounds.height - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
    }
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

/**
 * Extract and Filter Contours from Motion Mask
 * ============================================
 * This is the critical function that converts a binary motion mask into
 * discrete motion boxes (bounding rectangles).
 * 
 * Processing Flow:
 * 1. Find all contours in the motion mask
 * 2. Filter by area (remove tiny regions)
 * 3. Simplify contour shapes (optional)
 * 4. Analyze shape quality using convex hull
 * 5. Filter by solidity (how solid vs scattered)
 * 6. Filter by aspect ratio (remove elongated shapes)
 * 7. Return bounding boxes of accepted contours
 * 
 * Modes:
 * - Adaptive: Dynamically calculates thresholds from scene statistics
 * - Permissive: Uses fixed, loose thresholds to catch everything
 * 
 * @param processed - Binary motion mask (white = motion, black = no motion)
 * @return Vector of bounding rectangles (motion boxes) → feeds consolidator
 */
std::vector<cv::Rect> MotionProcessor::extractContours(const cv::Mat& processed) {
    // Step 1: Find Contours
    // RETR_EXTERNAL = only outer contours (ignore holes)
    // CHAIN_APPROX_SIMPLE = compress contour points (store endpoints only)
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(processed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    std::vector<cv::Rect> newBounds;  // Output: motion boxes
    static int frameCount = 0;
    frameCount++;
    
    // Debug visualization (only if enabled in config)
    cv::Mat debugViz;
    if (visualizationEnabled) {
        cv::cvtColor(processed, debugViz, cv::COLOR_GRAY2BGR);
    }
    
    // Tracking statistics for troubleshooting
    int totalContours = contours.size();       // Total found
    int areaFiltered = 0;                      // Rejected: too small
    int solidityFiltered = 0;                  // Rejected: too scattered
    int aspectRatioFiltered = 0;               // Rejected: too elongated
    int finalAccepted = 0;                     // Accepted count
    
    // Step 2: Determine Filtering Thresholds
    // Start with permissive defaults (will be updated based on mode)
    double adaptiveMinArea = permissiveMinArea;
    double adaptiveMinSolidity = permissiveMinSolidity;
    double adaptiveMaxAspectRatio = permissiveMaxAspectRatio;
    
    // ADAPTIVE MODE: Learn thresholds from scene
    // Recalculates thresholds every N frames based on detected contour statistics
    // Helps adapt to different scenes (close vs far, many vs few birds, etc.)
    if (contourDetectionMode == "adaptive") {
        // Only recalculate periodically (expensive operation)
        // Default: every 150 frames = 5 seconds at 30fps
        if (frameCount - lastAdaptiveUpdate >= adaptiveUpdateInterval) {
            // Calculate new thresholds from current contour distribution
            cachedAdaptiveMinArea = calculateAdaptiveMinArea(contours);
            cachedAdaptiveMinSolidity = calculateAdaptiveMinSolidity(contours);
            cachedAdaptiveMaxAspectRatio = calculateAdaptiveMaxAspectRatio(contours);
            lastAdaptiveUpdate = frameCount;
            LOG_INFO("Updated adaptive values at frame {}", frameCount);
        }
        // Use cached values for this frame
        adaptiveMinArea = cachedAdaptiveMinArea;
        adaptiveMinSolidity = cachedAdaptiveMinSolidity;
        adaptiveMaxAspectRatio = cachedAdaptiveMaxAspectRatio;
    } 
    // PERMISSIVE MODE: Use fixed, loose thresholds
    // Catches everything possible, lets consolidator do the heavy filtering
    else if (contourDetectionMode == "permissive") {
        adaptiveMinArea = permissiveMinArea;              // 50 pixels (very small)
        adaptiveMinSolidity = permissiveMinSolidity;      // 0.1 (very loose shape)
        adaptiveMaxAspectRatio = permissiveMaxAspectRatio; // 10.0 (very elongated OK)
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
    
    // Output motion boxes metadata for motion_region_consolidator
    if (!newBounds.empty()) {
        LOG_INFO("=== MOTION BOXES METADATA (Frame {}) ===", frameCount);
        LOG_INFO("Detected {} motion regions", newBounds.size());
        
        for (size_t i = 0; i < newBounds.size(); ++i) {
            const cv::Rect& bounds = newBounds[i];
            double area = bounds.width * bounds.height;
            double aspectRatio = static_cast<double>(bounds.width) / bounds.height;
            cv::Point center(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
            
            LOG_INFO("Motion Box {}: BBox({},{},{},{}) | Center({},{}) | Area: {:.0f} | Aspect: {:.2f}", 
                    i,
                    bounds.x, bounds.y, bounds.width, bounds.height,
                    center.x, center.y,
                    area, aspectRatio);
        }
        LOG_INFO("=== END MOTION BOXES METADATA ===");
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
// These functions analyze the current frame's contours to determine optimal
// filtering thresholds. They help the system adapt to different scenes:
// - Close-up scenes: Birds are large, use higher area thresholds
// - Far-away scenes: Birds are small, use lower area thresholds
// - Cluttered scenes: Many objects, use stricter shape filters
// - Simple scenes: Few objects, can be more permissive

/**
 * Calculate Adaptive Minimum Area Threshold
 * =========================================
 * Analyzes the size distribution of detected contours to determine
 * what size objects are "real" vs "noise" in the current scene.
 * 
 * Strategy:
 * - Collect all contour areas
 * - Sort them by size
 * - Use 10th percentile as minimum threshold
 * - This filters out the smallest 10% (likely noise)
 * 
 * Example:
 * If detected areas are: [20, 30, 45, 50, 200, 250, 300, 500, 800]
 * 10th percentile ≈ 45 pixels
 * So anything < 45 pixels is rejected as noise
 * 
 * Bounds: 50 - 1000 pixels (safety limits)
 * 
 * @param contours - All contours detected in current frame
 * @return Adaptive minimum area threshold in pixels
 */
double MotionProcessor::calculateAdaptiveMinArea(const std::vector<std::vector<cv::Point>>& contours) {
    if (contours.empty()) return permissiveMinArea;
    
    // Step 1: Calculate area of each contour
    std::vector<double> areas;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > 0) areas.push_back(area);
    }
    
    if (areas.empty()) return minContourArea;
    
    // Step 2: Sort areas from smallest to largest
    std::sort(areas.begin(), areas.end());
    
    // Step 3: Use 10th percentile as minimum
    // This means we reject the smallest 10% as noise
    size_t percentileIndex = static_cast<size_t>(areas.size() * 0.1);
    double adaptiveMin = areas[percentileIndex];
    
    // Step 4: Apply safety bounds
    // Never go below 50 (too noisy) or above 1000 (miss small birds)
    adaptiveMin = std::max(50.0, std::min(1000.0, adaptiveMin));
    
    LOG_DEBUG("Adaptive min area: {:.0f} pixels (from {} contours)", adaptiveMin, areas.size());
    
    return adaptiveMin;
}

/**
 * Calculate Adaptive Minimum Solidity Threshold
 * =============================================
 * Determines how "solid" a shape needs to be to qualify as a real object.
 * Solidity = contour area / convex hull area
 * 
 * Solidity Examples:
 * - 1.0 = Perfect solid shape (circle, square)
 * - 0.8 = Mostly solid with small irregularities (typical bird)
 * - 0.5 = Very irregular, scattered shape (tree branches swaying)
 * - 0.2 = Extremely scattered (reflections, shadows)
 * 
 * Strategy:
 * - Calculate solidity for all reasonable-sized contours
 * - Use 25th percentile as minimum threshold
 * - This allows the least solid 25% but rejects very scattered shapes
 * 
 * Why 25th percentile?
 * - More permissive than area (birds can have irregular shapes)
 * - Still filters out obvious non-objects
 * 
 * Bounds: 0.2 - 0.8 (safety limits)
 * 
 * @param contours - All contours detected in current frame
 * @return Adaptive minimum solidity threshold (0.0 - 1.0)
 */
double MotionProcessor::calculateAdaptiveMinSolidity(const std::vector<std::vector<cv::Point>>& contours) {
    if (contours.empty()) return permissiveMinSolidity;
    
    // Step 1: Calculate solidity for each reasonable-sized contour
    // Skip tiny contours (<100px) as they give unreliable solidity values
    std::vector<double> solidities;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < 100) continue; // Skip tiny noise
        
        // Calculate convex hull (smallest convex shape containing contour)
        std::vector<cv::Point> hull;
        cv::convexHull(contour, hull);
        double hullArea = cv::contourArea(hull);
        
        if (hullArea > 0) {
            // Solidity = how much of the hull is filled by the contour
            double solidity = area / hullArea;
            solidities.push_back(solidity);
        }
    }
    
    if (solidities.empty()) return minContourSolidity;
    
    // Step 2: Sort solidities from lowest to highest
    std::sort(solidities.begin(), solidities.end());
    
    // Step 3: Use 25th percentile as minimum
    // Allows irregularly-shaped birds while filtering scattered noise
    size_t percentileIndex = static_cast<size_t>(solidities.size() * 0.25);
    double adaptiveMin = solidities[percentileIndex];
    
    // Step 4: Apply safety bounds
    // Never go below 0.2 (too scattered) or above 0.8 (miss irregular birds)
    adaptiveMin = std::max(0.2, std::min(0.8, adaptiveMin));
    
    LOG_DEBUG("Adaptive min solidity: {:.2f} (from {} contours)", adaptiveMin, solidities.size());
    
    return adaptiveMin;
}

/**
 * Calculate Adaptive Maximum Aspect Ratio Threshold
 * =================================================
 * Determines how elongated a shape can be before it's rejected.
 * Aspect Ratio = width / height
 * 
 * Aspect Ratio Examples:
 * - 1.0 = Perfect square
 * - 2.0 = Twice as wide as tall (typical bird in flight)
 * - 5.0 = Very elongated (possible bird gliding)
 * - 10.0+ = Extremely elongated (likely shadow, wire, artifact)
 * 
 * Strategy:
 * - Calculate aspect ratio for all reasonable-sized contours
 * - Use 90th percentile as maximum threshold
 * - This allows most shapes (90%) but filters extreme outliers
 * 
 * Why 90th percentile?
 * - Permissive enough to allow various bird poses
 * - Strict enough to filter obvious non-birds (wires, shadows)
 * 
 * Bounds: 2.0 - 15.0 (safety limits)
 * 
 * @param contours - All contours detected in current frame
 * @return Adaptive maximum aspect ratio threshold
 */
double MotionProcessor::calculateAdaptiveMaxAspectRatio(const std::vector<std::vector<cv::Point>>& contours) {
    if (contours.empty()) return permissiveMaxAspectRatio;
    
    // Step 1: Calculate aspect ratio for each reasonable-sized contour
    // Skip tiny contours (<100px) as they give unreliable ratios
    std::vector<double> aspectRatios;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area < 100) continue; // Skip tiny noise
        
        // Get bounding rectangle
        cv::Rect bounds = cv::boundingRect(contour);
        if (bounds.width > 0 && bounds.height > 0) {
            // Aspect ratio = width / height
            // Note: Always >= 1.0 in our case (we could flip for consistency)
            double aspectRatio = static_cast<double>(bounds.width) / bounds.height;
            aspectRatios.push_back(aspectRatio);
        }
    }
    
    if (aspectRatios.empty()) return maxContourAspectRatio;
    
    // Step 2: Sort aspect ratios from lowest to highest
    std::sort(aspectRatios.begin(), aspectRatios.end());
    
    // Step 3: Use 90th percentile as maximum
    // This keeps 90% of shapes and only filters extreme outliers
    size_t percentileIndex = static_cast<size_t>(aspectRatios.size() * 0.9);
    double adaptiveMax = aspectRatios[percentileIndex];
    
    // Step 4: Apply safety bounds
    // Never go below 2.0 (too strict) or above 15.0 (allows obvious artifacts)
    adaptiveMax = std::max(2.0, std::min(15.0, adaptiveMax));
    
    LOG_DEBUG("Adaptive max aspect ratio: {:.1f} (from {} contours)", adaptiveMax, aspectRatios.size());
    
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

        // ===============================
        // IMAGE PROCESSING
        // ===============================
        if (config["processing_mode"]) processingMode = config["processing_mode"].as<std::string>();
        
        // Image Preprocessing
        if (config["contrast_enhancement"]) contrastEnhancement = config["contrast_enhancement"].as<bool>();
        if (config["clahe_clip_limit"]) claheClipLimit = config["clahe_clip_limit"].as<double>();
        if (config["clahe_tile_size"]) claheTileSize = config["clahe_tile_size"].as<int>();
        
        // Blur Parameters
        if (config["gaussian_blur_size"]) gaussianBlurSize = config["gaussian_blur_size"].as<int>();
        if (config["median_blur_size"]) medianBlurSize = config["median_blur_size"].as<int>();
        if (config["bilateral_d"]) bilateralD = config["bilateral_d"].as<int>();
        if (config["bilateral_sigma_color"]) bilateralSigmaColor = config["bilateral_sigma_color"].as<double>();
        if (config["bilateral_sigma_space"]) bilateralSigmaSpace = config["bilateral_sigma_space"].as<double>();

        // ===============================
        // MOTION DETECTION
        // ===============================
        if (config["background_subtraction"]) backgroundSubtraction = config["background_subtraction"].as<bool>();
        if (config["max_threshold"]) maxThreshold = config["max_threshold"].as<int>();

        // ===============================
        // MORPHOLOGICAL OPERATIONS
        // ===============================
        if (config["morphology"]) morphology = config["morphology"].as<bool>();
        if (config["morph_kernel_size"]) morphKernelSize = config["morph_kernel_size"].as<int>();
        if (config["morph_close"]) morphClose = config["morph_close"].as<bool>();
        if (config["morph_open"]) morphOpen = config["morph_open"].as<bool>();
        if (config["dilation"]) dilation = config["dilation"].as<bool>();
        if (config["erosion"]) erosion = config["erosion"].as<bool>();

        // ===============================
        // CONTOUR PROCESSING
        // ===============================
        if (config["convex_hull"]) convexHull = config["convex_hull"].as<bool>();
        if (config["contour_approximation"]) contourApproximation = config["contour_approximation"].as<bool>();
        if (config["contour_epsilon_factor"]) contourEpsilonFactor = config["contour_epsilon_factor"].as<double>();
        if (config["contour_filtering"]) contourFiltering = config["contour_filtering"].as<bool>();

        // Contour Filtering Parameters
        if (config["min_contour_area"]) minContourArea = config["min_contour_area"].as<int>();
        if (config["max_contour_aspect_ratio"]) maxContourAspectRatio = config["max_contour_aspect_ratio"].as<double>();
        if (config["min_contour_solidity"]) minContourSolidity = config["min_contour_solidity"].as<double>();
        
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
