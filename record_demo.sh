#!/bin/bash
# Record Birds of Play Motion Detection Demo Video
# =================================================
# This script helps you record the OpenCV visualization window showing
# DBSCAN clustering in action for the Vercel landing page demo.
#
# Requirements:
# 1. ffmpeg (for screen recording on macOS): brew install ffmpeg
# 2. Virtual environment activated: source venv/bin/activate
# 3. MongoDB running: brew services start mongodb-community
#
# Recording Process:
# 1. This script will start the full pipeline test
# 2. You'll need to manually start screen recording the OpenCV window
# 3. After ~15-20 seconds, press 'q' to stop the demo
# 4. The script will help you process the recorded video
#
# Usage:
#   chmod +x record_demo.sh
#   ./record_demo.sh

set -e  # Exit on error

echo "🎬 Birds of Play - Demo Video Recording Script"
echo "=============================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check prerequisites
echo -e "${BLUE}📋 Checking prerequisites...${NC}"

# Check if ffmpeg is installed
if ! command -v ffmpeg &> /dev/null; then
    echo -e "${RED}❌ ffmpeg is not installed${NC}"
    echo "Install with: brew install ffmpeg"
    exit 1
fi

# Check if virtual environment is activated
if [[ -z "$VIRTUAL_ENV" ]]; then
    echo -e "${RED}❌ Virtual environment not activated${NC}"
    echo "Run: source venv/bin/activate"
    exit 1
fi

# Check if MongoDB is running
if ! pgrep -x "mongod" > /dev/null; then
    echo -e "${YELLOW}⚠️  MongoDB not running, starting it...${NC}"
    brew services start mongodb-community
    sleep 3
fi

echo -e "${GREEN}✅ All prerequisites met${NC}"
echo ""

# Recording options
echo -e "${BLUE}🎥 Recording Options:${NC}"
echo "1. Use macOS built-in screen recording (Cmd+Shift+5)"
echo "2. Use QuickTime Player screen recording"
echo "3. Use ffmpeg screen recording (advanced)"
echo ""
echo -e "${YELLOW}Recommendation:${NC} Use option 1 (macOS built-in) for easiest setup"
echo ""

read -p "Choose recording method (1-3) or press Enter for option 1: " recording_method
recording_method=${recording_method:-1}

echo ""
echo -e "${BLUE}📹 Recording Instructions:${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

case $recording_method in
    1)
        echo "1. Press Cmd+Shift+5 to open screen recording"
        echo "2. Select 'Record Selected Portion'"
        echo "3. Position the capture area over the OpenCV window"
        echo "   (It will appear with title 'Motion Detection')"
        echo "4. Click 'Record' to start"
        echo "5. When the script starts the demo, capture 15-20 seconds"
        echo "6. Press the Stop button in menu bar when done"
        echo "7. The video will be saved to Desktop"
        ;;
    2)
        echo "1. Open QuickTime Player"
        echo "2. File → New Screen Recording"
        echo "3. Click the record button and select the OpenCV window"
        echo "4. When the script starts the demo, capture 15-20 seconds"
        echo "5. Press Cmd+Control+Esc to stop recording"
        echo "6. File → Save to save the video"
        ;;
    3)
        echo "FFmpeg will automatically record the main display"
        echo "Position the OpenCV window prominently on screen"
        echo "Recording will start automatically when demo begins"
        ;;
esac

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

read -p "Press Enter when you're ready to start the demo..."
echo ""

# If using ffmpeg, start recording
if [ "$recording_method" = "3" ]; then
    echo -e "${BLUE}🔴 Starting ffmpeg screen recording...${NC}"
    # Record main display at 30fps for 20 seconds
    ffmpeg -f avfoundation -framerate 30 -i "1:0" -t 20 \
           -c:v libx264 -preset ultrafast -crf 18 \
           -pix_fmt yuv420p \
           "demo_recording_$(date +%Y%m%d_%H%M%S).mov" &
    FFMPEG_PID=$!
    sleep 2
fi

echo -e "${GREEN}🚀 Starting Birds of Play demo...${NC}"
echo ""
echo -e "${YELLOW}⏱️  Let it run for 15-20 seconds to capture good footage${NC}"
echo -e "${YELLOW}⌨️  Press 'q' in the OpenCV window to stop${NC}"
echo ""

# Run the full pipeline test (motion detection only, skip ML training)
# This will display the OpenCV window with DBSCAN visualization
python test/full_pipeline_test.py --skip-video 2>&1 | head -n 50 || true

# If using ffmpeg, stop recording
if [ "$recording_method" = "3" ]; then
    echo ""
    echo -e "${BLUE}⏹️  Stopping ffmpeg recording...${NC}"
    kill $FFMPEG_PID 2>/dev/null || true
    wait $FFMPEG_PID 2>/dev/null || true
fi

echo ""
echo -e "${GREEN}✅ Demo completed!${NC}"
echo ""

# Post-processing instructions
echo -e "${BLUE}📝 Post-Processing Steps:${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1. Find your recorded video file:"
if [ "$recording_method" = "3" ]; then
    echo "   $(ls -t demo_recording_*.mov 2>/dev/null | head -n 1)"
else
    echo "   Check your Desktop or Movies folder"
fi
echo ""
echo "2. Trim to best 10-15 seconds showing:"
echo "   - Birds entering frame"
echo "   - Gray boxes (motion detection) appearing"
echo "   - Red boxes (DBSCAN consolidation) forming"
echo "   - Multiple regions being tracked"
echo ""
echo "3. Compress for web (recommended):"
echo "   ffmpeg -i input.mov -vf \"scale=1280:-2\" -c:v libx264 -preset slow -crf 23 demo.mp4"
echo ""
echo "4. Copy to Vercel public directory:"
echo "   cp demo.mp4 public/videos/demo.mp4"
echo ""
echo "5. Update api/index.js to use the demo video"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo -e "${GREEN}🎉 Recording process complete!${NC}"

