#include "motion_pipeline.hpp"
#include "logger.hpp"

std::pair<MotionProcessor::ProcessingResult, std::vector<ConsolidatedRegion>> 
processFrameAndConsolidate(MotionProcessor& motionProcessor, 
                          MotionRegionConsolidator& regionConsolidator,
                          const cv::Mat& frame,
                          const std::string& visualizationPath) {
    
    // Process frame using motion processor
    MotionProcessor::ProcessingResult processingResult = motionProcessor.processFrame(frame);
    
    // Create simple TrackedObjects from detected bounds for region consolidation
    std::vector<TrackedObject> trackedObjects;
    static int nextId = 0; // Simple ID assignment
    
    for (const auto& bounds : processingResult.detectedBounds) {
        int currentId = nextId++;
        trackedObjects.emplace_back(currentId, bounds, "uuid_" + std::to_string(currentId));
    }
    
    // Consolidate motion regions with optional visualization
    std::vector<ConsolidatedRegion> consolidatedRegions;
    if (!trackedObjects.empty()) {
        if (!visualizationPath.empty()) {
            // Use the original frame from motion processor for visualization
            consolidatedRegions = regionConsolidator.consolidateRegionsWithVisualization(
                trackedObjects, processingResult.originalFrame, visualizationPath);
        } else {
            consolidatedRegions = regionConsolidator.consolidateRegions(trackedObjects);
        }
        
        LOG_INFO("Motion detection: {} -> {} regions", 
                 trackedObjects.size(), consolidatedRegions.size());
    }
    
    return {processingResult, consolidatedRegions};
}
