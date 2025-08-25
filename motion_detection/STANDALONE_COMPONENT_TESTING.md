# Standalone Component Testing

## Overview

Both `MotionProcessor` and `MotionRegionConsolidator` now have **independent visualization capabilities** that allow you to test them separately without dependencies on each other. Each component has a **single consolidated test file** that supports both manual testing and Google Test framework modes.

## MotionProcessor Testing

### Features
- **Independent visualization**: Creates its own output images without requiring MotionRegionConsolidator
- **Comprehensive processing view**: Shows all 6 processing steps in a 2x3 grid layout
- **Real image processing**: Uses actual bird feeder images for testing
- **Dual testing modes**: Manual testing and Google Test framework
- **Automatic file output**: Saves visualizations to output directories

### Test Commands
```bash
# Manual testing mode (legacy)
make motion_processor_test
./motion_processor_test

# Google Test framework mode
make motion_processor_test
./motion_processor_test --gtest
```

### Output Files
**Manual Mode** (`test_results/motion_processor/`):
- `01_preprocess_frame/` - Original and preprocessed frames
- `02_detect_motion/` - Motion detection sequence
- `03_morphological_ops/` - Before/after morphological operations
- `04_extract_contours/` - Contour detection with bounding boxes
- `05_complete_pipeline/` - End-to-end processing pipeline

**Google Test Mode** (`motion_processor_test_output/`):
- `frame1_processing.jpg` - Baseline frame processing visualization
- `frame2_processing.jpg` - Motion detection frame visualization  
- `debug_contours_frame_1.jpg` - Contour extraction debug image

### Visualization Layout (2x3 Grid)
1. **Original + Motion Boxes** - Input image with detected motion regions
2. **Processed Frame** - Grayscale/blurred preprocessing result
3. **Frame Difference** - Motion detection intermediate step
4. **Threshold** - Binary motion mask
5. **Morphological** - Cleaned up motion regions
6. **Final Result** - Complete processing with motion boxes

## MotionRegionConsolidator Testing

### Features
- **Synthetic visualization**: Creates grid-based background when no input image provided
- **Independent consolidation**: Tests region consolidation logic without MotionProcessor
- **Multiple test scenarios**: Synthetic data, bird-like data, configuration variations
- **Dual testing modes**: Manual testing and Google Test framework
- **Automatic file output**: Saves visualizations to output directories

### Test Commands
```bash
# Manual testing mode (legacy)
make motion_region_consolidator_test
./motion_region_consolidator_test

# Google Test framework mode
make motion_region_consolidator_test
./motion_region_consolidator_test --gtest
```

### Output Files
**Manual Mode** (`test_results/motion_region_consolidator/`):
- Various test-specific output directories

**Google Test Mode** (`motion_region_consolidator_test_output/`):
- `synthetic_consolidation.jpg` - Synthetic object consolidation
- `bird_like_consolidation.jpg` - Bird-like object consolidation
- `tight_config.jpg` - Tight configuration test
- `loose_config.jpg` - Loose configuration test
- `empty_input.jpg` - Edge case: empty input
- `single_object.jpg` - Edge case: single object
- `min_objects.jpg` - Edge case: minimum objects
- `real_bird_consolidation_visualization.jpg` - Combined visualization from integration test

### Visualization Features
- **Grid background**: 100x100 pixel grid for spatial reference
- **Motion boxes**: Green boxes showing individual tracked objects
- **Consolidated regions**: Red boxes showing grouped regions
- **Labels**: Object IDs, region counts, and size information
- **Legend**: Color coding and summary statistics

## Integration Testing

### Combined Pipeline Testing
The `motion_region_consolidator_test` includes integration testing for the full pipeline:
```bash
make motion_region_consolidator_test
./motion_region_consolidator_test --gtest_filter="*RealBird*"
```

### Main Application Testing
The main application (`BirdsOfPlay`) uses both components together:
```bash
make BirdsOfPlay
./BirdsOfPlay
```

## Key Benefits

1. **Independent Debugging**: Test each component in isolation
2. **Focused Development**: Work on one component without affecting the other
3. **Visual Verification**: See exactly what each component produces
4. **Configuration Testing**: Test different parameters independently
5. **Edge Case Testing**: Validate behavior with boundary conditions
6. **Consolidated Testing**: Single test file per component with dual modes
7. **Backward Compatibility**: Manual testing mode preserved for legacy workflows

## File Structure

```
build_debug/
├── test_results/                          # Manual testing output
│   ├── motion_processor/
│   │   ├── 01_preprocess_frame/
│   │   ├── 02_detect_motion/
│   │   ├── 03_morphological_ops/
│   │   ├── 04_extract_contours/
│   │   └── 05_complete_pipeline/
│   └── motion_region_consolidator/
├── motion_processor_test_output/          # Google Test mode output
│   ├── frame1_processing.jpg
│   ├── frame2_processing.jpg
│   └── debug_contours_frame_1.jpg
└── motion_region_consolidator_test_output/ # Google Test mode output
    ├── synthetic_consolidation.jpg
    ├── bird_like_consolidation.jpg
    ├── tight_config.jpg
    ├── loose_config.jpg
    ├── empty_input.jpg
    ├── single_object.jpg
    ├── min_objects.jpg
    └── real_bird_consolidation_visualization.jpg
```

## Usage Examples

### Test MotionProcessor Only
```bash
# Manual mode - comprehensive step-by-step testing
./motion_processor_test

# Google Test mode - specific test cases
./motion_processor_test --gtest_filter="*StandaloneProcessing*"
```

### Test MotionRegionConsolidator Only  
```bash
# Manual mode - comprehensive testing
./motion_region_consolidator_test

# Google Test mode - specific test cases
./motion_region_consolidator_test --gtest_filter="*Synthetic*"
```

### Test Full Pipeline
```bash
# Test complete motion detection + consolidation pipeline
./motion_region_consolidator_test --gtest_filter="*RealBird*"
```

## Combined Visualization

The **integration test** creates a special combined visualization that shows:
- **MotionProcessor output**: Green motion boxes from real bird images
- **MotionRegionConsolidator output**: Red consolidated regions optimized for YOLO
- **Real bird data**: Uses your actual goldfinch feeder images
- **Complete pipeline**: Shows the full motion detection → consolidation workflow

This combined visualization is saved as `motion_region_consolidator_test_output/real_bird_consolidation_visualization.jpg` and demonstrates how both components work together in the complete pipeline.

## Test Modes Comparison

| Feature | Manual Mode | Google Test Mode |
|---------|-------------|------------------|
| **Execution** | Sequential function calls | Individual test cases |
| **Output** | Detailed step-by-step | Focused test results |
| **Visualization** | Comprehensive directories | Specific output files |
| **Debugging** | Verbose logging | Structured test reports |
| **Integration** | Full pipeline testing | Component isolation |
| **Legacy Support** | ✅ Preserved | ✅ Modern framework |

This setup gives you complete flexibility to test and debug each component independently while maintaining the ability to test the integrated pipeline. The consolidated test files eliminate redundancy while preserving both testing approaches.
