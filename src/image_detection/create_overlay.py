#!/usr/bin/env python3
"""
Create Detection Overlay Script
==============================

Creates full-frame detection overlay images showing YOLO11 detection boxes
on the original motion detection frames.
"""

import sys
import os
import cv2
import numpy as np
from pathlib import Path
from pymongo import MongoClient

# Add project root to path
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root))

def create_detection_overlay(frame_uuid: str) -> bool:
    """
    Create a detection overlay image for a frame
    
    Args:
        frame_uuid: UUID of the frame
        
    Returns:
        True if successful, False otherwise
    """
    try:
        # Connect to MongoDB
        client = MongoClient('mongodb://localhost:27017')
        db = client['birds_of_play']
        
        # Get frame data
        frame = db.captured_frames.find_one({'_id': frame_uuid})
        if not frame:
            print(f"❌ Frame {frame_uuid} not found")
            return False
        
        # Get detection results for this frame
        detections = list(db.detection_results.find({'frame_uuid': frame_uuid}))
        if not detections:
            print(f"⚠️  No detections found for frame {frame_uuid}")
            return False
        
        # Load original frame image
        image_path = frame.get('original_image_path')
        if not image_path:
            print(f"❌ No original image path for frame {frame_uuid}")
            return False
        
        abs_path = Path(project_root) / image_path
        if not abs_path.exists():
            print(f"❌ Original image not found: {abs_path}")
            return False
        
        # Load image
        frame_image = cv2.imread(str(abs_path))
        if frame_image is None:
            print(f"❌ Could not load image: {abs_path}")
            return False
        
        # Create overlay image
        overlay_image = frame_image.copy()
        
        # Draw all detection boxes
        for detection in detections:
            for det in detection.get('detections', []):
                x1, y1 = det['bbox_x1'], det['bbox_y1']
                x2, y2 = det['bbox_x2'], det['bbox_y2']
                
                # Draw bounding box
                cv2.rectangle(overlay_image, (x1, y1), (x2, y2), (0, 255, 0), 2)
                
                # Draw label
                label = f"{det['class_name']} {det['confidence']:.2f}"
                cv2.putText(overlay_image, label, (x1, y1-10), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        
        # Save overlay image
        output_dir = Path(project_root) / "data" / "detections"
        output_dir.mkdir(parents=True, exist_ok=True)
        
        overlay_path = output_dir / f"{frame_uuid}_frame_detection_overlay.jpg"
        cv2.imwrite(str(overlay_path), overlay_image)
        
        print(f"✅ Created detection overlay: {overlay_path}")
        return True
        
    except Exception as e:
        print(f"❌ Error creating detection overlay: {e}")
        return False
    
    finally:
        if 'client' in locals():
            client.close()

def main():
    """Main function"""
    if len(sys.argv) != 2:
        print("Usage: python create_overlay.py <frame_uuid>")
        sys.exit(1)
    
    frame_uuid = sys.argv[1]
    success = create_detection_overlay(frame_uuid)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
