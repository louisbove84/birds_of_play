# Motion Region Consolidator - Developer Guide

## Overview
The Motion Region Consolidator transforms raw motion detection boxes into optimized regions for YOLO11 object detection.

## Pipeline Flow

```
Input: TrackedObjects (motion boxes from motion_processor.cpp)
  ↓
  ├─ Step 1: Group by Proximity (groupObjectsByProximity)
  │  ├─ Small dataset (<50 objects): Pairwise O(n²) algorithm
  │  └─ Large dataset (≥50 objects): Grid-based O(n) algorithm
  ↓
  ├─ Step 2: Filter Groups (minObjectsPerRegion threshold)
  ↓
  ├─ Step 3: Create Consolidated Regions (createConsolidatedRegions)
  │  ├─ Calculate bounding boxes
  │  ├─ Expand boxes (regionExpansionFactor)
  │  ├─ Adjust to ideal size (640x640)
  │  └─ Split large regions into sub-regions
  ↓
  ├─ Step 4: Update Existing Regions (temporal tracking)
  ↓
  ├─ Step 5: Merge Overlapping Regions
  ↓
  ├─ Step 6: Remove Stale Regions (maxFramesWithoutUpdate)
  ↓
  ├─ Step 7: Split Large Regions (splitLargeRegions)
  │  └─ Divides regions >832x832 into 640x640 grids
  ↓
  └─ Step 8: Remove Overlapping Regions (removeOverlappingRegions)
     └─ Prioritizes regions closer to ideal 640x640 size
  ↓
Output: Optimized ConsolidatedRegions (ready for YOLO11)
```

## Key Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `idealModelRegionSize` | 640 | Target size for YOLO11 input (640x640) |
| `sizeTolerancePercent` | 30 | Acceptable deviation from ideal size (±30% = 448-832) |
| `maxDistanceThreshold` | varies | Maximum distance for grouping objects |
| `minObjectsPerRegion` | varies | Minimum objects required to form a region |
| `maxFramesWithoutUpdate` | varies | Frames before stale region removal |
| `regionExpansionFactor` | varies | How much to expand bounding boxes |
| `overlapThreshold` | varies | Overlap ratio for merging regions |

## Algorithm Details

### 1. Proximity Grouping

#### Pairwise Method (O(n²))
- Used for <50 objects
- Compares every object pair
- Groups objects within `maxDistanceThreshold`

#### Grid-Based Method (O(n))
- Used for ≥50 objects
- Partitions frame into grid cells
- Only checks neighbors in 3x3 cell grid
- Much faster for large object counts

### 2. Region Creation

1. **Calculate Bounding Box**: Union of all grouped object bounding boxes
2. **Expand Box**: Multiply by `regionExpansionFactor` to capture more context
3. **Adjust Aspect Ratio**: Make regions more square for YOLO
4. **Resize to Ideal**: Target 640x640 while preserving important features
5. **Clamp to Frame**: Ensure regions stay within frame boundaries

### 3. Region Splitting

**Why Split?**
- YOLO11 optimal input: 640x640 pixels
- Large regions (>832x832) get downscaled
- Downscaling loses detail and reduces detection accuracy

**How It Works:**
```
Example: 1280x900 region

Splits needed:
- Width: 1280 / 640 = 2 splits
- Height: 900 / 640 = 2 splits

Result: 2x2 grid = 4 sub-regions
- Region 1: 640x640 at (0, 0)
- Region 2: 640x640 at (640, 0)
- Region 3: 640x640 at (0, 640)
- Region 4: 640x260 at (640, 640)  [partial region kept]
```

### 4. Overlap Removal

**Decision Logic:**
1. If regions overlap >overlapThreshold:
   - Prefer region within ideal size range (448-832)
   - If both ideal or both non-ideal: prefer smaller deviation from 640
   - If tied: prefer region with more objects

2. Remove losing region

## Troubleshooting Guide

### Problem: Too Many Small Regions

**Symptoms**: 
- Many regions <448x448
- Poor YOLO detection

**Solutions**:
- Increase `minObjectsPerRegion` (requires more motion boxes per region)
- Increase `maxDistanceThreshold` (groups more distant objects)
- Decrease `regionExpansionFactor` (less expansion of boxes)

### Problem: Too Few Regions (Missing Birds)

**Symptoms**:
- Regions too large (>832x832)
- Birds not detected

**Solutions**:
- Decrease `maxDistanceThreshold` (tighter grouping)
- Ensure `splitLargeRegions` is enabled
- Check `sizeTolerancePercent` (should be ~30%)

### Problem: Overlapping Regions

**Symptoms**:
- Multiple regions covering same area
- Duplicate detections

**Solutions**:
- Decrease `overlapThreshold` (more aggressive removal)
- Check `removeOverlappingRegions` is working
- Ensure Step 8 in pipeline is executed

### Problem: Regions Disappearing Between Frames

**Symptoms**:
- Flickering regions
- Inconsistent tracking

**Solutions**:
- Increase `maxFramesWithoutUpdate` (keep regions longer)
- Check temporal tracking in `updateExistingRegions`
- Ensure object IDs are consistent across frames

## Code Comments Added

I've added detailed inline comments throughout the file:

1. **File Header**: Overview of the module and key concepts
2. **Function Headers**: Purpose, algorithm, parameters, and return values
3. **Algorithm Steps**: Line-by-line explanation of complex logic
4. **Decision Logic**: Why certain thresholds and conditions are used
5. **Performance Notes**: Complexity analysis and optimization rationale

## Debugging Tips

### Enable Debug Logging
The file uses LOG_DEBUG and LOG_INFO macros. Check logger configuration to see these messages.

### Key Log Messages to Watch
```
- "Consolidating N tracked objects" - Input size
- "Before/After final splitting" - Region splitting count
- "Before/After overlap removal" - Overlap removal count
- "Final region N: WxH at (X,Y)" - Output regions
```

### Visualization
Use `consolidateRegionsWithVisualization()` to create visual output:
- Green boxes: Original motion boxes
- Red boxes: Consolidated regions
- Labels show object counts and dimensions

## Performance Characteristics

| Object Count | Method | Complexity | Typical Time |
|--------------|--------|-----------|--------------|
| <50 | Pairwise | O(n²) | <1ms |
| 50-500 | Grid | O(n) | 1-5ms |
| >500 | Grid | O(n) | 5-20ms |

## Future Improvements

1. **Adaptive Thresholds**: Automatically adjust based on frame characteristics
2. **Machine Learning**: Learn optimal grouping from labeled data
3. **Multi-scale Processing**: Handle objects of varying sizes better
4. **GPU Acceleration**: Parallelize proximity calculations

## Related Files

- `motion_region_consolidator.hpp` - Class definition and interfaces
- `motion_processor.cpp` - Generates TrackedObjects (input)
- `motion_pipeline.cpp` - Calls consolidateRegions()
- `config.yaml` - Configuration parameters

## Questions?

If you're troubleshooting a specific issue:
1. Check the log output for the 8 pipeline steps
2. Visualize regions with `consolidateRegionsWithVisualization()`
3. Adjust config parameters based on symptoms above
4. Review inline comments in the specific function

---
*Last Updated: Based on commented version with 803 lines*

