#!/usr/bin/env python3
"""
Birds of Play - Motion Detection and YOLO11 Integration System
Run with virtual environment: source venv/bin/activate && python src/main.py
"""
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
from datetime import datetime, timezone
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
    """Setup MongoDB connection and frame database"""
    try:
        from mongodb.database_manager import DatabaseManager
        from mongodb.frame_database import FrameDatabase
        
        # Initialize database manager
        db_manager = DatabaseManager()
        
        # Connect to database
        if not db_manager.connect():
            print("❌ Failed to connect to MongoDB")
            return None, None
        
        # Initialize frame database
        frame_db = FrameDatabase(db_manager)
        
        # Create indexes for better performance
        frame_db.create_indexes()
        
        print("✅ MongoDB connection established")
        print(f"📊 Frame database initialized: {frame_db.get_frame_count()} frames")
        
        return db_manager, frame_db
        
    except ImportError as e:
        print(f"⚠️  MongoDB modules not available: {e}")
        print("Make sure MongoDB dependencies are installed")
        return None, None
    except Exception as e:
        print(f"❌ MongoDB connection failed: {e}")
        return None, None

def run_yolo_on_mongodb_frames(frame_db):
    """Run YOLO11 on consolidated motion regions from MongoDB frames"""
    print("🔍 Birds of Play - YOLO11 Analysis on Motion Regions")
    print("=" * 50)
    
    if frame_db is None:
        print("❌ No MongoDB connection available")
        return False
    
    try:
        # Get frame metadata from MongoDB (without frame data initially)
        frame_metadata_list = frame_db.list_frames(limit=1000)  # Get up to 1000 frames
        if not frame_metadata_list:
            print("📭 No frames found in MongoDB")
            return True
        
        print(f"📊 Found {len(frame_metadata_list)} frames in MongoDB")
        
        # Filter frames that have consolidated regions
        frames_with_regions = []
        total_regions = 0
        
        for frame_metadata in frame_metadata_list:
            metadata = frame_metadata.get('metadata', {})
            regions = metadata.get('consolidated_regions', [])
            if regions:
                # Get the actual frame data for frames with regions
                frame_uuid = frame_metadata.get('uuid') or frame_metadata.get('_id')
                if frame_uuid:
                    frame_image = frame_db.get_frame(frame_uuid)
                    if frame_image is not None:
                        frame_data = {
                            'uuid': frame_uuid,
                            'metadata': metadata,
                            'image': frame_image,
                            'timestamp': frame_metadata.get('timestamp')
                        }
                        frames_with_regions.append(frame_data)
                        total_regions += len(regions)
                    else:
                        print(f"   ⚠️  Could not retrieve image data for frame {str(frame_uuid)[:8]}...")
                else:
                    print(f"   ⚠️  No UUID found for frame with regions")
        
        if not frames_with_regions:
            print("📭 No frames with consolidated motion regions found")
            return True
            
        print(f"🎯 Found {len(frames_with_regions)} frames with {total_regions} motion regions")
        print("🚀 Starting YOLO11 analysis on motion regions only...")
        
        # Load YOLO11 model
        print("📦 Loading YOLO11 model...")
        try:
            from ultralytics import YOLO
            import numpy as np
            yolo_model = YOLO('yolo11n.pt')  # Lightweight model for real-time processing
            print("✅ YOLO11 model loaded successfully!")
        except Exception as e:
            print(f"❌ Failed to load YOLO11 model: {e}")
            return False
        
        for i, frame_data in enumerate(frames_with_regions):
            frame_uuid = frame_data.get('uuid', 'unknown')
            metadata = frame_data.get('metadata', {})
            regions = metadata.get('consolidated_regions', [])
            
            print(f"🔍 Processing frame {i+1}/{len(frames_with_regions)} (UUID: {frame_uuid[:8]}...) - {len(regions)} regions")
            
            # Process each region with YOLO11
            yolo_results = []
            
            # Get the image data from MongoDB (already a numpy array)
            full_frame = frame_data.get('image')
            if full_frame is None:
                print(f"   ❌ No image data found for frame {frame_uuid[:8]}...")
                continue
                
            # Verify it's a valid image
            if not isinstance(full_frame, np.ndarray) or full_frame.size == 0:
                print(f"   ❌ Invalid image data for frame {frame_uuid[:8]}...")
                continue
            
            for j, region in enumerate(regions):
                x, y, w, h = region['x'], region['y'], region['width'], region['height']
                print(f"   📦 Region {j+1}: ({x},{y},{w},{h}) - {region.get('object_count', 0)} objects")
                
                # Extract region from full frame
                region_image = full_frame[y:y+h, x:x+w]
                
                if region_image.size == 0:
                    print(f"   ⚠️  Empty region {j+1}, skipping...")
                    continue
                
                try:
                    # Run YOLO11 on the cropped region
                    region_results = yolo_model(region_image, verbose=False)
                    
                    # Process YOLO results
                    region_detections = []
                    for result in region_results:
                        boxes = result.boxes
                        if boxes is not None:
                            for box in boxes:
                                # Get box coordinates (relative to region)
                                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                                confidence = float(box.conf[0].cpu().numpy())
                                class_id = int(box.cls[0].cpu().numpy())
                                class_name = yolo_model.names[class_id]
                                
                                # Convert coordinates back to full frame
                                detection = {
                                    'x1': int(x1 + x),  # Adjust to full frame coordinates
                                    'y1': int(y1 + y),
                                    'x2': int(x2 + x),
                                    'y2': int(y2 + y),
                                    'confidence': confidence,
                                    'class_id': class_id,
                                    'class_name': class_name,
                                    'region_x': int(x1),  # Original region coordinates
                                    'region_y': int(y1),
                                    'region_w': int(x2 - x1),
                                    'region_h': int(y2 - y1)
                                }
                                region_detections.append(detection)
                    
                    print(f"   🔍 Found {len(region_detections)} objects in region {j+1}")
                    for detection in region_detections:
                        print(f"      - {detection['class_name']}: {detection['confidence']:.2f}")
                    
                    yolo_results.append({
                        'region_id': j,
                        'region_bounds': region,
                        'detections': region_detections,
                        'processed': True,
                        'detection_count': len(region_detections)
                    })
                    
                except Exception as e:
                    print(f"   ❌ YOLO processing failed for region {j+1}: {e}")
                    yolo_results.append({
                        'region_id': j,
                        'region_bounds': region,
                        'detections': [],
                        'processed': False,
                        'error': str(e)
                    })
            
            # Update frame metadata with YOLO results
            updated_metadata = metadata.copy()
            updated_metadata['yolo_analysis'] = {
                'processed': True,
                'timestamp': datetime.now(timezone.utc).isoformat(),
                'regions_processed': len(regions),
                'results': yolo_results
            }
            
            # Update MongoDB with YOLO results (TODO: Implement update_frame_metadata)
            try:
                total_detections = sum(result.get('detection_count', 0) for result in yolo_results)
                print(f"   💾 YOLO analysis complete: {total_detections} total detections")
                # TODO: Implement frame_db.update_frame_metadata(frame_uuid, updated_metadata)
                print(f"   📊 Results: {len(yolo_results)} regions processed")
            except Exception as e:
                print(f"   ❌ Error processing YOLO results for frame {frame_uuid[:8]}...: {e}")
            
            print(f"   ✅ Processed {len(regions)} regions for frame {frame_uuid[:8]}...")
        
        print(f"✅ YOLO11 analysis completed! Processed {total_regions} motion regions across {len(frames_with_regions)} frames")
        return True
        
    except Exception as e:
        print(f"❌ Error running YOLO11 analysis: {e}")
        return False

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

def run_motion_detection_demo(frame_db=None):
    """Run the C++ motion detection demo with webcam and direct MongoDB integration"""
    print("🐦 Birds of Play - C++ Motion Detection Demo with Direct MongoDB")
    print("=" * 50)
    
    if not check_build():
        return False
    
    executable_path = os.path.join("build", "birds_of_play")
    config_path = os.path.join("src", "motion_detection", "config.yaml")
    
    print(f"📹 Starting motion detection with webcam...")
    print(f"🔧 Using config: {config_path}")
    print(f"⚙️  Executable: {executable_path}")
    if frame_db:
        print(f"💾 MongoDB integration: DIRECT C++ → MongoDB (no file I/O)")
        print(f"🚀 Frames saved automatically every second via C++ pybind11")
    print("\n⌨️  Controls:")
    print("   'q' or ESC - Quit application")
    print("   's' - Save current frame with detections")
    print("\n🔍 Watch for:")
    print("   🟢 Green/Blue/Red boxes - Individual motion detections")
    print("   ⬜ White boxes - Consolidated motion regions")
    print("   💾 MongoDB save confirmations in output")
    print("\n" + "=" * 50)
    
    try:
        # Run the C++ executable with direct MongoDB integration
        cmd = [executable_path, config_path]
        print(f"🚀 Running: {' '.join(cmd)}")
        print("📊 C++ will handle MongoDB saves directly - no Python file monitoring needed!")
        
        # Start the process
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1
        )
        
        # Print output in real-time (C++ handles MongoDB directly)
        print("\n📺 Motion detection window should open...")
        print("📊 Live output:")
        print("-" * 30)
        
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                output_line = output.strip()
                print(output_line)
                
                # No file monitoring needed - C++ saves directly to MongoDB!
                # Just print the output for user feedback

        
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

def run_with_video_file(video_path, frame_db=None):
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
    if frame_db:
        print(f"💾 MongoDB integration: DIRECT C++ → MongoDB (no file I/O)")
        print(f"🚀 Frames saved automatically via C++ pybind11")
    
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
        description="Birds of Play - Motion Detection System (requires --video or --webcam)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python src/main.py --webcam                    # Live webcam detection
  python src/main.py --video input.mp4           # Process video file
  python src/main.py --webcam --mongo            # With direct C++ → MongoDB
  python src/main.py --python-bindings --webcam  # Use Python bindings
  python src/main.py --mongo --yolo              # Run YOLO11 on MongoDB frames
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
    parser.add_argument('--yolo', action='store_true',
                       help='Run YOLO11 analysis on MongoDB frames (requires --mongo)')
    
    args = parser.parse_args()
    
    # Print banner
    print("🐦 Birds of Play - Motion Detection System")
    print("=" * 60)
    print("🎯 Main Application Entry Point")
    print("🔧 Integrating C++, Python, and MongoDB")
    print("=" * 60)
    
    
    # Setup MongoDB if requested
    db_manager = None
    frame_db = None
    if args.mongo:
        print("\n📊 Setting up MongoDB connection...")
        db_manager, frame_db = setup_mongodb_connection()
    
    # Determine video source
    video_source = None
    if args.video:
        video_source = args.video
        if not os.path.exists(video_source):
            print(f"❌ Video file not found: {video_source}")
            return 1
    elif args.webcam:
        video_source = None  # Use webcam
    elif not args.yolo:
        # Require explicit video source specification (unless running YOLO-only mode)
        print("❌ Error: Must specify either --video <path> or --webcam")
        print("   Use --help for usage information")
        return 1
    
    # Validate YOLO option
    if args.yolo and not args.mongo:
        print("❌ --yolo requires --mongo to be enabled")
        return 1
    
    # Run appropriate mode
    success = False
    if args.yolo:
        print("\n🔍 Running YOLO11 Analysis Mode...")
        success = run_yolo_on_mongodb_frames(frame_db)
    elif args.python_bindings:
        print("\n🐍 Running in Python Bindings Mode...")
        success = run_with_python_bindings(video_source)
    else:
        print("\n⚙️  Running in C++ Executable Mode...")
        success = run_with_video_file(video_source, frame_db)
    
    
    # Cleanup MongoDB connection
    if db_manager:
        db_manager.disconnect()
        print("📊 MongoDB connection closed")
    
    if success:
        print("\n✅ Birds of Play completed successfully!")
        return 0
    else:
        print("\n❌ Birds of Play encountered an error")
        return 1

if __name__ == "__main__":
    sys.exit(main())
