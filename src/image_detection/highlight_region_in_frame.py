#!/usr/bin/env python3
"""
Highlight Region in Frame Script
===============================

Creates a motion frame image with a specific region highlighted in green
to show which region contains the detected object.
"""

import sys
import os
import cv2
import numpy as np
from pathlib import Path
from pymongo import MongoClient

def highlight_region_in_frame(object_id: str) -> bool:
    """
    Create a motion frame image with the specific region highlighted in green
    
    Args:
        object_id: ID of the object (format: frameId_regionIndex_obj_detectionIndex)
        
    Returns:
        True if successful, False otherwise
    """
    try:
        # Parse object ID
        parts = object_id.split('_obj_')
        if len(parts) != 2:
            print(f"❌ Invalid object ID format: {object_id}")
            return False
        
        region_id = parts[0]
        detection_index = int(parts[1])
        
        frame_id, region_index = region_id.split('_')
        region_index = int(region_index)
        
        print(f"🔍 Highlighting region {region_index} in frame {frame_id}")
        
        # Connect to MongoDB
        client = MongoClient('mongodb://localhost:27017')
        db = client['birds_of_play']
        
        # Get frame data
        frame = db.captured_frames.find_one({'_id': frame_id})
        if not frame:
            print(f"❌ Frame {frame_id} not found")
            return False
        
        # Get consolidated regions
        consolidated_regions = frame.get('metadata', {}).get('consolidated_regions', [])
        if region_index >= len(consolidated_regions):
            print(f"❌ Region index {region_index} not found")
            return False
        
        target_region = consolidated_regions[region_index]
        
        # Load processed frame image (with motion detection boxes)
        image_path = frame.get('processed_image_path')
        if not image_path:
            print(f"❌ No processed image path for frame {frame_id}")
            return False
        
        # Resolve path
        project_root = Path(__file__).parent.parent.parent
        abs_path = project_root / image_path
        
        if not abs_path.exists():
            print(f"❌ Processed image not found: {abs_path}")
            return False
        
        # Load image
        frame_image = cv2.imread(str(abs_path))
        if frame_image is None:
            print(f"❌ Could not load image: {abs_path}")
            return False
        
        print(f"✅ Loaded frame image: {frame_image.shape}")
        
        # Create highlighted image
        highlighted_image = frame_image.copy()
        
        # Draw bright green border around the target region
        x = target_region.get('x', 0)
        y = target_region.get('y', 0)
        w = target_region.get('width', 0)
        h = target_region.get('height', 0)
        
        # Draw thick green border
        cv2.rectangle(highlighted_image, (x, y), (x + w, y + h), (0, 255, 0), 8)
        
        # Draw green corner markers for extra visibility
        corner_size = 30
        thickness = 8
        
        # Top-left corner
        cv2.line(highlighted_image, (x, y), (x + corner_size, y), (0, 255, 0), thickness)
        cv2.line(highlighted_image, (x, y), (x, y + corner_size), (0, 255, 0), thickness)
        
        # Top-right corner
        cv2.line(highlighted_image, (x + w, y), (x + w - corner_size, y), (0, 255, 0), thickness)
        cv2.line(highlighted_image, (x + w, y), (x + w, y + corner_size), (0, 255, 0), thickness)
        
        # Bottom-left corner
        cv2.line(highlighted_image, (x, y + h), (x + corner_size, y + h), (0, 255, 0), thickness)
        cv2.line(highlighted_image, (x, y + h), (x, y + h - corner_size), (0, 255, 0), thickness)
        
        # Bottom-right corner
        cv2.line(highlighted_image, (x + w, y + h), (x + w - corner_size, y + h), (0, 255, 0), thickness)
        cv2.line(highlighted_image, (x + w, y + h), (x + w, y + h - corner_size), (0, 255, 0), thickness)
        
        # Add region label
        label = f"Region {region_index + 1}"
        label_size = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 1.2, 3)[0]
        
        # Draw label background
        label_x = x + 10
        label_y = y + 40
        cv2.rectangle(highlighted_image, 
                     (label_x - 5, label_y - label_size[1] - 10),
                     (label_x + label_size[0] + 5, label_y + 5),
                     (0, 255, 0), -1)
        
        # Draw label text
        cv2.putText(highlighted_image, label, (label_x, label_y),
                   cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 0, 0), 3)
        
        print(f"✅ Highlighted region {region_index} at ({x}, {y}) size {w}x{h}")
        
        # Create output directory
        output_dir = project_root / "data" / "highlighted_frames"
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Save highlighted frame
        highlighted_path = output_dir / f"{object_id}_highlighted_frame.jpg"
        cv2.imwrite(str(highlighted_path), highlighted_image)
        
        print(f"✅ Saved highlighted frame: {highlighted_path}")
        return True
        
    except Exception as e:
        print(f"❌ Error highlighting region: {e}")
        return False
    
    finally:
        if 'client' in locals():
            client.close()

def main():
    """Main function"""
    if len(sys.argv) != 2:
        print("Usage: python highlight_region_in_frame.py <object_id>")
        sys.exit(1)
    
    object_id = sys.argv[1]
    success = highlight_region_in_frame(object_id)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
