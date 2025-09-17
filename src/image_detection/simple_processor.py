#!/usr/bin/env python3
"""
Simple Image Detection Processor
===============================

Processes motion detection frames in FIFO order and stores individual region results
for the web viewer to display.
"""

import os
import sys
import uuid
import cv2
import numpy as np
from pathlib import Path
from datetime import datetime, timezone
from typing import List, Dict, Any, Optional

# Add project root to path
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root))

try:
    from mongodb.database_manager import DatabaseManager
    from mongodb.frame_database import FrameDatabase
    from .detectors.object_detector import ObjectDetector
except ImportError as e:
    print(f"❌ Import error: {e}")
    sys.exit(1)


class SimpleDetectionProcessor:
    """Simple processor for FIFO frame processing with individual region storage"""
    
    def __init__(self, model_type: str = "yolo", detection_model: str = "yolo11n", confidence_threshold: float = 0.25):
        """
        Initialize the simple detection processor
        
        Args:
            model_type: Type of detection model (e.g., "yolo")
            detection_model: Detection model to use (e.g., "yolo11n")
            confidence_threshold: Minimum confidence for detections
        """
        self.model_type = model_type
        self.detection_model = detection_model
        self.confidence_threshold = confidence_threshold
        
        # Setup MongoDB connection
        self.db_manager = DatabaseManager()
        if not self.db_manager.connect():
            raise RuntimeError("Failed to connect to MongoDB")
        
        self.frame_db = FrameDatabase(self.db_manager)
        self.detection_collection = self.db_manager.db.detection_results
        
        # Initialize detector
        try:
            self.detector = ObjectDetector(
                model_type=model_type,
                model_path=f"{detection_model}.pt",
                confidence_threshold=confidence_threshold
            )
            print(f"✅ {detection_model.upper()} detector initialized")
        except Exception as e:
            raise RuntimeError(f"Failed to initialize detector: {e}")
        
        # Create output directories
        self.output_dir = Path(project_root) / "data" / "detections"
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        print(f"📁 Detection output directory: {self.output_dir}")
    
    def process_frames_fifo(self, limit: int = 1000) -> Dict[str, Any]:
        """
        Process frames in FIFO order (oldest first)
        
        Args:
            limit: Maximum number of frames to process
            
        Returns:
            Processing summary
        """
        print("🔍 Starting FIFO frame processing...")
        print("=" * 50)
        
        start_time = datetime.now(timezone.utc)
        
        # Get frames in FIFO order (oldest first)
        frames = list(self.frame_db.collection.find({})
                     .sort("timestamp", 1)  # Ascending order (oldest first)
                     .limit(limit))
        
        if not frames:
            print("📭 No frames found in MongoDB")
            return {"processed": 0, "regions": 0, "detections": 0}
        
        print(f"📊 Found {len(frames)} frames to process (FIFO order)")
        
        processed_frames = 0
        processed_regions = 0
        total_detections = 0
        
        for frame in frames:
            try:
                frame_uuid = frame.get('_id')
                metadata = frame.get('metadata', {})
                consolidated_regions = metadata.get('consolidated_regions', [])
                
                if not consolidated_regions:
                    continue
                
                print(f"🎯 Processing frame {frame_uuid} with {len(consolidated_regions)} regions")
                
                # Load frame image
                frame_image = self._load_frame_image(frame)
                if frame_image is None:
                    print(f"⚠️  Could not load image for frame {frame_uuid}")
                    continue
                
                # Process each consolidated region individually
                for region_idx, region in enumerate(consolidated_regions):
                    try:
                        result = self._process_single_region(
                            frame_uuid, frame_image, region, region_idx, frame
                        )
                        if result:
                            processed_regions += 1
                            total_detections += len(result.get('detections', []))
                            
                    except Exception as e:
                        print(f"❌ Error processing region {region_idx} in frame {frame_uuid}: {e}")
                        continue
                
                processed_frames += 1
                
            except Exception as e:
                print(f"❌ Error processing frame {frame.get('_id', 'unknown')}: {e}")
                continue
        
        end_time = datetime.now(timezone.utc)
        duration = (end_time - start_time).total_seconds()
        
        summary = {
            "processed_frames": processed_frames,
            "processed_regions": processed_regions,
            "total_detections": total_detections,
            "duration_seconds": duration,
            "start_time": start_time.isoformat(),
            "end_time": end_time.isoformat()
        }
        
        print(f"✅ Processing completed!")
        print(f"📊 Processed {processed_frames} frames, {processed_regions} regions")
        print(f"🎯 Found {total_detections} total detections")
        print(f"⏱️  Duration: {duration:.1f} seconds")
        
        return summary
    
    def _load_frame_image(self, frame: Dict[str, Any]) -> Optional[np.ndarray]:
        """Load frame image from file path"""
        try:
            # Try processed image first, then original
            image_path = frame.get('processed_image_path') or frame.get('original_image_path')
            if not image_path:
                return None
            
            # Resolve absolute path
            abs_path = Path(project_root) / image_path
            if not abs_path.exists():
                return None
            
            # Load image
            image = cv2.imread(str(abs_path))
            if image is None:
                return None
            
            return image
            
        except Exception as e:
            print(f"❌ Error loading frame image: {e}")
            return None
    
    def _process_single_region(self, frame_uuid: str, frame_image: np.ndarray, 
                              region: Dict[str, Any], region_idx: int, 
                              frame_data: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """
        Process a single consolidated region
        
        Args:
            frame_uuid: UUID of the frame
            frame_image: Full frame image
            region: Region metadata
            region_idx: Index of the region in the frame
            frame_data: Full frame data from MongoDB
            
        Returns:
            Detection result dictionary or None if failed
        """
        try:
            # Extract region coordinates
            x = region.get('x', 0)
            y = region.get('y', 0)
            w = region.get('width', 0)
            h = region.get('height', 0)
            
            if w <= 0 or h <= 0:
                return None
            
            # Crop region from frame
            region_image = frame_image[y:y+h, x:x+w]
            if region_image.size == 0:
                return None
            
            # Run object detection on the region
            detections = self.detector.detect_objects_in_region(
                region_image, 
                {'x': x, 'y': y, 'width': w, 'height': h}
            )
            
            # Create detection result
            detection_id = str(uuid.uuid4())
            timestamp = datetime.now(timezone.utc)
            
            result = {
                '_id': detection_id,
                'frame_uuid': frame_uuid,
                'region_id': region_idx,
                'timestamp': timestamp,
                'detection_model': self.detection_model,
                'model_type': self.model_type,
                'confidence_threshold': self.confidence_threshold,
                'region_bounds': {
                    'x': x, 'y': y, 'width': w, 'height': h
                },
                'detections': [
                    {
                        'class_id': d.class_id,
                        'class_name': d.class_name,
                        'confidence': d.confidence,
                        'bbox_x1': d.bbox_x1,
                        'bbox_y1': d.bbox_y1,
                        'bbox_x2': d.bbox_x2,
                        'bbox_y2': d.bbox_y2,
                        'region_x': d.region_x,
                        'region_y': d.region_y,
                        'region_w': d.region_w,
                        'region_h': d.region_h
                    }
                    for d in detections
                ],
                'detection_count': len(detections),
                'processing_success': True
            }
            
            # Save region images
            self._save_region_images(detection_id, region_image, frame_image, region, detections)
            
            # Store in MongoDB
            self.detection_collection.insert_one(result)
            
            print(f"  ✅ Region {region_idx}: {len(detections)} detections")
            return result
            
        except Exception as e:
            print(f"  ❌ Error processing region {region_idx}: {e}")
            return None
    
    def _save_region_images(self, detection_id: str, region_image: np.ndarray, 
                           frame_image: np.ndarray, region: Dict[str, Any], 
                           detections: List[Any]):
        """Save original and detection overlay images for the region"""
        try:
            # Create detection overlay
            overlay_image = region_image.copy()
            
            # Draw detection boxes on the region
            for detection in detections:
                x1, y1 = detection.region_x, detection.region_y
                x2, y2 = x1 + detection.region_w, y1 + detection.region_h
                
                # Draw bounding box
                cv2.rectangle(overlay_image, (x1, y1), (x2, y2), (0, 255, 0), 2)
                
                # Draw label
                label = f"{detection.class_name} {detection.confidence:.2f}"
                cv2.putText(overlay_image, label, (x1, y1-10), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
            
            # Save images
            original_path = self.output_dir / f"{detection_id}_original.jpg"
            detection_path = self.output_dir / f"{detection_id}_detection.jpg"
            
            cv2.imwrite(str(original_path), region_image)
            cv2.imwrite(str(detection_path), overlay_image)
            
            # Update result with image paths
            self.detection_collection.update_one(
                {'_id': detection_id},
                {
                    '$set': {
                        'original_region_path': str(original_path.relative_to(project_root)),
                        'detection_image_path': str(detection_path.relative_to(project_root))
                    }
                }
            )
            
        except Exception as e:
            print(f"❌ Error saving region images: {e}")
    
    def create_frame_detection_overlay(self, frame_uuid: str) -> Optional[str]:
        """
        Create a full-frame detection overlay image showing all detections for a frame
        
        Args:
            frame_uuid: UUID of the frame
            
        Returns:
            Path to the overlay image or None if failed
        """
        try:
            # Get all detections for this frame
            detections = list(self.detection_collection.find({'frame_uuid': frame_uuid}))
            if not detections:
                return None
            
            # Get frame data
            frame = self.frame_db.collection.find_one({'_id': frame_uuid})
            if not frame:
                return None
            
            # Load original frame image
            frame_image = self._load_frame_image(frame)
            if frame_image is None:
                return None
            
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
            overlay_path = self.output_dir / f"{frame_uuid}_frame_detection_overlay.jpg"
            cv2.imwrite(str(overlay_path), overlay_image)
            
            return str(overlay_path.relative_to(project_root))
            
        except Exception as e:
            print(f"❌ Error creating frame detection overlay: {e}")
            return None
    
    def cleanup(self):
        """Clean up resources"""
        if self.db_manager:
            self.db_manager.disconnect()


def main():
    """Main function for CLI usage"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Simple Image Detection Processor")
    parser.add_argument('--model', type=str, default='yolo11n', help='Detection model')
    parser.add_argument('--confidence', type=float, default=0.25, help='Confidence threshold')
    parser.add_argument('--limit', type=int, default=1000, help='Max frames to process')
    
    args = parser.parse_args()
    
    try:
        processor = SimpleDetectionProcessor(
            detection_model=args.model,
            confidence_threshold=args.confidence
        )
        
        summary = processor.process_frames_fifo(limit=args.limit)
        
        print(f"\n🎉 Processing Summary:")
        print(f"   Frames: {summary['processed_frames']}")
        print(f"   Regions: {summary['processed_regions']}")
        print(f"   Detections: {summary['total_detections']}")
        print(f"   Duration: {summary['duration_seconds']:.1f}s")
        
        return 0
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1
    
    finally:
        if 'processor' in locals():
            processor.cleanup()


if __name__ == "__main__":
    sys.exit(main())
