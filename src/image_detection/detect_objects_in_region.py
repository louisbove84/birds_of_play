#!/usr/bin/env python3
"""
Detect Objects in Region Script
==============================

Runs YOLO11 object detection on consolidated region cutouts and creates overlay images.
"""

import sys
import os
import cv2
import numpy as np
import json
from pathlib import Path
from pymongo import MongoClient

def detect_objects_in_region(region_id: str) -> bool:
    """
    Run YOLO11 detection on a region cutout and create overlay image
    
    Args:
        region_id: ID of the region (format: frameId_regionIndex)
        
    Returns:
        True if successful, False otherwise
    """
    try:
        # Import YOLO
        try:
            from ultralytics import YOLO
        except ImportError:
            print(f"❌ ultralytics not available. Install with: pip install ultralytics")
            return False
        
        # Connect to MongoDB
        client = MongoClient('mongodb://localhost:27017')
        db = client['birds_of_play']
        
        # Parse region ID
        frame_id, region_index = region_id.split('_')
        region_index = int(region_index)
        
        print(f"🔍 Processing region {region_id} (Frame: {frame_id}, Region: {region_index})")
        
        # Get frame data to access consolidated regions metadata
        frame = db.captured_frames.find_one({'_id': frame_id})
        if not frame:
            print(f"❌ Frame {frame_id} not found")
            return False
        
        consolidated_regions = frame.get('metadata', {}).get('consolidated_regions', [])
        if region_index >= len(consolidated_regions):
            print(f"❌ Region index {region_index} not found")
            return False
        
        region_metadata = consolidated_regions[region_index]
        
        # Load the region image
        project_root = Path(__file__).parent.parent
        region_image_path = project_root / "data" / "regions" / f"{region_id}.jpg"
        
        if not region_image_path.exists():
            print(f"❌ Region image not found: {region_image_path}")
            return False
        
        region_image = cv2.imread(str(region_image_path))
        if region_image is None:
            print(f"❌ Could not load region image: {region_image_path}")
            return False
        
        print(f"✅ Loaded region image: {region_image.shape}")
        
        # Load YOLO model
        # Load model from organized location
        script_dir = Path(__file__).parent
        model_path = script_dir / "models" / "yolo11n.pt"
        model = YOLO(str(model_path))  # Using YOLO11 nano for speed
        print(f"✅ YOLO11 model loaded")
        
        # Run detection
        results = model(region_image, verbose=False)
        
        detections = []
        overlay_image = region_image.copy()
        
        # Process detection results
        for result in results:
            boxes = result.boxes
            if boxes is not None:
                for box in boxes:
                    # Get box coordinates
                    x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                    confidence = float(box.conf[0].cpu().numpy())
                    class_id = int(box.cls[0].cpu().numpy())
                    class_name = model.names[class_id]
                    
                    # Filter by confidence threshold (25%)
                    if confidence < 0.25:
                        continue
                    
                    # Store detection info
                    detection_info = {
                        'class_id': class_id,
                        'class_name': class_name,
                        'confidence': confidence,
                        'bbox': [int(x1), int(y1), int(x2), int(y2)]
                    }
                    detections.append(detection_info)
                    
                    # Draw bounding box on overlay image
                    cv2.rectangle(overlay_image, (int(x1), int(y1)), (int(x2), int(y2)), (0, 255, 0), 2)
                    
                    # Draw label with background
                    label = f"{class_name} {confidence:.2f}"
                    label_size = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2)[0]
                    
                    # Draw background rectangle for label
                    cv2.rectangle(overlay_image, 
                                 (int(x1), int(y1) - label_size[1] - 10),
                                 (int(x1) + label_size[0], int(y1)),
                                 (0, 255, 0), -1)
                    
                    # Draw label text
                    cv2.putText(overlay_image, label, (int(x1), int(y1) - 5),
                               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 0), 2)
        
        print(f"🎯 Found {len(detections)} objects")
        
        # Save overlay image
        overlay_path = project_root / "data" / "regions" / f"{region_id}_detection.jpg"
        cv2.imwrite(str(overlay_path), overlay_image)
        print(f"✅ Saved detection overlay: {overlay_path}")
        
        # Save detection results as JSON
        results_path = project_root / "data" / "regions" / f"{region_id}_results.json"
        detection_results = {
            'region_id': region_id,
            'frame_id': frame_id,
            'region_index': region_index,
            'region_metadata': region_metadata,
            'detections': detections,
            'detection_count': len(detections),
            'model': 'yolo11n',
            'confidence_threshold': 0.25
        }
        
        with open(results_path, 'w') as f:
            json.dump(detection_results, f, indent=2)
        print(f"✅ Saved detection results: {results_path}")
        
        # Store in MongoDB region_detections collection
        region_detections = db.region_detections
        
        # Update or insert detection results
        region_detections.replace_one(
            {'region_id': region_id},
            detection_results,
            upsert=True
        )
        print(f"✅ Stored results in MongoDB")
        
        return True
        
    except Exception as e:
        print(f"❌ Error detecting objects in region: {e}")
        return False
    
    finally:
        if 'client' in locals():
            client.close()

def main():
    """Main function"""
    if len(sys.argv) != 2:
        print("Usage: python detect_objects_in_region.py <region_id>")
        sys.exit(1)
    
    region_id = sys.argv[1]
    success = detect_objects_in_region(region_id)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
