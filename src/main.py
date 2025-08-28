#!/usr/bin/env python3
"""
Birds of Play - Main Application Entry Point
===========================================

This is the main orchestrator for the Birds of Play system, integrating:
- C++ Motion Detection Engine
- Python Bindings and Utilities  
- MongoDB Database Integration
- Webcam and Video Processing
- Real-time Motion Tracking

Usage:
    python src/main.py [options]

Options:
    --webcam          Run live webcam motion detection
    --video <path>    Process video file
    --config <path>   Use custom config file
    --mongo           Enable MongoDB logging
    --python-bindings Use Python bindings instead of C++ executable
"""

import subprocess
import sys
import os
import time
import argparse
import json
from datetime import datetime
from pathlib import Path

def check_build():
    """Check if the C++ executable is built"""
    executable_path = os.path.join("build", "birds_of_play")
    if not os.path.exists(executable_path):
        print(f"❌ C++ executable not found at: {executable_path}")
        print("Please build the project first:")
        print("  mkdir -p build && cd build")
        print("  cmake .. -DBUILD_TESTING=ON")
        print("  make -j$(nproc)")
        return False
    return True

def setup_mongodb_connection():
    """Setup MongoDB connection for motion detection logging"""
    try:
        import pymongo
        from pymongo import MongoClient
        
        # MongoDB connection settings
        client = MongoClient('mongodb://localhost:27017/')
        db = client['birds_of_play']
        motion_collection = db['motion_detections']
        
        print("✅ MongoDB connection established")
        return motion_collection
    except ImportError:
        print("⚠️  PyMongo not installed. Install with: pip install pymongo")
        return None
    except Exception as e:
        print(f"❌ MongoDB connection failed: {e}")
        return None

def log_motion_to_mongodb(collection, frame_data):
    """Log motion detection data to MongoDB"""
    if collection is None:
        return
    
    try:
        # Add timestamp
        frame_data['timestamp'] = datetime.utcnow()
        frame_data['created_at'] = datetime.utcnow()
        
        # Insert into MongoDB
        result = collection.insert_one(frame_data)
        return result.inserted_id
    except Exception as e:
        print(f"❌ MongoDB logging failed: {e}")

def run_with_python_bindings(video_source=None):
    """Run motion detection using Python bindings"""
    print("🐍 Birds of Play - Python Bindings Mode")
    print("=" * 50)
    
    try:
        import birds_of_play_python
        import cv2
        import numpy as np
        
        # Create motion processor
        processor = birds_of_play_python.MotionProcessor()
        
        if video_source is None:
            # Use webcam
            cap = cv2.VideoCapture(0)
            print("📹 Using webcam")
        else:
            # Use video file
            cap = cv2.VideoCapture(video_source)
            print(f"🎬 Processing video: {video_source}")
        
        if not cap.isOpened():
            print("❌ Could not open video source")
            return False
        
        print("⌨️  Press 'q' to quit, 's' to save frame")
        
        frame_count = 0
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            # Process frame with motion detection
            processed_frame = processor.process_frame(frame)
            
            # Convert back to BGR for display
            if len(processed_frame.shape) == 3 and processed_frame.shape[2] == 1:
                processed_frame = cv2.cvtColor(processed_frame, cv2.COLOR_GRAY2BGR)
            
            # Display both original and processed frames
            combined = np.hstack([frame, processed_frame])
            cv2.imshow('Birds of Play - Python Bindings (Original | Processed)', combined)
            
            frame_count += 1
            if frame_count % 30 == 0:
                print(f"📊 Processed {frame_count} frames...")
            
            # Check for quit key
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        
        cap.release()
        cv2.destroyAllWindows()
        print(f"✅ Python bindings processing complete. Processed {frame_count} frames.")
        return True
        
    except ImportError as e:
        print(f"❌ Failed to import birds_of_play_python: {e}")
        print("Make sure to build the Python bindings first")
        return False
    except Exception as e:
        print(f"❌ Error in Python bindings mode: {e}")
        return False

def run_motion_detection_demo():
    """Run the C++ motion detection demo with webcam"""
    print("🐦 Birds of Play - C++ Motion Detection Demo")
    print("=" * 50)
    
    if not check_build():
        return False
    
    executable_path = os.path.join("build", "birds_of_play")
    config_path = os.path.join("src", "motion_detection", "config.yaml")
    
    print(f"📹 Starting motion detection with webcam...")
    print(f"🔧 Using config: {config_path}")
    print(f"⚙️  Executable: {executable_path}")
    print("\n⌨️  Controls:")
    print("   'q' or ESC - Quit application")
    print("   's' - Save current frame with detections")
    print("\n🔍 Watch for:")
    print("   🟢 Green/Blue/Red boxes - Individual motion detections")
    print("   ⬜ White boxes - Consolidated motion regions")
    print("\n" + "=" * 50)
    
    try:
        # Run the C++ executable
        cmd = [executable_path, config_path]
        print(f"🚀 Running: {' '.join(cmd)}")
        
        # Start the process
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1
        )
        
        # Print output in real-time
        print("\n📺 Motion detection window should open...")
        print("📊 Live output:")
        print("-" * 30)
        
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                print(output.strip())
        
        # Wait for process to finish
        return_code = process.poll()
        
        if return_code == 0:
            print("\n✅ Motion detection demo completed successfully!")
        else:
            print(f"\n❌ Motion detection demo exited with code: {return_code}")
            
    except KeyboardInterrupt:
        print("\n⏹️  Demo interrupted by user")
        if 'process' in locals():
            process.terminate()
            process.wait()
    except Exception as e:
        print(f"\n❌ Error running motion detection demo: {e}")
        return False
    
    return True

def run_with_video_file(video_path):
    """Run motion detection on a video file instead of webcam"""
    print(f"🎬 Birds of Play - Motion Detection on Video: {video_path}")
    print("=" * 50)
    
    if not check_build():
        return False
    
    if not os.path.exists(video_path):
        print(f"❌ Video file not found: {video_path}")
        return False
    
    executable_path = os.path.join("build", "birds_of_play")
    config_path = os.path.join("src", "motion_detection", "config.yaml")
    
    print(f"📹 Processing video: {video_path}")
    print(f"🔧 Using config: {config_path}")
    
    try:
        # Run the C++ executable with video file
        cmd = [executable_path, config_path, video_path]
        print(f"🚀 Running: {' '.join(cmd)}")
        
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1
        )
        
        print("\n📊 Processing output:")
        print("-" * 30)
        
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                print(output.strip())
        
        return_code = process.poll()
        
        if return_code == 0:
            print("\n✅ Video processing completed successfully!")
        else:
            print(f"\n❌ Video processing exited with code: {return_code}")
            
    except KeyboardInterrupt:
        print("\n⏹️  Processing interrupted by user")
        if 'process' in locals():
            process.terminate()
            process.wait()
    except Exception as e:
        print(f"\n❌ Error processing video: {e}")
        return False
    
    return True

def main():
    """Main orchestrator function"""
    parser = argparse.ArgumentParser(
        description="Birds of Play - Motion Detection System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python src/main.py --webcam                    # Live webcam detection
  python src/main.py --video input.mp4           # Process video file
  python src/main.py --webcam --mongo            # With MongoDB logging
  python src/main.py --python-bindings --webcam  # Use Python bindings
  python src/main.py --config custom.yaml        # Custom config
        """
    )
    
    parser.add_argument('--webcam', action='store_true',
                       help='Run live webcam motion detection')
    parser.add_argument('--video', type=str, metavar='PATH',
                       help='Process video file')
    parser.add_argument('--config', type=str, metavar='PATH',
                       help='Use custom config file')
    parser.add_argument('--mongo', action='store_true',
                       help='Enable MongoDB logging')
    parser.add_argument('--python-bindings', action='store_true',
                       help='Use Python bindings instead of C++ executable')
    
    args = parser.parse_args()
    
    # Print banner
    print("🐦 Birds of Play - Motion Detection System")
    print("=" * 60)
    print("🎯 Main Application Entry Point")
    print("🔧 Integrating C++, Python, and MongoDB")
    print("=" * 60)
    
    # Setup MongoDB if requested
    mongo_collection = None
    if args.mongo:
        print("\n📊 Setting up MongoDB connection...")
        mongo_collection = setup_mongodb_connection()
    
    # Determine video source
    video_source = None
    if args.video:
        video_source = args.video
        if not os.path.exists(video_source):
            print(f"❌ Video file not found: {video_source}")
            return 1
    elif args.webcam:
        video_source = None  # Use webcam
    else:
        # Default to webcam if no source specified
        video_source = None
        print("📹 No video source specified, defaulting to webcam")
    
    # Run appropriate mode
    success = False
    if args.python_bindings:
        print("\n🐍 Running in Python Bindings Mode...")
        success = run_with_python_bindings(video_source)
    else:
        print("\n⚙️  Running in C++ Executable Mode...")
        if video_source is None:
            success = run_motion_detection_demo()
        else:
            success = run_with_video_file(video_source)
    
    if success:
        print("\n✅ Birds of Play completed successfully!")
        return 0
    else:
        print("\n❌ Birds of Play encountered an error")
        return 1

if __name__ == "__main__":
    sys.exit(main())
