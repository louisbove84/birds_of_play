#!/usr/bin/env python3
"""
Image Detection Module Test Script
=================================

This script tests the image detection functionality by processing
consolidated motion regions from MongoDB.
"""

import sys
import os
import argparse
from pathlib import Path

# Add the project root to the Python path
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root))

def setup_mongodb_connection():
    """Setup MongoDB connection for testing"""
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

def test_image_detection():
    """Test the image detection processor"""
    print("🧪 Testing Image Detection Module")
    print("=" * 50)
    
    # Setup MongoDB connection
    db_manager, frame_db = setup_mongodb_connection()
    if not frame_db:
        print("❌ Cannot test without MongoDB connection")
        return False
    
    try:
        # Import the detection processor
        from image_detection.models.detection_processor import ImageDetectionProcessor
        
        # Initialize processor
        processor = ImageDetectionProcessor(
            frame_db=frame_db,
            detection_model="yolo11n",
            confidence_threshold=0.25
        )
        
        print("✅ Image detection processor initialized")
        
        # Process frames (limit to 5 for testing)
        print("\n🚀 Processing frames...")
        summary = processor.process_all_frames(limit=5)
        
        # Print results
        print("\n📊 Detection Results Summary:")
        print(f"   Frames processed: {summary.total_frames_processed}")
        print(f"   Regions processed: {summary.total_regions_processed}")
        print(f"   Detections found: {summary.total_detections_found}")
        print(f"   Success rate: {summary.success_rate:.1%}")
        print(f"   Processing time: {summary.processing_start_time} → {summary.processing_end_time}")
        
        return True
        
    except ImportError as e:
        print(f"❌ Failed to import detection modules: {e}")
        print("Make sure ultralytics is installed: pip install ultralytics")
        return False
    except Exception as e:
        print(f"❌ Error during testing: {e}")
        return False
    finally:
        if db_manager:
            db_manager.disconnect()
            print("📊 MongoDB connection closed")

def main():
    """Main test function"""
    parser = argparse.ArgumentParser(description="Test Image Detection Module")
    parser.add_argument('--limit', type=int, default=5, help='Limit number of frames to process')
    
    args = parser.parse_args()
    
    print("🐦 Birds of Play - Image Detection Test")
    print("=" * 60)
    
    success = test_image_detection()
    
    if success:
        print("\n✅ Image detection test completed successfully!")
        return 0
    else:
        print("\n❌ Image detection test failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
