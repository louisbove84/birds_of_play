# Recording DBSCAN Motion Detection Demo

## Quick Start (Easiest Method)

### Step 1: Prepare Environment
```bash
cd /Users/beuxb/Desktop/Projects/birds_of_play
source venv/bin/activate
brew services start mongodb-community
```

### Step 2: Start Recording (macOS Built-in)
1. Press **Cmd+Shift+5** on your keyboard
2. Click "Record Selected Portion"
3. A crosshair appears - don't select anything yet
4. Keep this ready to go

### Step 3: Run the Demo
```bash
python test/full_pipeline_test.py
```

Wait for the OpenCV window to appear (titled "Motion Detection" or similar)

### Step 4: Capture the Demo
1. When the OpenCV window appears, quickly use your crosshair to select just that window
2. Click "Record"
3. **Let it run for 15-20 seconds** to capture:
   - Birds flying into frame
   - Gray boxes appearing (individual motion)
   - Red boxes forming (DBSCAN consolidation)
   - Multiple regions tracking together
4. Press **Stop** button in menu bar (or Cmd+Control+Esc)
5. Video saves to Desktop automatically

### Step 5: Process the Video
Find your recording (usually on Desktop, named like "Screen Recording 2025-10-25 at...mov")

**Trim to best clip (10-15 seconds):**
```bash
# Open in QuickTime, trim to best part, then:
ffmpeg -i "Screen Recording 2025-10-25 at....mov" \
       -ss 00:00:02 -t 00:00:12 \
       -vf "scale=1280:-2" \
       -c:v libx264 -preset slow -crf 23 \
       -pix_fmt yuv420p \
       public/videos/demo.mp4
```

- `-ss 00:00:02`: Start at 2 seconds (skip initial startup)
- `-t 00:00:12`: Take 12 seconds of video
- `scale=1280:-2`: Resize to 1280px width (optimized for web)
- `-crf 23`: Quality (lower = better, 18-28 recommended)

### Step 6: Deploy to Vercel
```bash
git add public/videos/demo.mp4
git commit -m "feat: add DBSCAN demo video"
git push origin main
```

The video is already referenced in `api/index.js` as `/videos/demo.mp4`

---

## What Makes a Good Demo Video

### ✅ Good Footage Shows:
- **Motion Detection**: Gray boxes tracking individual movements
- **DBSCAN Clustering**: Red boxes consolidating nearby motion
- **Overlap Awareness**: Small boxes near big boxes getting grouped
- **Multiple Regions**: 2-3 consolidated regions at once
- **Temporal Tracking**: Regions persisting across frames

### ❌ Avoid:
- Long periods with no motion
- Too much motion (frame too busy)
- Recording before system is ready
- Poor lighting or low contrast

### Ideal Scenario:
```
Frame 1-3:   Bird enters frame → Gray boxes appear
Frame 4-7:   More gray boxes → DBSCAN starts consolidating → Red box forms
Frame 8-15:  Multiple birds → Multiple red regions → Tracking in action
```

---

## Alternative Methods

### Using QuickTime Player
1. Open QuickTime Player
2. File → New Screen Recording
3. Click dropdown → Select microphone (optional)
4. Click record button → Select OpenCV window
5. Start demo: `python test/full_pipeline_test.py`
6. Capture 15-20 seconds
7. Cmd+Control+Esc to stop
8. File → Save

### Using FFmpeg Directly (Advanced)
```bash
# List available screens
ffmpeg -f avfoundation -list_devices true -i ""

# Record screen 1 for 20 seconds at 30fps
ffmpeg -f avfoundation -framerate 30 -i "1:0" -t 20 \
       -c:v libx264 -preset ultrafast -crf 18 \
       -pix_fmt yuv420p demo_raw.mov
```

---

## Troubleshooting

### OpenCV Window Doesn't Appear
- Make sure you're running the C++ binary, not just Python
- Check that visualization is enabled in config.yaml
- Try: `./src/motion_detection/BirdsOfPlay test/vid/vid_4.mov`

### Recording is Laggy/Choppy
- Close other applications
- Use "ultrafast" preset: `-preset ultrafast`
- Record at lower resolution initially
- Do compression/quality pass separately

### File Too Large
```bash
# Aggressive compression for web
ffmpeg -i input.mov -vf "scale=960:-2" -c:v libx264 -crf 28 output.mp4
```

### Need to Extract Best Clip
```bash
# Preview timestamps
ffplay input.mov

# Extract specific timerange
ffmpeg -i input.mov -ss 00:00:05 -to 00:00:18 -c copy clip.mov
```

---

## Technical Details

### What the Demo Shows

**Gray Boxes** (`cv::Scalar(200, 200, 200)`)
- Individual motion detections from `MotionProcessor`
- Each represents a contour/blob of motion
- May overlap, may be scattered

**Red Boxes** (`cv::Scalar(0, 0, 255)`)
- DBSCAN consolidated regions from `MotionRegionConsolidator`
- Uses overlap-aware distance metric
- Groups nearby/overlapping gray boxes
- Label shows: "Region:N (M objs)"

### Configuration
Edit `src/motion_detection/config.yaml` for:
- `eps`: DBSCAN epsilon (max distance for clustering)
- `min_pts`: Minimum points to form cluster
- `overlap_weight`: Weight for bounding box overlap
- `edge_weight`: Weight for edge-to-edge distance

### Video Format Specs
- **Container**: MP4 (H.264)
- **Resolution**: 1280x720 or 1920x1080
- **Framerate**: 30fps
- **Codec**: libx264
- **Pixel Format**: yuv420p (for browser compatibility)
- **Recommended Size**: < 5MB for fast loading

