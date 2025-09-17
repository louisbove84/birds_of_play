#!/usr/bin/env python3
"""
Extract Region Script
====================

Extracts individual consolidated regions from frames and saves them as separate images.
"""

import sys
import os
import cv2
import numpy as np
from pathlib import Path
from pymongo import MongoClient

def extract_region(frame_id: str, region_index: int) -> bool:
    """
    Extract a consolidated region from a frame
    
    Args:
        frame_id: UUID of the frame
        region_index: Index of the region to extract
        
    Returns:
        True if successful, False otherwise
    """
    try:
        # Connect to MongoDB
        client = MongoClient('mongodb://localhost:27017')
        db = client['birds_of_play']
        
        # Get frame data
        frame = db.captured_frames.find_one({'_id': frame_id})
        if not frame:
            print(f"❌ Frame {frame_id} not found")
            return False
        
        # Get consolidated regions
        regions = frame.get('metadata', {}).get('consolidated_regions', [])
        if region_index >= len(regions):
            print(f"❌ Region index {region_index} not found (only {len(regions)} regions)")
            return False
        
        region = regions[region_index]
        
        # Load original frame image
        image_path = frame.get('original_image_path')
        if not image_path:
            print(f"❌ No original image path for frame {frame_id}")
            return False
        
        # Resolve path relative to project root
        project_root = Path(__file__).parent.parent
        abs_path = project_root / image_path
        
        if not abs_path.exists():
            print(f"❌ Original image not found: {abs_path}")
            return False
        
        # Load image
        frame_image = cv2.imread(str(abs_path))
        if frame_image is None:
            print(f"❌ Could not load image: {abs_path}")
            return False
        
        # Extract region coordinates
        x = region.get('x', 0)
        y = region.get('y', 0)
        w = region.get('width', 0)
        h = region.get('height', 0)
        
        if w <= 0 or h <= 0:
            print(f"❌ Invalid region dimensions: {w}x{h}")
            return False
        
        # Ensure coordinates are within image bounds
        img_height, img_width = frame_image.shape[:2]
        x = max(0, min(x, img_width - 1))
        y = max(0, min(y, img_height - 1))
        w = min(w, img_width - x)
        h = min(h, img_height - y)
        
        # Crop region from frame
        region_image = frame_image[y:y+h, x:x+w]
        
        if region_image.size == 0:
            print(f"❌ Empty region after cropping")
            return False
        
        # Create output directory
        output_dir = Path(__file__).parent.parent / "data" / "regions"
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Save region image
        region_path = output_dir / f"{frame_id}_{region_index}.jpg"
        cv2.imwrite(str(region_path), region_image)
        
        print(f"✅ Extracted region: {region_path}")
        return True
        
    except Exception as e:
        print(f"❌ Error extracting region: {e}")
        return False
    
    finally:
        if 'client' in locals():
            client.close()

def main():
    """Main function"""
    if len(sys.argv) != 3:
        print("Usage: python extract_region.py <frame_id> <region_index>")
        sys.exit(1)
    
    frame_id = sys.argv[1]
    try:
        region_index = int(sys.argv[2])
    except ValueError:
        print("Error: region_index must be an integer")
        sys.exit(1)
    
    success = extract_region(frame_id, region_index)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
