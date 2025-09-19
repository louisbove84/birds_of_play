#!/usr/bin/env python3
"""
Extract Individual Object Script
===============================

Extracts individual detected objects from region images based on bounding box coordinates.
"""

import sys
import os
import cv2
import numpy as np
from pathlib import Path
from pymongo import MongoClient

def extract_object(object_id: str) -> bool:
    """
    Extract an individual detected object from a region image
    
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
        
        print(f"🔍 Extracting object {object_id}")
        print(f"   Region: {region_id}")
        print(f"   Detection index: {detection_index}")
        
        # Connect to MongoDB
        client = MongoClient('mongodb://localhost:27017')
        db = client['birds_of_play']
        
        # Get detection data
        region_detection = db.region_detections.find_one({'region_id': region_id})
        if not region_detection:
            print(f"❌ No detection data found for region {region_id}")
            return False
        
        detections = region_detection.get('detections', [])
        if detection_index >= len(detections):
            print(f"❌ Detection index {detection_index} not found (only {len(detections)} detections)")
            return False
        
        detection = detections[detection_index]
        bbox = detection['bbox']  # [x1, y1, x2, y2]
        
        print(f"   Object: {detection['display_name']} ({detection['confidence']:.1%})")
        print(f"   Bounding box: {bbox}")
        
        # Load the region image
        project_root = Path(__file__).parent.parent.parent
        region_image_path = project_root / "data" / "regions" / f"{region_id}.jpg"
        
        if not region_image_path.exists():
            print(f"❌ Region image not found: {region_image_path}")
            return False
        
        region_image = cv2.imread(str(region_image_path))
        if region_image is None:
            print(f"❌ Could not load region image: {region_image_path}")
            return False
        
        print(f"✅ Loaded region image: {region_image.shape}")
        
        # Extract bounding box coordinates
        x1, y1, x2, y2 = bbox
        
        # Ensure coordinates are within image bounds
        img_height, img_width = region_image.shape[:2]
        x1 = max(0, min(x1, img_width - 1))
        y1 = max(0, min(y1, img_height - 1))
        x2 = max(x1 + 1, min(x2, img_width))
        y2 = max(y1 + 1, min(y2, img_height))
        
        # Add some padding around the object (10% of object size)
        obj_width = x2 - x1
        obj_height = y2 - y1
        padding_x = max(5, int(obj_width * 0.1))
        padding_y = max(5, int(obj_height * 0.1))
        
        # Apply padding
        x1_padded = max(0, x1 - padding_x)
        y1_padded = max(0, y1 - padding_y)
        x2_padded = min(img_width, x2 + padding_x)
        y2_padded = min(img_height, y2 + padding_y)
        
        # Crop object from region
        object_image = region_image[y1_padded:y2_padded, x1_padded:x2_padded]
        
        if object_image.size == 0:
            print(f"❌ Empty object after cropping")
            return False
        
        print(f"✅ Extracted object: {object_image.shape} (with padding)")
        
        # Create output directory
        output_dir = project_root / "data" / "objects"
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Save object image
        object_path = output_dir / f"{object_id}.jpg"
        cv2.imwrite(str(object_path), object_image)
        
        print(f"✅ Saved object image: {object_path}")
        return True
        
    except Exception as e:
        print(f"❌ Error extracting object: {e}")
        return False
    
    finally:
        if 'client' in locals():
            client.close()

def main():
    """Main function"""
    if len(sys.argv) != 2:
        print("Usage: python extract_object.py <object_id>")
        sys.exit(1)
    
    object_id = sys.argv[1]
    success = extract_object(object_id)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()