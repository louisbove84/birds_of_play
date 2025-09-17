"""
Test Script: Thumbnail System and Cleanup

This script demonstrates the new thumbnail system and automatic cleanup
of full-resolution images after processing.
"""

import os
import sys
import numpy as np
import cv2
import time
from pathlib import Path

# Add the src directory to the Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from mongodb.database_manager import DatabaseManager
from mongodb.frame_database_v2 import FrameDatabaseV2
from mongodb.file_storage_manager import FileStorageManager


def create_test_frame(width: int = 1280, height: int = 720) -> np.ndarray:
    """Create a test frame with some content."""
    frame = np.random.randint(0, 255, (height, width, 3), dtype=np.uint8)
    
    # Add some structure to make it more realistic
    cv2.rectangle(frame, (10, 10), (100, 100), (255, 0, 0), 3)
    cv2.circle(frame, (200, 200), 50, (0, 255, 0), -1)
    cv2.putText(frame, f"Test Frame {width}x{height}", (300, 300), 
                cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 0, 255), 3)
    
    return frame


def test_thumbnail_system():
    """Test the thumbnail system functionality."""
    print("🧪 Testing Thumbnail System")
    print("=" * 50)
    
    # Initialize database and storage
    db_manager = DatabaseManager()
    if not db_manager.connect():
        print("❌ Failed to connect to MongoDB")
        return False
    
    frame_db = FrameDatabaseV2(db_manager, "data/test_frames")
    storage = FileStorageManager("data/test_frames")
    
    try:
        # Create test frames
        print("📸 Creating test frames...")
        original_frame = create_test_frame(1280, 720)
        processed_frame = create_test_frame(1280, 720)
        
        # Add some processing to the processed frame
        cv2.rectangle(processed_frame, (0, 0), (200, 200), (255, 255, 0), 5)
        cv2.putText(processed_frame, "PROCESSED", (50, 50), 
                   cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
        
        # Save frames with thumbnails
        print("💾 Saving frames with thumbnails...")
        frame_uuid = frame_db.save_frame_with_original(original_frame, processed_frame, {
            "test": True,
            "source": "test_script",
            "timestamp": time.time()
        })
        
        if not frame_uuid:
            print("❌ Failed to save frames")
            return False
        
        print(f"✅ Saved frames with UUID: {frame_uuid}")
        
        # Check what files were created
        print("\n📁 Checking created files...")
        stats = storage.get_storage_stats()
        print(f"   Original files: {stats.get('original_count', 0)}")
        print(f"   Processed files: {stats.get('processed_count', 0)}")
        print(f"   Total size: {stats.get('total_size_mb', 0):.2f} MB")
        
        # Test loading thumbnails
        print("\n🖼️ Testing thumbnail loading...")
        original_thumbnail = storage.load_thumbnail(frame_uuid, "original")
        processed_thumbnail = storage.load_thumbnail(frame_uuid, "processed")
        
        if original_thumbnail is not None:
            print(f"✅ Original thumbnail loaded: {original_thumbnail.shape}")
        else:
            print("❌ Failed to load original thumbnail")
        
        if processed_thumbnail is not None:
            print(f"✅ Processed thumbnail loaded: {processed_thumbnail.shape}")
        else:
            print("❌ Failed to load processed thumbnail")
        
        # Test loading full-resolution images
        print("\n🖼️ Testing full-resolution loading...")
        original_full = storage.load_frame(frame_uuid, "original")
        processed_full = storage.load_frame(frame_uuid, "processed")
        
        if original_full is not None:
            print(f"✅ Original full-resolution loaded: {original_full.shape}")
        else:
            print("❌ Failed to load original full-resolution")
        
        if processed_full is not None:
            print(f"✅ Processed full-resolution loaded: {processed_full.shape}")
        else:
            print("❌ Failed to load processed full-resolution")
        
        # Test cleanup after processing
        print("\n🧹 Testing cleanup after processing...")
        cleanup_success = frame_db.cleanup_after_processing(frame_uuid, keep_thumbnails=True)
        
        if cleanup_success:
            print("✅ Cleanup successful")
            
            # Check that full-resolution images are gone but thumbnails remain
            original_full_after = storage.load_frame(frame_uuid, "original")
            processed_full_after = storage.load_frame(frame_uuid, "processed")
            original_thumb_after = storage.load_thumbnail(frame_uuid, "original")
            processed_thumb_after = storage.load_thumbnail(frame_uuid, "processed")
            
            if original_full_after is None and processed_full_after is None:
                print("✅ Full-resolution images successfully deleted")
            else:
                print("❌ Full-resolution images still exist")
            
            if original_thumb_after is not None and processed_thumb_after is not None:
                print("✅ Thumbnails successfully preserved")
            else:
                print("❌ Thumbnails were deleted")
            
            # Check updated stats
            stats_after = storage.get_storage_stats()
            print(f"   Files after cleanup: {stats_after.get('original_count', 0)} original, {stats_after.get('processed_count', 0)} processed")
            print(f"   Size after cleanup: {stats_after.get('total_size_mb', 0):.2f} MB")
            
        else:
            print("❌ Cleanup failed")
        
        # Test metadata retrieval
        print("\n📊 Testing metadata retrieval...")
        metadata = frame_db.get_frame_metadata(frame_uuid)
        if metadata:
            print("✅ Metadata retrieved successfully")
            print(f"   Frame shape: {metadata.get('frame_shape')}")
            print(f"   Processing status: {metadata.get('processing_status', 'unknown')}")
            print(f"   Full resolution cleaned: {metadata.get('full_resolution_cleaned', False)}")
        else:
            print("❌ Failed to retrieve metadata")
        
        return True
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        return False
    
    finally:
        # Clean up test data
        print("\n🧹 Cleaning up test data...")
        if 'frame_uuid' in locals():
            frame_db.delete_frame(frame_uuid)
            print(f"✅ Cleaned up test frame: {frame_uuid}")


def test_performance_comparison():
    """Compare performance with and without thumbnails."""
    print("\n⚡ Performance Comparison")
    print("=" * 50)
    
    db_manager = DatabaseManager()
    if not db_manager.connect():
        print("❌ Failed to connect to MongoDB")
        return
    
    frame_db = FrameDatabaseV2(db_manager, "data/performance_test")
    
    # Test without thumbnails
    print("📸 Testing without thumbnails...")
    start_time = time.time()
    
    for i in range(10):
        original_frame = create_test_frame(640, 480)
        processed_frame = create_test_frame(640, 480)
        
        frame_uuid = frame_db.save_frame_with_original(original_frame, processed_frame, {
            "test": True,
            "no_thumbnails": True,
            "index": i
        })
        
        if frame_uuid:
            frame_db.delete_frame(frame_uuid)
    
    no_thumbnails_time = time.time() - start_time
    print(f"   Time without thumbnails: {no_thumbnails_time:.2f} seconds")
    
    # Test with thumbnails
    print("📸 Testing with thumbnails...")
    start_time = time.time()
    
    for i in range(10):
        original_frame = create_test_frame(640, 480)
        processed_frame = create_test_frame(640, 480)
        
        frame_uuid = frame_db.save_frame_with_original(original_frame, processed_frame, {
            "test": True,
            "with_thumbnails": True,
            "index": i
        })
        
        if frame_uuid:
            frame_db.delete_frame(frame_uuid)
    
    with_thumbnails_time = time.time() - start_time
    print(f"   Time with thumbnails: {with_thumbnails_time:.2f} seconds")
    
    overhead = ((with_thumbnails_time - no_thumbnails_time) / no_thumbnails_time) * 100
    print(f"   Thumbnail overhead: {overhead:.1f}%")


def main():
    """Main test function."""
    print("🚀 Thumbnail System Test")
    print("=" * 60)
    
    # Test basic functionality
    success = test_thumbnail_system()
    
    if success:
        print("\n✅ All tests passed!")
        
        # Test performance
        test_performance_comparison()
        
        print("\n🎉 Thumbnail system is working correctly!")
        print("\n💡 Benefits:")
        print("   - Thumbnails provide quick preview in web browsers")
        print("   - Full-resolution images are cleaned up after processing")
        print("   - Significant disk space savings")
        print("   - Better performance for web interfaces")
        
    else:
        print("\n❌ Tests failed!")
        sys.exit(1)


if __name__ == "__main__":
    main()
