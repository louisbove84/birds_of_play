# Config Variable Usage Analysis

This analysis shows which variables in `config.yaml` are actually being used by the codebase and which are unused.

## ✅ **USED CONFIG VARIABLES**

### **Logging Configuration (main.cpp)**
- `logging.log_level` ✅ Used
- `logging.log_to_file` ✅ Used  
- `logging.log_file_path` ✅ Used

### **Data Collection (data_collector.cpp)**
- `data_collection` ✅ Used
- `cleanup_old_data` ✅ Used
- `mongodb_uri` ✅ Used
- `database_name` ✅ Used
- `collection_prefix` ✅ Used
- `image_format` ✅ Used
- `min_tracking_confidence` ✅ Used

### **Motion Processor (motion_processor.cpp)**
**Note:** The motion processor looks for different config key names than what's in the config file!

**Used but with DIFFERENT key names:**
- `max_threshold` (looks for this, but config has `max_threshold: 255`)
- `gaussian_blur_size` (looks for this, config has `gaussian_blur_size: 7`)
- `processing_mode` (looks for this, config has `processing_mode: "grayscale"`)
- `clahe_clip_limit` (looks for this, config has `clahe_clip_limit: 3.0`)
- `clahe_tile_size` (looks for this, config has `clahe_tile_size: 8`)
- `bilateral_d` (looks for this, config has `bilateral_d: 15`)
- `bilateral_sigma_color` (looks for this, config has `bilateral_sigma_color: 75`)
- `bilateral_sigma_space` (looks for this, config has `bilateral_sigma_space: 75`)
- `convex_hull` (looks for this, config has `convex_hull: true`)
- `contour_approximation` (looks for this, config has `contour_approximation: true`)
- `contour_epsilon_factor` (looks for this, config has `contour_epsilon_factor: 0.03`)
- `contour_filtering` (looks for this, config has `contour_filtering: true`)

**Used but MISSING from config file:**
- `morphology_kernel_size` (looks for this, config has `morph_kernel_size: 7`)
- `enable_morphology` (looks for this, config has `morphology: true`)
- `enable_dilation` (looks for this, config has `dilation: true`)
- `enable_morph_close` (looks for this, config has `morph_close: true`)
- `enable_morph_open` (looks for this, config has `morph_open: true`)
- `enable_erosion` (looks for this, config has `erosion: false`)
- `enable_contrast_enhancement` (looks for this, config has `contrast_enhancement: true`)
- `enable_background_subtraction` (looks for this, config has `background_subtraction: true`)
- `median_blur_size` (looks for this, config has `median_blur_size: 5`)
- `enable_median_blur` (looks for this, not in config)
- `enable_bilateral_filter` (looks for this, not in config)
- `contour_detection_mode` (looks for this, not in config)
- `permissive_min_area` (looks for this, not in config)
- `permissive_min_solidity` (looks for this, not in config)
- `permissive_max_aspect_ratio` (looks for this, not in config)
- `adaptive_update_interval` (looks for this, not in config)

## ❌ **UNUSED CONFIG VARIABLES**

### **HSV Color Filtering** (All unused - no code loads these)
- `hsv_lower_h: 0`
- `hsv_lower_s: 30`
- `hsv_lower_v: 60`
- `hsv_upper_h: 20`
- `hsv_upper_s: 150`
- `hsv_upper_v: 255`

### **Image Preprocessing** (Partially unused)
- `blur_type: "gaussian"` ❌ Unused
- `threshold_type: "binary"` ❌ Unused
- `threshold_value: 40` ❌ Unused
- `adaptive_block_size: 11` ❌ Unused
- `adaptive_c: 2` ❌ Unused

### **Motion Detection Methods** (Mostly unused)
- `background_subtraction_method: "MOG2"` ❌ Unused
- `optical_flow_mode: "none"` ❌ Unused
- `motion_history_duration: 0` ❌ Unused
- `background_history: 300` ❌ Unused
- `background_threshold: 40` ❌ Unused
- `background_detect_shadows: false` ❌ Unused
- `pbas_history: 500` ❌ Unused
- `pbas_threshold: 25.0` ❌ Unused
- `pbas_learning_rate: 0.01` ❌ Unused
- `pbas_detect_shadows: true` ❌ Unused

### **Contour Processing** (Mostly unused)
- `min_contour_area: 200` ❌ Unused (hardcoded in hpp as 100)
- `max_contour_aspect_ratio: 3.0` ❌ Unused (hardcoded in hpp as 5.0)
- `min_contour_solidity: 0.3` ❌ Unused (hardcoded in hpp as 0.2)

### **Edge Detection** (Completely unused)
- `canny_low_threshold: 50` ❌ Unused
- `canny_high_threshold: 150` ❌ Unused

### **Object Tracking** (All unused - motion tracker was removed)
- `max_tracking_distance: 200` ❌ Unused
- `max_trajectory_points: 30` ❌ Unused
- `min_trajectory_length: 25` ❌ Unused
- `spatial_merging: true` ❌ Unused
- `spatial_merge_distance: 100` ❌ Unused
- `spatial_merge_overlap_threshold: 0.1` ❌ Unused
- `motion_clustering: true` ❌ Unused
- `motion_similarity_threshold: 0.5` ❌ Unused
- `motion_history_frames: 3` ❌ Unused
- `smoothing_factor: 0.7` ❌ Unused

### **Object Classification** (All unused - no code loads these)
- `enable_classification: true` ❌ Unused
- `model_path: "models/squeezenet.onnx"` ❌ Unused
- `labels_path: "models/imagenet_labels.txt"` ❌ Unused
- `classification_confidence_threshold: 0.3` ❌ Unused
- `classification_interval: 10` ❌ Unused

### **Visualization & Output** (All unused)
- `split_screen: true` ❌ Unused
- `draw_contours: true` ❌ Unused
- `save_on_motion: true` ❌ Unused
- `split_screen_window_name: "..."` ❌ Unused
- `debug.split_screen: true` ❌ Unused

## 🔧 **RECOMMENDATIONS**

1. **Fix Config Key Mismatches**: Update motion_processor.cpp to use the correct config keys
2. **Remove Unused Variables**: Clean up ~70% of unused config variables
3. **Add Missing Variables**: Add the missing config variables that the code expects
4. **Simplify Config**: Create a minimal config with only used variables

## 📊 **SUMMARY**
- **Total Config Variables**: ~80
- **Actually Used**: ~15 (19%)
- **Unused**: ~65 (81%)
- **Key Mismatches**: ~12 variables have wrong key names
