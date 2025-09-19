# 🔬 Birds of Play - Clustering Configuration Guide

This guide explains how to tune the unsupervised ML bird clustering system using the `clustering_config.yaml` file.

## 📁 Configuration File Location

```
src/unsupervised_ml/clustering_config.yaml
```

## 🎯 Key Parameters to Tune

### 🔧 **Distance Thresholds (Most Important)**

These control how similar birds need to be to group them into the same species:

```yaml
clustering:
  ward_linkage:
    conservative_threshold: 50.0   # More species (~8 clusters)
    balanced_threshold: 75.0       # Balanced (~3 clusters) ✅ RECOMMENDED
    permissive_threshold: 90.0     # Fewer species (~2 clusters)
```

**🎛️ How to Adjust:**
- **Lower values** (30-60): More bird species, stricter grouping
- **Higher values** (80-120): Fewer bird species, looser grouping
- **Current optimal range**: 50-90 for ResNet50 2048D features

### 🎯 **Species Count Preferences**

Control what the system considers a "good" number of bird species:

```yaml
evaluation:
  ideal_species_range:
    min_species: 2          # Minimum reasonable species count
    max_species: 6          # Maximum reasonable species count
    sweet_spot_min: 2       # Ideal minimum (gets bonus)
    sweet_spot_max: 6       # Ideal maximum (gets bonus)
```

### 🔍 **Detection Sensitivity**

Control which bird detections to include in clustering:

```yaml
feature_extraction:
  min_confidence: 0.5       # Minimum YOLO confidence (0.0-1.0)
```

**📊 Recommendations:**
- **0.3-0.5**: Include more birds (may include false positives)
- **0.7-0.9**: Only high-confidence birds (may miss some real birds)
- **0.5**: Good balance ✅

## 🧪 **Experimental Tuning Process**

### Step 1: Analyze Your Data
```bash
# Check how many bird objects you have
curl "http://localhost:3002/api/objects" | jq length
```

### Step 2: Test Different Thresholds
Edit `clustering_config.yaml` and restart the server:

```yaml
# For MORE species (if you have many different bird types):
ward_linkage:
  conservative_threshold: 30.0
  balanced_threshold: 50.0
  permissive_threshold: 70.0

# For FEWER species (if birds are very similar):
ward_linkage:
  conservative_threshold: 70.0
  balanced_threshold: 100.0
  permissive_threshold: 130.0
```

### Step 3: Restart and Test
```bash
# Kill old server
pkill -f cluster_server

# Start with new config
cd src/unsupervised_ml
source ../../venv_ml/bin/activate
python cluster_server.py &

# Initialize with new settings
curl "http://localhost:3002/initialize"

# Check results
open http://localhost:3002
```

## 📊 **Understanding the Results**

### Clustering Quality Indicators

1. **Silhouette Score** (-1 to +1):
   - **> 0.5**: Excellent clustering
   - **0.2-0.5**: Good clustering ✅
   - **< 0.2**: Poor clustering

2. **Species Count**:
   - **2-6 species**: Typical for local bird diversity ✅
   - **> 10 species**: Likely overfitting
   - **1 species**: Threshold too high

### Example Good Results
```
Ward balanced (75.0): 3 clusters, silhouette: 0.103 ✅
Ward permissive (90.0): 2 clusters, silhouette: 0.089 ✅
```

## 🎨 **Visual Tuning with Dendrogram**

The system provides dendrogram analysis to help choose optimal thresholds:

```bash
# Access dendrogram data
curl "http://localhost:3002/api/objects" | jq '.dendrogram_data.suggested_thresholds'
```

## ⚡ **Performance Tuning**

### For Large Datasets
```yaml
performance:
  max_objects_per_batch: 50     # Reduce if running out of memory
  batch_size: 16               # Smaller batches for limited GPU memory
  cache_features: true         # Cache to avoid recomputation
```

### For Faster Processing
```yaml
feature_extraction:
  model_name: "resnet18"       # Faster but less accurate than resnet50
  use_gpu: true               # Enable if GPU available
```

## 🧪 **Testing Configuration**

### Test Video Selection
```yaml
testing:
  test_video_path: "test/vid/vid_3.mov"    # Main test video
  alternative_videos:                      # Other available test videos
    - "test/vid/vid_1.mp4"
    - "test/vid/vid_2.mp4"
```

### Test Behavior
```yaml
testing:
  clear_database_before_test: true    # Clean MongoDB before each test
  save_test_results: true            # Keep test outputs for inspection
  test_timeout_seconds: 300          # Maximum test duration
```

**📝 How to Change Test Video:**
1. **Edit config file**: Update `test_video_path` in `clustering_config.yaml`
2. **Add new video**: Place video file in `test/vid/` directory
3. **Run tests**: The pipeline will automatically use the configured video

**💡 Pro Tip**: Use different videos for different test scenarios:
- `vid_1.mp4`: Short test video for quick validation
- `vid_2.mp4`: Longer video for performance testing  
- `vid_3.mov`: High-quality video for accuracy testing

## 🔄 **Common Adjustments**

### Too Many Species (Over-clustering)
```yaml
# Increase thresholds to merge similar birds
ward_linkage:
  balanced_threshold: 100.0    # Was 75.0
```

### Too Few Species (Under-clustering)  
```yaml
# Decrease thresholds to separate different birds
ward_linkage:
  balanced_threshold: 50.0     # Was 75.0
```

### Poor Clustering Quality
```yaml
# Try average linkage for irregular cluster shapes
# Or adjust confidence threshold
feature_extraction:
  min_confidence: 0.7          # Higher confidence = better quality
```

## 🚀 **Quick Start Examples**

### Conservative Setup (More Species)
```yaml
clustering:
  ward_linkage:
    conservative_threshold: 30.0
    balanced_threshold: 50.0
    permissive_threshold: 70.0
```

### Permissive Setup (Fewer Species)
```yaml
clustering:
  ward_linkage:
    conservative_threshold: 80.0
    balanced_threshold: 110.0
    permissive_threshold: 140.0
```

---

💡 **Pro Tip**: Start with the default values and adjust gradually. The system will automatically test all 6 methods and choose the best one based on your preferences!
