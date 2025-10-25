# Quick Recording Steps

## 🎬 Record the DBSCAN Demo in 5 Minutes

### Step 1: Open Terminal & Prepare (30 seconds)
```bash
cd /Users/beuxb/Desktop/Projects/birds_of_play
source venv/bin/activate
brew services start mongodb-community
```

### Step 2: Get Ready to Record (30 seconds)
- Press **Cmd+Shift+5** on your Mac
- Click **"Record Selected Portion"**
- Don't select anything yet - just have it ready

### Step 3: Start the Pipeline (1 minute)
```bash
python test/full_pipeline_test.py
```

Wait for the OpenCV window to pop up showing the video with bounding boxes

### Step 4: Record! (20 seconds)
1. When the OpenCV window appears:
   - Use the crosshair from Step 2 to select the OpenCV window
   - Click **"Record"**
   
2. **Watch for these visuals** (capture 15-20 seconds):
   - Gray boxes appearing around individual motion
   - Red boxes forming to consolidate nearby regions
   - Multiple regions tracking together
   
3. Press **Stop button** in menu bar (or Cmd+Control+Esc)

4. Press **'q'** in the OpenCV window to stop the pipeline

### Step 5: Process the Video (2 minutes)
Your recording is saved to Desktop as "Screen Recording YYYY-MM-DD at...mov"

```bash
# Navigate to Desktop
cd ~/Desktop

# Find your recording
ls -t "Screen Recording"* | head -n 1

# Compress and trim to best 12 seconds
ffmpeg -i "Screen Recording 2025-10-25 at....mov" \
       -ss 00:00:03 -t 00:00:12 \
       -vf "scale=1280:-2" \
       -c:v libx264 -preset slow -crf 23 \
       -pix_fmt yuv420p \
       /Users/beuxb/Desktop/Projects/birds_of_play/public/videos/demo.mp4
```

Replace the filename with your actual recording name!

### Step 6: Deploy (1 minute)
```bash
cd /Users/beuxb/Desktop/Projects/birds_of_play
git add public/videos/demo.mp4 api/index.js RECORDING_GUIDE.md QUICK_RECORDING_STEPS.md record_demo.sh
git commit -m "feat: add live DBSCAN demo video with visualization"
git push origin main
```

Done! Your live demo video will show on the Vercel landing page! 🎉

---

## 💡 Tips for Best Results

### ✅ Good Demo Shows:
- Birds flying across frame
- Gray boxes tracking individual motion
- Red boxes consolidating overlapping regions
- Multiple consolidated regions at once

### 🎯 Perfect Timing:
- Skip first 2-3 seconds (system warming up)
- Capture seconds 3-15 (best action)
- Total video: 10-15 seconds max

### ⚙️ If Video is Too Large:
```bash
# More aggressive compression
ffmpeg -i input.mov -vf "scale=960:-2" -c:v libx264 -crf 28 output.mp4
```

### 🔍 Preview Before Deploying:
```bash
# Open the processed video
open public/videos/demo.mp4
```

Make sure it shows the DBSCAN clustering action clearly!

---

## 🚨 Troubleshooting

**OpenCV window doesn't appear:**
```bash
# Try running the C++ binary directly
./src/motion_detection/BirdsOfPlay test/vid/vid_4.mov
```

**MongoDB error:**
```bash
brew services restart mongodb-community
```

**Video too dark/unclear:**
- Adjust your screen brightness before recording
- Make the OpenCV window larger
- Try a different section of the test video

**File too large (>5MB):**
```bash
# Check file size
ls -lh public/videos/demo.mp4

# Re-compress if needed
ffmpeg -i public/videos/demo.mp4 -crf 28 public/videos/demo_compressed.mp4
mv public/videos/demo_compressed.mp4 public/videos/demo.mp4
```

