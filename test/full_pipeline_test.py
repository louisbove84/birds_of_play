#!/usr/bin/env python3
"""
Birds of Play - Full Pipeline Test Script
========================================

This script tests the complete Birds of Play pipeline:
1. Video Input → C++ Motion Detection → MongoDB
2. MongoDB → YOLO11 Analysis → Updated Results
3. Verification of all pipeline stages

Usage:
    cd /Users/beuxb/Desktop/Projects/birds_of_play
    source venv/bin/activate
    python test/full_pipeline_test.py
"""

import subprocess
import sys
import os
import time
import json
from pathlib import Path

# Add the src directory to Python path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src', 'unsupervised_ml'))
from config_loader import load_clustering_config

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src'))

def print_banner(title):
    """Print a formatted banner"""
    print("\n" + "=" * 60)
    print(f"🦅 {title}")
    print("=" * 60)

def print_step(step_num, title):
    """Print a formatted step header"""
    print(f"\n🔸 Step {step_num}: {title}")
    print("-" * 40)

def check_prerequisites():
    """Check if all prerequisites are met"""
    print_step(0, "Checking Prerequisites")
    
    # Check if we're in the right directory
    if not os.path.exists('src/main.py'):
        print("❌ Error: Must run from birds_of_play root directory")
        return False
    
    # Load configuration
    try:
        config = load_clustering_config()
        video_path = config.test_video_path
        print(f"📋 Using test video from config: {video_path}")
    except Exception as e:
        print(f"⚠️  Warning: Could not load config ({e}), using default")
        video_path = "test/vid/vid_3.mov"
    
    # Check if video file exists
    if not os.path.exists(video_path):
        print(f"❌ Error: Video file not found: {video_path}")
        return False
    
    # Check if C++ executable exists
    if not os.path.exists('build/birds_of_play'):
        print("❌ Error: C++ executable not found. Run 'make build' first.")
        return False
    
    # Check if virtual environment is activated
    if not hasattr(sys, 'real_prefix') and not (hasattr(sys, 'base_prefix') and sys.base_prefix != sys.prefix):
        print("❌ Error: Virtual environment not activated. Run 'source venv/bin/activate' first.")
        return False
    
    print("✅ All prerequisites met")
    return True

def clear_mongodb():
    """Clear MongoDB collection"""
    print_step(1, "Clearing MongoDB")
    
    try:
        result = subprocess.run([
            'mongosh', 'birds_of_play', '--eval', 
            'db.captured_frames.deleteMany({}); print("Cleared", db.captured_frames.countDocuments({}), "frames");'
        ], capture_output=True, text=True, timeout=10)
        
        if result.returncode == 0:
            print("✅ MongoDB cleared successfully")
            return True
        else:
            print(f"❌ MongoDB clear failed: {result.stderr}")
            return False
    except Exception as e:
        print(f"❌ MongoDB clear error: {e}")
        return False

def run_motion_detection_on_video():
    """Run C++ motion detection on the test video"""
    print_step(2, "Running Motion Detection on Video")
    
    # Load configuration to get video path
    try:
        config = load_clustering_config()
        video_path = os.path.abspath(config.test_video_path)
        print(f"📹 Processing video from config: {video_path}")
    except Exception as e:
        print(f"⚠️  Warning: Could not load config ({e}), using default")
        video_path = os.path.abspath("test/vid/vid_3.mov")
        print(f"📹 Processing video: {video_path}")
    
    try:
        # Run motion detection with MongoDB integration
        result = subprocess.run([
            'python', 'src/main.py', 
            '--video', video_path,
            '--mongo'
        ], capture_output=True, text=True, timeout=120)
        
        if result.returncode == 0:
            print("✅ Motion detection completed successfully")
            print("📊 Output summary:")
            # Print last few lines of output
            output_lines = result.stdout.strip().split('\n')
            for line in output_lines[-5:]:
                if line.strip():
                    print(f"   {line}")
            return True
        else:
            print(f"❌ Motion detection failed: {result.stderr}")
            print(f"📊 stdout: {result.stdout}")
            return False
            
    except subprocess.TimeoutExpired:
        print("❌ Motion detection timed out (120s)")
        return False
    except Exception as e:
        print(f"❌ Motion detection error: {e}")
        return False

def verify_mongodb_frames():
    """Verify frames were saved to MongoDB"""
    print_step(3, "Verifying MongoDB Frames")
    
    try:
        result = subprocess.run([
            'mongosh', 'birds_of_play', '--eval',
            '''
            const total = db.captured_frames.countDocuments({});
            const withRegions = db.captured_frames.countDocuments({"metadata.consolidated_regions_count": {$gt: 0}});
            console.log("Total frames:", total);
            console.log("Frames with regions:", withRegions);
            
            if (withRegions > 0) {
                console.log("\\n=== Sample Frame Metadata ===");
                const sample = db.captured_frames.findOne({"metadata.consolidated_regions_count": {$gt: 0}}, {metadata: 1});
                console.log(JSON.stringify(sample.metadata, null, 2));
            }
            '''
        ], capture_output=True, text=True, timeout=10)
        
        if result.returncode == 0:
            print("✅ MongoDB verification completed")
            print("📊 Results:")
            for line in result.stdout.strip().split('\n'):
                if line.strip():
                    print(f"   {line}")
            
            # Check if we have frames with regions
            if "Frames with regions: 0" in result.stdout:
                print("⚠️  Warning: No frames with consolidated regions found")
                return False
            return True
        else:
            print(f"❌ MongoDB verification failed: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"❌ MongoDB verification error: {e}")
        return False

def run_yolo11_analysis():
    """Run YOLO11 analysis on MongoDB frames"""
    print_step(4, "Running YOLO11 Analysis")
    
    try:
        result = subprocess.run([
            'python', 'src/main.py',
            '--mongo', '--yolo'
        ], capture_output=True, text=True, timeout=300)
        
        if result.returncode == 0:
            print("✅ YOLO11 analysis completed successfully")
            print("📊 Analysis summary:")
            # Print relevant output lines
            output_lines = result.stdout.strip().split('\n')
            for line in output_lines:
                if any(keyword in line.lower() for keyword in ['yolo', 'processing frame', 'found', 'regions', 'detections', 'completed']):
                    print(f"   {line}")
            return True
        else:
            print(f"❌ YOLO11 analysis failed: {result.stderr}")
            print(f"📊 stdout: {result.stdout}")
            return False
            
    except subprocess.TimeoutExpired:
        print("❌ YOLO11 analysis timed out (300s)")
        return False
    except Exception as e:
        print(f"❌ YOLO11 analysis error: {e}")
        return False

def verify_final_results():
    """Verify the complete pipeline results"""
    print_step(5, "Verifying Final Results")
    
    try:
        result = subprocess.run([
            'mongosh', 'birds_of_play', '--eval',
            '''
            console.log("=== FINAL PIPELINE RESULTS ===");
            const total = db.captured_frames.countDocuments({});
            const withRegions = db.captured_frames.countDocuments({"metadata.consolidated_regions_count": {$gt: 0}});
            const withYolo = db.captured_frames.countDocuments({"metadata.yolo_analysis": {$exists: true}});
            
            console.log("📊 Frame Statistics:");
            console.log("   Total frames:", total);
            console.log("   Frames with motion regions:", withRegions);
            console.log("   Frames with YOLO analysis:", withYolo);
            
            if (withYolo > 0) {
                console.log("\\n🧠 YOLO Analysis Sample:");
                const yoloSample = db.captured_frames.findOne({"metadata.yolo_analysis": {$exists: true}}, {"metadata.yolo_analysis": 1});
                if (yoloSample && yoloSample.metadata.yolo_analysis) {
                    console.log("   Processed:", yoloSample.metadata.yolo_analysis.processed);
                    console.log("   Regions processed:", yoloSample.metadata.yolo_analysis.regions_processed);
                    console.log("   Timestamp:", yoloSample.metadata.yolo_analysis.timestamp);
                }
            }
            
            console.log("\\n🎯 Motion Region Details:");
            const regionSample = db.captured_frames.findOne({"metadata.consolidated_regions": {$exists: true, $ne: []}}, {"metadata.consolidated_regions": 1});
            if (regionSample && regionSample.metadata.consolidated_regions) {
                regionSample.metadata.consolidated_regions.forEach((region, i) => {
                    console.log(`   Region ${i+1}: (${region.x}, ${region.y}, ${region.width}x${region.height}) - ${region.object_count} objects`);
                });
            }
            '''
        ], capture_output=True, text=True, timeout=10)
        
        if result.returncode == 0:
            print("✅ Final verification completed")
            print(result.stdout)
            return True
        else:
            print(f"❌ Final verification failed: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"❌ Final verification error: {e}")
        return False

def main():
    """Main test execution"""
    print_banner("Birds of Play - Full Pipeline Test")
    
    start_time = time.time()
    
    # Track results
    results = {
        'prerequisites': False,
        'mongodb_clear': False,
        'motion_detection': False,
        'mongodb_verify': False,
        'yolo_analysis': False,
        'final_verification': False
    }
    
    # Run all test steps
    try:
        results['prerequisites'] = check_prerequisites()
        if not results['prerequisites']:
            return 1
        
        results['mongodb_clear'] = clear_mongodb()
        if not results['mongodb_clear']:
            return 1
        
        results['motion_detection'] = run_motion_detection_on_video()
        if not results['motion_detection']:
            return 1
        
        results['mongodb_verify'] = verify_mongodb_frames()
        if not results['mongodb_verify']:
            return 1
        
        results['yolo_analysis'] = run_yolo11_analysis()
        if not results['yolo_analysis']:
            return 1
        
        results['final_verification'] = verify_final_results()
        
    except KeyboardInterrupt:
        print("\n⏹️  Test interrupted by user")
        return 1
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        return 1
    
    # Print final summary
    elapsed_time = time.time() - start_time
    print_banner("Test Summary")
    
    all_passed = all(results.values())
    
    print("📊 Results:")
    for step, passed in results.items():
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"   {step.replace('_', ' ').title()}: {status}")
    
    print(f"\n⏱️  Total execution time: {elapsed_time:.1f} seconds")
    
    if all_passed:
        print("\n🎉 ALL TESTS PASSED! Full pipeline working correctly.")
        print("\n🌐 You can now view results at: http://localhost:3000")
        print("   (Make sure web server is running: make web-start)")
        return 0
    else:
        print("\n❌ Some tests failed. Check the output above for details.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
