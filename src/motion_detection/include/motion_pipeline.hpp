#pragma once

#include "motion_processor.hpp"
#include "motion_region_consolidator.hpp"
#include "tracked_object.hpp"
#include <opencv2/opencv.hpp>
#include <utility>
#include <vector>
#include <string>

/**
 * @brief Unified function to process frame and consolidate regions with optional visualization
 * 
 * This function encapsulates the complete pipeline:
 * 1. Process frame using MotionProcessor
 * 2. Convert detected bounds to TrackedObjects  
 * 3. Consolidate regions using MotionRegionConsolidator
 * 4. Optionally save visualization
 * 
 * @param motionProcessor Reference to MotionProcessor instance
 * @param regionConsolidator Reference to MotionRegionConsolidator instance
 * @param frame Input frame to process
 * @param visualizationPath Optional path to save consolidation visualization
 * @return Pair of ProcessingResult and ConsolidatedRegions
 */
std::pair<MotionProcessor::ProcessingResult, std::vector<ConsolidatedRegion>> 
processFrameAndConsolidate(MotionProcessor& motionProcessor, 
                          MotionRegionConsolidator& regionConsolidator,
                          const cv::Mat& frame,
                          const std::string& visualizationPath = "");
