# Motion Processor - Developer Guide

## Overview
The Motion Processor is the **first stage** of the birds_of_play detection pipeline. It converts raw video frames into motion boxes (bounding rectangles around moving objects).

## Pipeline Position

```
┌─────────────────────────────────────────────────────────────────────┐
│                    BIRDS OF PLAY DETECTION PIPELINE                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  1. MOTION PROCESSOR (this file)                                    │
│     Raw Frame → Motion Boxes (cv::Rect)                             │
│                                                                       │
│  2. MOTION TRACKER                                                   │
│     Motion Boxes → TrackedObjects (with IDs, velocity, etc.)        │
│                                                                       │
│  3. MOTION REGION CONSOLIDATOR                                       │
│     TrackedObjects → Consolidated Regions (640x640 for YOLO)        │
│                                                                       │
│  4. YOLO11 CLASSIFICATION                                            │
│     Consolidated Regions → Bird Species                              │
│                                                                       │
└─────────────────────────────────────────────────────────────────────┘
```

## Processing Pipeline

The `processFrame()` function executes these steps:

### Step 1: Preprocessing
```
Raw Frame → Grayscale → Contrast Enhancement → Blur → Preprocessed Frame
```

**Purpose**: Prepare frame for motion detection
- Convert to grayscale (faster processing)
- Optional CLAHE contrast enhancement (low-light scenes)
- Blur to reduce noise (gaussian/median/bilateral)

**Key Parameters**:
- `processingMode`: "grayscale" (default) or "rgb"
- `contrastEnhancement`: true/false
- `blurType`: "gaussian", "median", or "bilateral"

### Step 2: Motion Detection
```
Preprocessed Frame → Frame Differencing → Threshold → Motion Mask
                  ↓
            Background Subtraction (optional)
```

**Purpose**: Identify pixels that changed between frames

**Methods**:
1. **Frame Differencing** (always on)
   - Compare current frame with previous frame
   - Fast, good for sudden movements
   - `frameDiff = absdiff(current, previous)`

2. **Background Subtraction** (optional)
   - Compare current frame with learned background model
   - Good for slow-moving objects
   - Uses MOG2 algorithm
   - Enable via `backgroundSubtraction: true`

**Output**: Binary mask (white = motion, black = no motion)

### Step 3: Morphological Operations
```
Motion Mask → Close → Open → Dilate → Erode → Clean Mask
```

**Purpose**: Clean up noise and connect nearby motion regions

**Operations** (in order):
1. **Close**: Fill small holes in motion regions
2. **Open**: Remove small noise blobs
3. **Dilate**: Expand motion regions to connect nearby areas
4. **Erode**: Optional shrinking to counter over-expansion

**Key Parameters**:
- `morphKernelSize`: Size of kernel (default: 5)
- Individual on/off: `morphClose`, `morphOpen`, `dilation`, `erosion`

### Step 4: Contour Extraction & Filtering
```
Clean Mask → Find Contours → Filter by Area → Filter by Solidity → 
             Filter by Aspect Ratio → Motion Boxes (cv::Rect)
```

**Purpose**: Convert binary mask into discrete bounding rectangles

**Filters Applied**:
1. **Area Filter**: Remove tiny regions (noise)
2. **Solidity Filter**: Remove scattered/irregular shapes
3. **Aspect Ratio Filter**: Remove extremely elongated shapes

**Two Modes**:

#### Adaptive Mode (default)
- Dynamically calculates thresholds based on scene statistics
- Recalculates every N frames (default: 150 frames = 5 seconds)
- Adapts to different scenes automatically

#### Permissive Mode
- Uses fixed, loose thresholds
- Catches everything possible
- Lets consolidator do heavy filtering

## Configuration Parameters

### Image Preprocessing

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `processing_mode` | string | "grayscale" | Color space: "grayscale" or "rgb" |
| `contrast_enhancement` | bool | false | Enable CLAHE contrast enhancement |
| `clahe_clip_limit` | double | 2.0 | CLAHE contrast limit (1.0-4.0) |
| `clahe_tile_size` | int | 8 | CLAHE tile size (4-16) |
| `blur_type` | string | "gaussian" | Blur type: "gaussian", "median", "bilateral" |
| `gaussian_blur_size` | int | 5 | Gaussian kernel size (odd number) |
| `median_blur_size` | int | 5 | Median kernel size (odd number) |

### Motion Detection

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `background_subtraction` | bool | false | Enable MOG2 background subtraction |
| `max_threshold` | int | 255 | Maximum threshold value |

### Morphological Operations

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `morphology` | bool | true | Enable all morphological operations |
| `morph_kernel_size` | int | 5 | Kernel size for all operations |
| `morph_close` | bool | true | Fill holes in regions |
| `morph_open` | bool | true | Remove noise blobs |
| `dilation` | bool | true | Expand regions |
| `erosion` | bool | false | Shrink regions |

### Contour Detection

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `contour_detection_mode` | string | "adaptive" | "adaptive" or "permissive" |
| `convex_hull` | bool | true | Use convex hull for shape analysis |
| `contour_approximation` | bool | true | Simplify contour shapes |
| `contour_epsilon_factor` | double | 0.03 | Shape simplification amount (0.01-0.05) |
| `contour_filtering` | bool | true | Apply quality filters |

#### Adaptive Mode Parameters
These are calculated automatically but have safety bounds:
- `min_area`: 50 - 1000 pixels
- `min_solidity`: 0.2 - 0.8
- `max_aspect_ratio`: 2.0 - 15.0

#### Permissive Mode Parameters
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `permissive_min_area` | int | 50 | Minimum contour area in pixels |
| `permissive_min_solidity` | double | 0.1 | Minimum shape solidity (0.0-1.0) |
| `permissive_max_aspect_ratio` | double | 10.0 | Maximum width/height ratio |

## Adaptive Threshold Calculation

### How Adaptive Mode Works

The system analyzes contours every 150 frames and calculates optimal thresholds:

#### 1. Minimum Area (10th Percentile)
```
Example contour areas: [20, 30, 45, 50, 200, 250, 300, 500, 800]
10th percentile ≈ 45 pixels
→ Reject anything < 45 pixels as noise
```

**Logic**: The smallest 10% are likely noise

#### 2. Minimum Solidity (25th Percentile)
```
Solidity = contour_area / convex_hull_area

Example solidities: [0.2, 0.3, 0.5, 0.6, 0.7, 0.8, 0.9, 0.95]
25th percentile ≈ 0.5
→ Reject anything < 0.5 solidity as too scattered
```

**Logic**: The least solid 25% are likely false detections

#### 3. Maximum Aspect Ratio (90th Percentile)
```
Aspect Ratio = width / height

Example ratios: [1.0, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 8.0, 12.0]
90th percentile ≈ 8.0
→ Reject anything > 8.0 aspect ratio as too elongated
```

**Logic**: The most elongated 10% are likely artifacts (wires, shadows)

## Troubleshooting Guide

### Problem: Too Many False Detections (Noise)

**Symptoms**:
- Hundreds of tiny motion boxes
- Motion detected in static areas
- Excessive processing time

**Solutions**:

1. **Increase Blur**
   ```yaml
   gaussian_blur_size: 7  # or 9 (from default 5)
   ```

2. **Strengthen Morphological Operations**
   ```yaml
   morph_kernel_size: 7  # from default 5
   morph_open: true       # ensure this is on
   ```

3. **Use Stricter Thresholds (if permissive mode)**
   ```yaml
   permissive_min_area: 200  # from default 50
   permissive_min_solidity: 0.3  # from default 0.1
   ```

4. **Switch to Adaptive Mode**
   ```yaml
   contour_detection_mode: adaptive
   ```

### Problem: Missing Real Motion (Under-detection)

**Symptoms**:
- Birds not detected
- Only detecting very obvious motion
- Motion boxes too few

**Solutions**:

1. **Switch to Permissive Mode**
   ```yaml
   contour_detection_mode: permissive
   permissive_min_area: 50
   permissive_min_solidity: 0.1
   ```

2. **Enable Background Subtraction**
   ```yaml
   background_subtraction: true
   ```

3. **Reduce Morphological Filtering**
   ```yaml
   morph_open: false  # don't remove small regions
   erosion: false     # don't shrink regions
   ```

4. **Enhance Contrast (low-light scenes)**
   ```yaml
   contrast_enhancement: true
   clahe_clip_limit: 3.0
   ```

### Problem: Unstable Detection (Flickering)

**Symptoms**:
- Motion boxes appear/disappear rapidly
- Same bird detected intermittently
- Inconsistent across frames

**Solutions**:

1. **Increase Dilation**
   ```yaml
   dilation: true
   morph_kernel_size: 7  # larger kernel
   ```

2. **Enable Background Subtraction**
   ```yaml
   background_subtraction: true
   ```

3. **Reduce Blur** (if too much smoothing)
   ```yaml
   gaussian_blur_size: 3  # from default 5
   ```

### Problem: Detecting Shadows/Reflections

**Symptoms**:
- Long, thin motion boxes
- Motion in obviously non-bird areas
- High aspect ratio boxes

**Solutions**:

1. **Reduce Max Aspect Ratio**
   ```yaml
   permissive_max_aspect_ratio: 5.0  # from default 10.0
   ```

2. **Increase Solidity Requirement**
   ```yaml
   permissive_min_solidity: 0.3  # from default 0.1
   ```

3. **Use Adaptive Mode** (learns from scene)
   ```yaml
   contour_detection_mode: adaptive
   ```

## Output Format

### Motion Boxes (cv::Rect)
Each motion box contains:
- `x`: Left edge (pixels from left)
- `y`: Top edge (pixels from top)
- `width`: Box width in pixels
- `height`: Box height in pixels

### Log Output Example
```
=== MOTION DETECTION SUMMARY ===
Motion detected: 12 regions
Frame size: 1920x1080
Processing mode: grayscale
Background subtraction: disabled
=== END MOTION DETECTION SUMMARY ===

=== MOTION BOXES METADATA (Frame 123) ===
Detected 12 motion regions
Motion Box 0: BBox(100,200,50,60) | Center(125,230) | Area: 3000 | Aspect: 0.83
Motion Box 1: BBox(400,300,80,70) | Center(440,335) | Area: 5600 | Aspect: 1.14
...
=== END MOTION BOXES METADATA ===
```

## Performance Characteristics

| Frame Size | Objects | Processing Time | Bottleneck |
|-----------|---------|-----------------|------------|
| 1920x1080 | <10 | ~5ms | Frame differencing |
| 1920x1080 | 10-50 | ~10ms | Morphology + contours |
| 1920x1080 | 50-100 | ~20ms | Contour filtering |
| 1920x1080 | >100 | ~30ms+ | Adaptive calculations |

### Optimization Tips

1. **Use Grayscale** (not RGB)
   - 3x faster processing
   - Same motion detection quality

2. **Limit Morphology Kernel Size**
   - Keep at 5-7 pixels
   - Larger = slower but cleaner

3. **Use Adaptive Mode**
   - Recalculate only every 150 frames
   - Much faster than per-frame calculations

4. **Disable Unnecessary Features**
   - Turn off background subtraction if not needed
   - Disable contrast enhancement if lighting is good

## Integration with Next Stage

**Output → Motion Tracker**:
```cpp
// Motion Processor output
std::vector<cv::Rect> motionBoxes = processor.processFrame(frame).detectedBounds;

// Motion Tracker input
std::vector<TrackedObject> trackedObjects = tracker.updateTracks(motionBoxes, currentFrameNumber);
```

The Motion Tracker adds:
- Persistent object IDs
- Velocity tracking
- Kalman filtering for smooth trajectories
- Track lifecycle management

## Related Files

- `motion_processor.hpp` - Class definition
- `motion_tracker.cpp` - Next stage (adds IDs and tracking)
- `motion_region_consolidator.cpp` - Final stage (groups for YOLO)
- `config.yaml` - Configuration file

## Debugging Checklist

When troubleshooting motion detection issues:

1. ✅ Check log output for detection counts
2. ✅ Visualize each processing step (use `saveProcessingVisualization`)
3. ✅ Verify config parameters loaded correctly
4. ✅ Test with simpler scene (single bird)
5. ✅ Compare adaptive vs permissive modes
6. ✅ Check if background subtraction helps
7. ✅ Examine contour statistics in logs

---
*Last Updated: Based on commented version with detailed inline documentation*

