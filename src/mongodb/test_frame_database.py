#!/usr/bin/env python3
"""
Test script for Frame Database functionality.
"""

import cv2
import numpy as np
import logging
from .database_manager import DatabaseManager
from .frame_database import FrameDatabase

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def create_test_frame(width: int = 640, height: int = 480) -> np.ndarray:
    """Create a test frame with some content."""
    frame = np.zeros((height, width, 3), dtype=np.uint8)
    
    # Add some shapes
    cv2.rectangle(frame, (100, 100), (200, 200), (255, 0, 0), -1)  # Blue rectangle
    cv2.circle(frame, (400, 300), 50, (0, 255, 0), -1)  # Green circle
    cv2.putText(frame, "Test Frame", (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
    
    return frame

def test_frame_database():
    """Test the frame database functionality."""
    logger.info("Starting Frame Database Test")
    
    # Initialize database manager
    db_manager = DatabaseManager()
    
    try:
        # Connect to database
        if not db_manager.connect():
            logger.error("Failed to connect to MongoDB")
            return False
        
        # Initialize frame database
        frame_db = FrameDatabase(db_manager)
        
        # Create indexes
        frame_db.create_indexes()
        
        # Create test frame
        test_frame = create_test_frame()
        logger.info(f"Created test frame with shape: {test_frame.shape}")
        
        # Test metadata
        metadata = {
            "source": "test",
            "motion_detected": True,
            "motion_regions": 3,
            "confidence": 0.85
        }
        
        # Save frame
        logger.info("Saving frame to database...")
        frame_uuid = frame_db.save_frame(test_frame, metadata)
        
        if not frame_uuid:
            logger.error("Failed to save frame")
            return False
        
        logger.info(f"Saved frame with UUID: {frame_uuid}")
        
        # Get frame count
        count = frame_db.get_frame_count()
        logger.info(f"Total frames in database: {count}")
        
        # Retrieve frame metadata
        logger.info("Retrieving frame metadata...")
        retrieved_metadata = frame_db.get_frame_metadata(frame_uuid)
        
        if retrieved_metadata:
            logger.info(f"Retrieved metadata: {retrieved_metadata}")
        else:
            logger.error("Failed to retrieve metadata")
            return False
        
        # Retrieve frame
        logger.info("Retrieving frame from database...")
        retrieved_frame = frame_db.get_frame(frame_uuid)
        
        if retrieved_frame is not None:
            logger.info(f"Retrieved frame with shape: {retrieved_frame.shape}")
            
            # Compare frames (accounting for JPEG compression)
            # Calculate mean absolute difference
            diff = cv2.absdiff(test_frame, retrieved_frame)
            mean_diff = np.mean(diff)
            
            if mean_diff < 5.0:  # Allow small differences due to JPEG compression
                logger.info(f"✅ Frame comparison successful - mean difference: {mean_diff:.2f}")
            else:
                logger.error(f"❌ Frame comparison failed - mean difference: {mean_diff:.2f}")
                return False
        else:
            logger.error("Failed to retrieve frame")
            return False
        
        # List frames
        logger.info("Listing frames...")
        frames = frame_db.list_frames(limit=10)
        logger.info(f"Found {len(frames)} frames in database")
        
        # Test frame deletion
        logger.info("Testing frame deletion...")
        if frame_db.delete_frame(frame_uuid):
            logger.info("✅ Frame deleted successfully")
            
            # Verify deletion
            count_after = frame_db.get_frame_count()
            logger.info(f"Frames after deletion: {count_after}")
            
            if count_after == count - 1:
                logger.info("✅ Frame count updated correctly")
            else:
                logger.error("❌ Frame count not updated correctly")
                return False
        else:
            logger.error("Failed to delete frame")
            return False
        
        # Get database stats
        stats = db_manager.get_database_stats()
        logger.info(f"Database stats: {stats}")
        
        logger.info("✅ All tests passed!")
        return True
        
    except Exception as e:
        logger.error(f"Test failed with exception: {e}")
        return False
    
    finally:
        # Disconnect from database
        db_manager.disconnect()

if __name__ == "__main__":
    success = test_frame_database()
    if success:
        print("\n🎉 Frame Database Test Completed Successfully!")
    else:
        print("\n❌ Frame Database Test Failed!")
