#!/usr/bin/env python3
"""
Batch Region Detection Script
============================

Automatically runs YOLO11 detection on all consolidated regions and creates
individual detection cards for high-confidence objects (>90%).
"""

import sys
import os
import cv2
import numpy as np
import json
import yaml
from pathlib import Path
from pymongo import MongoClient

def load_config(config_path: str = None) -> dict:
    """Load detection configuration from YAML file"""
    if config_path is None:
        config_path = Path(__file__).parent / "detection_config.yaml"
    
    try:
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)
        print(f"✅ Loaded config from {config_path}")
        return config
    except Exception as e:
        print(f"⚠️ Could not load config: {e}, using defaults")
        return {
            'detection': {
                'high_confidence_threshold': 0.93
            },
            'display_classes': ['bird', 'cat', 'dog', 'person'],
            'model': {
                'type': 'yolo11n',
                'path': 'yolo11n.pt'
            }
        }

def batch_detect_all_regions(config: dict = None) -> bool:
    """
    Run YOLO11 detection on all region cutouts
    
    Args:
        config: Detection configuration dictionary
        
    Returns:
        True if successful, False otherwise
    """
    try:
        # Load config if not provided
        if config is None:
            config = load_config()
        
        # Extract config values
        confidence_threshold = config['detection']['high_confidence_threshold']
        display_classes = set(config['display_classes'])
        model_path = config['model']['path']
        class_aliases = config.get('class_aliases', {})
        
        print(f"🔍 Starting batch detection:")
        print(f"   Confidence threshold: {confidence_threshold*100}%")
        print(f"   Display classes: {', '.join(sorted(display_classes))}")
        
        # Import YOLO
        try:
            from ultralytics import YOLO
        except ImportError:
            print(f"❌ ultralytics not available. Install with: pip install ultralytics")
            return False
        
        # Connect to MongoDB
        client = MongoClient('mongodb://localhost:27017')
        db = client['birds_of_play']
        
        # Load YOLO model once (resolve path relative to this script)
        script_dir = Path(__file__).parent
        model_full_path = script_dir / model_path
        model = YOLO(str(model_full_path))
        print(f"✅ YOLO11 model loaded: {model_full_path}")
        
        # Get all frames with consolidated regions
        frames = list(db.captured_frames.find({}).sort("timestamp", 1))  # FIFO order
        
        total_regions = 0
        processed_regions = 0
        high_confidence_detections = 0
        
        # Clear existing detection results
        db.region_detections.delete_many({})
        db.high_confidence_detections.delete_many({})
        print("🗑️ Cleared existing detection results")
        
        project_root = Path(__file__).parent.parent.parent
        
        for frame_idx, frame in enumerate(frames):
            frame_id = frame['_id']
            consolidated_regions = frame.get('metadata', {}).get('consolidated_regions', [])
            
            if not consolidated_regions:
                continue
                
            print(f"📊 Frame {frame_idx + 1}/{len(frames)}: {frame_id} ({len(consolidated_regions)} regions)")
            
            for region_idx, region_metadata in enumerate(consolidated_regions):
                region_id = f"{frame_id}_{region_idx}"
                total_regions += 1
                
                # Load region image
                region_image_path = project_root / "data" / "regions" / f"{region_id}.jpg"
                
                if not region_image_path.exists():
                    print(f"  ⚠️ Region image not found: {region_id}")
                    continue
                
                region_image = cv2.imread(str(region_image_path))
                if region_image is None:
                    print(f"  ⚠️ Could not load region image: {region_id}")
                    continue
                
                # Run detection
                results = model(region_image, verbose=False)
                
                detections = []
                high_conf_detections = []
                overlay_image = region_image.copy()
                
                # Process detection results
                for result in results:
                    boxes = result.boxes
                    if boxes is not None:
                        for box in boxes:
                            x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                            confidence = float(box.conf[0].cpu().numpy())
                            class_id = int(box.cls[0].cpu().numpy())
                            class_name = model.names[class_id]
                            
                            # Apply confidence filter and class filter (simplified)
                            if confidence >= confidence_threshold and class_name in display_classes:
                                # Use alias if available
                                display_name = class_aliases.get(class_name, class_name)
                                
                                detection_info = {
                                    'class_id': class_id,
                                    'class_name': class_name,
                                    'display_name': display_name,
                                    'confidence': confidence,
                                    'bbox': [int(x1), int(y1), int(x2), int(y2)]
                                }
                                detections.append(detection_info)
                                
                                # Draw green box on overlay
                                cv2.rectangle(overlay_image, (int(x1), int(y1)), (int(x2), int(y2)), (0, 255, 0), 3)
                                
                                label = f"{display_name} {confidence:.2f}"
                                label_size = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2)[0]
                                
                                cv2.rectangle(overlay_image, 
                                             (int(x1), int(y1) - label_size[1] - 10),
                                             (int(x1) + label_size[0], int(y1)),
                                             (0, 255, 0), -1)
                                
                                cv2.putText(overlay_image, label, (int(x1), int(y1) - 5),
                                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 0), 2)
                                
                                # Store as high-confidence detection
                                high_conf_detection = {
                                    'detection_id': f"{region_id}_det_{len(high_conf_detections)}",
                                    'region_id': region_id,
                                    'frame_id': frame_id,
                                    'region_index': region_idx,
                                    'detection_info': detection_info,
                                    'region_metadata': region_metadata,
                                    'timestamp': frame.get('timestamp')
                                }
                                high_conf_detections.append(high_conf_detection)
                                print(f"    🔥 {display_name}: {confidence:.1%}")
                            else:
                                # Log what was filtered out
                                if class_name not in display_classes:
                                    print(f"    🚫 Skipping {class_name} (not in display classes)")
                                else:
                                    print(f"    ⚡ Low confidence {class_name}: {confidence:.1%} (below {confidence_threshold:.1%})")
                
                # Save overlay image
                overlay_path = project_root / "data" / "regions" / f"{region_id}_detection.jpg"
                cv2.imwrite(str(overlay_path), overlay_image)
                
                # Only store if we have detections that passed the filters
                if detections:
                    # Store region detection results
                    region_results = {
                        'region_id': region_id,
                        'frame_id': frame_id,
                        'region_index': region_idx,
                        'region_metadata': region_metadata,
                        'detections': detections,
                        'detection_count': len(detections),
                        'model': model_path,
                        'confidence_threshold': confidence_threshold,
                        'timestamp': frame.get('timestamp')
                    }
                    
                    db.region_detections.replace_one(
                        {'region_id': region_id},
                        region_results,
                        upsert=True
                    )
                    
                    # Store high-confidence detections as individual cards
                    for high_conf_det in high_conf_detections:
                        db.high_confidence_detections.replace_one(
                            {'detection_id': high_conf_det['detection_id']},
                            high_conf_det,
                            upsert=True
                        )
                        high_confidence_detections += 1
                    
                    print(f"  ✅ Region {region_idx}: {len(detections)} detections saved")
                else:
                    print(f"  ⚪ Region {region_idx}: No qualifying objects")
                
                processed_regions += 1
        
        print(f"\n🎉 Batch detection completed!")
        print(f"📊 Processed {processed_regions}/{total_regions} regions")
        print(f"🎯 Found {high_confidence_detections} high-confidence detections (>{confidence_threshold*100}%)")
        
        return True
        
    except Exception as e:
        print(f"❌ Error in batch detection: {e}")
        return False
    
    finally:
        if 'client' in locals():
            client.close()

def main():
    """Main function"""
    # Load configuration
    config_path = None
    if len(sys.argv) > 1:
        config_path = sys.argv[1]
    
    config = load_config(config_path)
    print(f"🎯 Running batch detection with configuration-based filtering")
    success = batch_detect_all_regions(config)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
