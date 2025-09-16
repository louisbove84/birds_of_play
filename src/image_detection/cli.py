#!/usr/bin/env python3
"""
Image Detection CLI
==================

Command-line interface for the Birds of Play image detection module.
"""

import argparse
import sys
import os
from pathlib import Path

# Add the project root to the Python path
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root))

def setup_mongodb_connection():
    """Setup MongoDB connection"""
    try:
        from mongodb.database_manager import DatabaseManager
        from mongodb.frame_database import FrameDatabase
        
        db_manager = DatabaseManager()
        if not db_manager.connect():
            print("❌ Failed to connect to MongoDB")
            return None, None
        
        frame_db = FrameDatabase(db_manager)
        frame_db.create_indexes()
        
        print("✅ MongoDB connection established")
        return db_manager, frame_db
        
    except ImportError as e:
        print(f"⚠️  MongoDB modules not available: {e}")
        return None, None
    except Exception as e:
        print(f"❌ MongoDB connection failed: {e}")
        return None, None

def run_detection(frame_db, config_path=None, limit=None, model=None, confidence=None):
    """Run image detection on MongoDB frames"""
    try:
        from .models.detection_processor import ImageDetectionProcessor
        
        # Load configuration
        if config_path and os.path.exists(config_path):
            import yaml
            with open(config_path, 'r') as f:
                config = yaml.safe_load(f)
        else:
            # Use defaults
            config = {
                'model_type': 'yolo',
                'detection_model': model or 'yolo11n',
                'confidence_threshold': confidence or 0.25
            }
        
        # Initialize processor with model type validation
        processor = ImageDetectionProcessor(
            frame_db=frame_db,
            model_type=config.get('model_type', 'yolo'),
            detection_model=config['detection_model'],
            confidence_threshold=config['confidence_threshold']
        )
        
        print("✅ Image detection processor initialized")
        
        # Process frames
        summary = processor.process_all_frames(limit=limit or 1000)
        
        print(f"✅ Detection processing completed!")
        print(f"📊 Processed {summary.total_frames_processed} frames")
        print(f"🎯 Found {summary.total_detections_found} total detections")
        print(f"📈 Success rate: {summary.success_rate:.1%}")
        
        return True
        
    except ImportError as e:
        print(f"❌ Failed to import detection modules: {e}")
        print("Make sure ultralytics is installed: pip install ultralytics")
        return False
    except Exception as e:
        print(f"❌ Error during detection processing: {e}")
        return False

def main():
    """Main CLI function"""
    parser = argparse.ArgumentParser(
        description="Birds of Play - Image Detection CLI",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python src/image_detection/cli.py                    # Run with default settings
  python src/image_detection/cli.py --limit 10         # Process only 10 frames
  python src/image_detection/cli.py --model yolo11s    # Use YOLO11s model
  python src/image_detection/cli.py --confidence 0.5   # Set confidence threshold
  python src/image_detection/cli.py --config config.yaml  # Use custom config
        """
    )
    
    parser.add_argument('--config', type=str, help='Path to configuration file')
    parser.add_argument('--limit', type=int, help='Maximum number of frames to process')
    parser.add_argument('--model', type=str, help='Detection model to use (e.g., yolo11n)')
    parser.add_argument('--confidence', type=float, help='Confidence threshold (0.0-1.0)')
    
    args = parser.parse_args()
    
    print("🔍 Birds of Play - Image Detection CLI")
    print("=" * 50)
    
    # Setup MongoDB connection
    db_manager, frame_db = setup_mongodb_connection()
    if not frame_db:
        return 1
    
    try:
        # Run detection
        success = run_detection(
            frame_db=frame_db,
            config_path=args.config,
            limit=args.limit,
            model=args.model,
            confidence=args.confidence
        )
        
        return 0 if success else 1
        
    finally:
        if db_manager:
            db_manager.disconnect()
            print("📊 MongoDB connection closed")

if __name__ == "__main__":
    sys.exit(main())
