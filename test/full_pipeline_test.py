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
import argparse
from pathlib import Path

# Add the src directory to Python path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src', 'unsupervised_ml'))
from config_loader import load_clustering_config

# Add the src directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src'))

# Global variable for custom video path
custom_video_path = None

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

def clear_data_images():
    """Clear all images from the data directory and user corrections"""
    print_step(1, "Clearing Data Directory Images & User Corrections")
    
    try:
        import shutil
        
        data_dir = Path("data")
        removed_count = 0
        
        # Clear data directory if it exists
        if data_dir.exists():
            # Remove all subdirectories and files in data/
            for item in data_dir.iterdir():
                if item.is_dir():
                    print(f"   🗑️  Removing directory: {item.name}")
                    shutil.rmtree(item)
                    removed_count += 1
                elif item.is_file():
                    print(f"   🗑️  Removing file: {item.name}")
                    item.unlink()
                    removed_count += 1
        
        # Clear user corrections from fine-tuning
        corrections_file = Path("src/unsupervised_ml/user_corrections.json")
        if corrections_file.exists():
            print(f"   🗑️  Clearing user corrections: {corrections_file.name}")
            corrections_file.unlink()
            removed_count += 1
        
        # Clear trained model to force retraining
        model_file = Path("src/unsupervised_ml/trained_bird_classifier.pth")
        if model_file.exists():
            print(f"   🗑️  Removing previous model: {model_file.name}")
            model_file.unlink()
            removed_count += 1
        
        print(f"✅ Data and corrections cleared successfully - removed {removed_count} items")
        return True
        
    except Exception as e:
        print(f"❌ Data directory clear error: {e}")
        return False

def clear_mongodb():
    """Clear MongoDB collection"""
    print_step(2, "Clearing MongoDB")
    
    try:
        result = subprocess.run([
            'mongosh', 'birds_of_play', '--eval', 
            '''
            db.captured_frames.deleteMany({}); 
            db.high_confidence_detections.deleteMany({});
            db.region_detections.deleteMany({});
            print("Cleared captured_frames:", db.captured_frames.countDocuments({}));
            print("Cleared detections:", db.high_confidence_detections.countDocuments({}));
            print("Cleared regions:", db.region_detections.countDocuments({}));
            '''
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
    print_step(3, "Running Motion Detection on Video")
    
    # Use custom video path if provided, otherwise load from config
    if custom_video_path:
        video_path = os.path.abspath(custom_video_path)
        print(f"📹 Processing custom video: {video_path}")
        print(f"⏱️  Timeout: 300s (default)")
        timeout = 300
    else:
        # Load configuration to get video path
        try:
            config = load_clustering_config()
            video_path = os.path.abspath(config.test_video_path)
            timeout = config.test_timeout_seconds
            print(f"📹 Processing video from config: {video_path}")
            print(f"⏱️  Timeout: {timeout}s")
        except Exception as e:
            print(f"⚠️  Warning: Could not load config ({e}), using defaults")
            video_path = os.path.abspath("test/vid/vid_3.mov")
            timeout = 300
            print(f"📹 Processing video: {video_path}")
            print(f"⏱️  Timeout: {timeout}s")
    
    try:
        # Run motion detection with MongoDB integration
        result = subprocess.run([
            'python', 'src/main.py', 
            '--video', video_path,
            '--mongo'
        ], capture_output=True, text=True, timeout=timeout)
        
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
    print_step(4, "Verifying MongoDB Frames")
    
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

def extract_all_regions():
    """Extract all consolidated regions from frames as individual cutout images"""
    print_step(5, "Extracting Region Cutouts")
    
    try:
        print("✂️ Extracting all consolidated regions from frames...")
        print("📊 This will create individual region cutout images needed for YOLO11 detection")
        
        # Connect to MongoDB to get all frames with regions
        from pymongo import MongoClient
        client = MongoClient('mongodb://localhost:27017')
        db = client['birds_of_play']
        
        # Get all frames with consolidated regions
        frames = list(db.captured_frames.find(
            {"metadata.consolidated_regions_count": {"$gt": 0}}
        ).sort("timestamp", 1))
        
        print(f"📊 Found {len(frames)} frames with consolidated regions")
        
        total_regions = 0
        extracted_regions = 0
        
        for frame in frames:
            frame_id = frame['_id']
            consolidated_regions = frame.get('metadata', {}).get('consolidated_regions', [])
            
            for region_index in range(len(consolidated_regions)):
                total_regions += 1
                
                # Run extract_region.py for each region
                result = subprocess.run([
                    'python', 'src/image_detection/extract_region.py',
                    frame_id, str(region_index)
                ], capture_output=True, text=True, timeout=30)
                
                if result.returncode == 0:
                    extracted_regions += 1
                    if extracted_regions % 50 == 0:  # Progress update every 50 regions
                        print(f"   📊 Extracted {extracted_regions}/{total_regions} regions...")
                else:
                    print(f"   ⚠️ Failed to extract region {frame_id}_{region_index}: {result.stderr}")
        
        print(f"✅ Region extraction completed!")
        print(f"📊 Successfully extracted {extracted_regions}/{total_regions} region cutouts")
        
        return extracted_regions > 0
        
    except Exception as e:
        print(f"❌ Error in region extraction: {e}")
        return False
    
    finally:
        if 'client' in locals():
            client.close()

def run_batch_region_detection():
    """Run comprehensive YOLO11 detection on all extracted region cutouts"""
    print_step(6, "Running Batch Region Detection")
    
    try:
        print("🎯 Running comprehensive YOLO11 detection on extracted region cutouts...")
        print("📊 This will:")
        print("   • Load pre-extracted region cutout images")
        print("   • Run YOLO11 detection on each region")
        print("   • Save detection results to MongoDB")
        print("   • Create detection overlay images")
        print("   • Filter by confidence threshold and object classes")
        
        # Run the batch detection script directly (more efficient than via main.py)
        result = subprocess.run([
            'python', 'src/image_detection/batch_detect_regions.py'
        ], capture_output=True, text=True, timeout=300)
        
        if result.returncode == 0:
            print("✅ Batch region detection completed successfully")
            print("📊 Detection summary:")
            # Print relevant output lines
            output_lines = result.stdout.strip().split('\n')
            for line in output_lines:
                if any(keyword in line.lower() for keyword in ['batch detection completed', 'processed', 'found', 'high-confidence', 'detections']):
                    print(f"   {line}")
            return True
        else:
            print(f"❌ Batch region detection failed: {result.stderr}")
            print(f"📊 stdout: {result.stdout}")
            return False
            
    except subprocess.TimeoutExpired:
        print("❌ Batch region detection timed out (300s)")
        return False
    except Exception as e:
        print(f"❌ Batch region detection error: {e}")
        return False

def start_web_servers():
    """Start all web servers for viewing results"""
    print_step(7, "Starting Web Servers")
    
    try:
        import subprocess
        import time
        
        # Get the project root directory
        project_root = Path(__file__).parent.parent
        
        print("🌐 Starting web servers...")
        print("📹 Motion Detection: http://localhost:3000")
        print("🎯 Object Detection: http://localhost:3001") 
        print("🔬 Bird Clustering: http://localhost:3002")
        print("🧠 Fine-Tuning: http://localhost:3003")
        
        # Start motion detection server (port 3000)
        web_dir = project_root / "web"
        if web_dir.exists():
            subprocess.Popen([
                'node', 'simple_viewer.js'
            ], cwd=str(web_dir), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            print("   ✅ Motion detection server starting...")
            time.sleep(2)
            
            # Start object detection server (port 3001)
            subprocess.Popen([
                'node', 'region_viewer.js'
            ], cwd=str(web_dir), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            print("   ✅ Object detection server starting...")
            time.sleep(2)
        else:
            print("   ⚠️  Web directory not found, skipping Node.js servers")
        
        # Start ML servers (ports 3002 and 3003)
        ml_dir = project_root / "src" / "unsupervised_ml"
        if ml_dir.exists():
            venv_python = project_root / "venv_ml" / "bin" / "python"
            if venv_python.exists():
                # Start clustering server (port 3002) - using full clustering system
                subprocess.Popen([
                    str(venv_python), 'cluster_server.py'
                ], cwd=str(ml_dir), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                print("   ✅ Bird clustering server starting...")
                
                # Start fine-tuning server (port 3003) - using full fine-tuning system
                subprocess.Popen([
                    str(venv_python), 'fine_tuning_viewer.py'
                ], cwd=str(project_root / "web"), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                print("   ✅ Fine-tuning server starting...")
                time.sleep(3)
            else:
                print("   ⚠️  ML virtual environment not found, skipping ML servers")
        else:
            print("   ⚠️  ML directory not found, skipping ML servers")
        
        print("\n🌐 Web servers started! Give them a few seconds to initialize...")
        print("📱 Open your browser and navigate to:")
        print("   • Motion Detection: http://localhost:3000")
        print("   • Object Detection: http://localhost:3001")
        print("   • Bird Clustering: http://localhost:3002") 
        print("   • Fine-Tuning: http://localhost:3003")
        
        return True
        
    except Exception as e:
        print(f"❌ Error starting web servers: {e}")
        print("💡 You can start them manually with: make web-start")
        return False

def extract_all_objects():
    """Extract individual object images from detected regions for clustering"""
    print_step(8, "Extracting Object Images")
    
    try:
        print("✂️ Extracting individual object images from detected regions...")
        print("📊 This will create cropped images of each detected bird for clustering analysis")
        
        # Connect to MongoDB to get all high-confidence detections
        from pymongo import MongoClient
        client = MongoClient('mongodb://localhost:27017')
        db = client['birds_of_play']
        
        # Get all high-confidence detections
        detections = list(db.high_confidence_detections.find({}))
        
        print(f"📊 Found {len(detections)} high-confidence bird detections")
        
        if len(detections) == 0:
            print("⚠️ No high-confidence detections found for object extraction")
            return True  # Don't fail the pipeline for this
        
        extracted_objects = 0
        failed_extractions = 0
        
        for detection in detections:
            detection_id = detection.get('detection_id')
            if not detection_id:
                continue
                
            # Convert detection_id to object_id format (det_0 -> obj_0)
            object_id = detection_id.replace('_det_', '_obj_')
            
            # Run extract_object.py for each detection
            result = subprocess.run([
                'python', 'src/image_detection/extract_object.py', object_id
            ], capture_output=True, text=True, timeout=30)
            
            if result.returncode == 0:
                extracted_objects += 1
                if extracted_objects % 5 == 0:  # Progress update every 5 objects
                    print(f"   📊 Extracted {extracted_objects}/{len(detections)} objects...")
            else:
                failed_extractions += 1
                if failed_extractions <= 3:  # Show first 3 failures
                    print(f"   ⚠️ Failed to extract {object_id}: {result.stderr.strip()}")
        
        print(f"✅ Object extraction completed!")
        print(f"📊 Successfully extracted {extracted_objects}/{len(detections)} object images")
        print(f"📊 Failed extractions: {failed_extractions}")
        
        if extracted_objects == 0:
            print("❌ No objects were successfully extracted")
            return False
        
        return True
        
    except Exception as e:
        print(f"❌ Error in object extraction: {e}")
        return False
    
    finally:
        if 'client' in locals():
            client.close()

def initialize_clustering_system():
    """Initialize the bird clustering system with detected objects"""
    print_step(9, "Initializing Bird Clustering System")
    
    try:
        print("🔬 Initializing clustering system with detected bird objects...")
        print("📊 This will:")
        print("   • Load detected bird objects from MongoDB")
        print("   • Extract features using ResNet CNN")
        print("   • Run hierarchical clustering algorithms")
        print("   • Analyze bird species groupings")
        
        # Give the clustering server time to start up
        import time
        time.sleep(5)
        
        # Initialize the clustering system via HTTP API
        import requests
        
        try:
            response = requests.get('http://localhost:3002/initialize', timeout=60)
            if response.status_code == 200:
                result = response.json()
                if result.get('status') == 'success':
                    print("✅ Clustering system initialized successfully")
                    print("📊 Bird species analysis ready")
                    return True
                else:
                    print(f"❌ Clustering initialization failed: {result.get('message', 'Unknown error')}")
                    return False
            else:
                print(f"❌ Clustering server error: HTTP {response.status_code}")
                return False
                
        except requests.exceptions.RequestException as e:
            print(f"❌ Could not connect to clustering server: {e}")
            print("💡 The clustering server may still be starting up")
            return False
            
    except Exception as e:
        print(f"❌ Error initializing clustering system: {e}")
        return False

def train_supervised_classifier():
    """Train supervised classifier using clustering results as pseudo-labels"""
    print_step(10, "Training Supervised Classifier")
    
    try:
        print("🧠 Training supervised bird classifier using clustering pseudo-labels...")
        print("📊 This will:")
        print("   • Load clustering results from the initialized system")
        print("   • Use cluster labels as pseudo-labels for supervised training")
        print("   • Train SimCLR backbone with classification head")
        print("   • Save trained model for fine-tuning interface")
        
        # Give some time for clustering system to be fully ready
        import time
        time.sleep(2)
        
        # Import required modules
        import sys
        sys.path.insert(0, 'src/unsupervised_ml')
        
        from object_data_manager import ObjectDataManager
        from feature_extractor import FeatureExtractor, FeaturePipeline
        from bird_clusterer import BirdClusterer, ClusteringExperiment
        from supervised_classifier import SupervisedBirdTrainer
        from config_loader import load_clustering_config
        
        # Load configuration
        config = load_clustering_config()
        
        # Load data and run clustering (reuse from clustering initialization)
        with ObjectDataManager() as data_manager:
            feature_extractor = FeatureExtractor(model_name=config.model_name)
            pipeline = FeaturePipeline(data_manager, feature_extractor)
            
            features, metadata = pipeline.extract_all_features(min_confidence=config.min_confidence)
            
            if len(features) == 0:
                print("⚠️ No bird objects found for supervised training")
                return True  # Don't fail pipeline for this
            
            print(f"📊 Found {len(features)} bird objects for supervised training")
            
            # Run clustering experiment to get best clustering
            experiment = ClusteringExperiment(features, metadata, config=config)
            results = experiment.run_all_methods()
            
            best_method, best_result = experiment.get_best_method()
            
            if not best_result:
                print("❌ No successful clustering method found")
                return False
            
            clusterer = best_result['clusterer']
            n_clusters = best_result['metrics']['n_clusters']
            
            print(f"✅ Best clustering method: {best_method}")
            print(f"📊 Found {n_clusters} bird species clusters")
            
            # Train supervised classifier
            trainer = SupervisedBirdTrainer(config=config)
            
            print("🔄 Preparing training data from clustering results...")
            success = trainer.prepare_data_from_clustering(clusterer, metadata)
            
            if not success:
                print("❌ Failed to prepare training data")
                return False
            
            print("🔄 Phase 1: Training with frozen backbone...")
            trainer.train_phase1_frozen_backbone(epochs=5, lr=0.001)
            
            print("🔄 Phase 2: Fine-tuning entire model...")
            trainer.train_phase2_full_finetuning(epochs=3, lr=0.0001)
            
            print("🔄 Evaluating model...")
            eval_results = trainer.evaluate()
            
            print(f"✅ Model training completed!")
            print(f"📊 Test Accuracy: {eval_results['accuracy']:.3f}")
            print(f"📊 Mean Confidence: {eval_results['mean_confidence']:.3f}")
            
            # Save model
            model_path = "src/unsupervised_ml/trained_bird_classifier.pth"
            trainer.save_model(model_path)
            
            print(f"✅ Model saved to {model_path}")
            print("🧠 Fine-tuning interface now ready with trained model")
            
            return True
            
    except Exception as e:
        print(f"❌ Error in supervised training: {e}")
        import traceback
        traceback.print_exc()
        return False

def verify_final_results():
    """Verify the complete pipeline results"""
    print_step(11, "Verifying Final Results")
    
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
    parser = argparse.ArgumentParser(description='Birds of Play - Full Pipeline Test')
    parser.add_argument('--video', type=str, help='Path to custom video file to process')
    parser.add_argument('--skip-video', action='store_true', help='Skip video processing and use existing data')
    args = parser.parse_args()
    
    print_banner("Birds of Play - Full Pipeline Test")
    
    # Set custom video path if provided
    global custom_video_path
    custom_video_path = args.video
    
    start_time = time.time()
    
    # Track results
    results = {
        'prerequisites': False,
        'clear_data_images': False,
        'mongodb_clear': False,
        'motion_detection': False,
        'mongodb_verify': False,
        'extract_regions': False,
        'batch_detection': False,
        'web_servers': False,
        'extract_objects': False,
        'clustering_initialization': False,
        'supervised_training': False,
        'final_verification': False
    }
    
    # Run all test steps
    try:
        results['prerequisites'] = check_prerequisites()
        if not results['prerequisites']:
            return 1
        
        if args.skip_video:
            print("🔄 Skipping video processing - using existing data")
            results['clear_data_images'] = True
            results['mongodb_clear'] = True
            results['motion_detection'] = True
            results['mongodb_verify'] = True
        else:
            results['clear_data_images'] = clear_data_images()
            if not results['clear_data_images']:
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
        
        results['extract_regions'] = extract_all_regions()
        if not results['extract_regions']:
            return 1
        
        results['batch_detection'] = run_batch_region_detection()
        if not results['batch_detection']:
            return 1
        
        results['web_servers'] = start_web_servers()
        # Don't fail the test if web servers don't start - it's not critical
        
        results['extract_objects'] = extract_all_objects()
        if not results['extract_objects']:
            return 1
        
        results['clustering_initialization'] = initialize_clustering_system()
        
        results['supervised_training'] = train_supervised_classifier()
        
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
        print("\n🌐 Web servers are starting up! View results at:")
        print("   📹 Motion Detection: http://localhost:3000")
        print("   🎯 Object Detection: http://localhost:3001")
        print("   🔬 Bird Clustering: http://localhost:3002")
        print("   🧠 Fine-Tuning: http://localhost:3003")
        print("\n💡 Give the servers 10-15 seconds to fully initialize before browsing.")
        return 0
    else:
        print("\n❌ Some tests failed. Check the output above for details.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
