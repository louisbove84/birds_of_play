"""
Image Detection Processor
========================

Main orchestrator class for processing consolidated motion regions with various
detection algorithms and storing results in MongoDB.
"""

import numpy as np
from typing import List, Dict, Any, Optional, Tuple
from datetime import datetime, timezone
from ..detectors.object_detector import ObjectDetector
from .detection_result import FrameDetectionResult, DetectionSummary
from .detection_database import DetectionDatabase


class ImageDetectionProcessor:
    """Main processor for image detection on motion regions"""
    
    def __init__(self, frame_db, model_type: str = "yolo", detection_model: str = "yolo11n", confidence_threshold: float = 0.25):
        """
        Initialize the detection processor
        
        Args:
            frame_db: MongoDB frame database connection
            model_type: Type of detection model (e.g., "yolo", "rcnn", "ssd")
            detection_model: Detection model to use (e.g., "yolo11n")
            confidence_threshold: Minimum confidence for detections
        """
        self.frame_db = frame_db
        self.model_type = model_type
        self.detection_model = detection_model
        self.confidence_threshold = confidence_threshold
        
        # Initialize detector with model type validation
        try:
            self.detector = ObjectDetector(
                model_type=model_type,
                model_path=f"{detection_model}.pt",
                confidence_threshold=confidence_threshold
            )
        except Exception as e:
            raise ValueError(f"Failed to initialize {model_type} detector: {e}")
        
        # Initialize detection database
        self.detection_db = DetectionDatabase(frame_db.db_manager)
    
    def process_all_frames(self, limit: int = 1000) -> DetectionSummary:
        """
        Process all frames in MongoDB with consolidated regions
        
        Args:
            limit: Maximum number of frames to process
            
        Returns:
            Detection summary with statistics
        """
        print("🔍 Birds of Play - Image Detection Processing")
        print("=" * 50)
        
        start_time = datetime.now(timezone.utc)
        
        # Get frame metadata from MongoDB
        frame_metadata_list = self.frame_db.list_frames(limit=limit)
        if not frame_metadata_list:
            print("📭 No frames found in MongoDB")
            return self._create_empty_summary(start_time)
        
        print(f"📊 Found {len(frame_metadata_list)} frames in MongoDB")
        
        # Filter frames that have consolidated regions
        frames_with_regions = []
        total_regions = 0
        
        for frame_metadata in frame_metadata_list:
            metadata = frame_metadata.get('metadata', {})
            regions = metadata.get('consolidated_regions', [])
            if regions:
                frame_uuid = frame_metadata.get('uuid') or frame_metadata.get('_id')
                if frame_uuid:
                    frame_image = self.frame_db.get_frame(frame_uuid)
                    if frame_image is not None:
                        frames_with_regions.append({
                            'uuid': frame_uuid,
                            'metadata': metadata,
                            'image': frame_image,
                            'timestamp': frame_metadata.get('timestamp')
                        })
                        total_regions += len(regions)
        
        if not frames_with_regions:
            print("📭 No frames with consolidated motion regions found")
            return self._create_empty_summary(start_time)
        
        print(f"🎯 Found {len(frames_with_regions)} frames with {total_regions} motion regions")
        print(f"🚀 Starting {self.detection_model} analysis...")
        
        # Process each frame
        frame_results = []
        total_detections = 0
        error_count = 0
        
        for i, frame_data in enumerate(frames_with_regions):
            try:
                result = self.process_single_frame(frame_data)
                frame_results.append(result)
                total_detections += result.total_detections
                
                if not result.processing_success:
                    error_count += 1
                
                print(f"✅ Frame {i+1}/{len(frames_with_regions)}: {result.total_detections} detections")
                
            except Exception as e:
                print(f"❌ Error processing frame {i+1}: {e}")
                error_count += 1
        
        # Store results in MongoDB
        stored_count = self.detection_db.store_detection_results(frame_results)
        
        # Clean up full-resolution images after processing to save disk space
        # Keep thumbnails for quick preview in web browsers
        if stored_count > 0:
            self._cleanup_processed_frames(frame_results)
        
        end_time = datetime.now(timezone.utc)
        
        # Create summary
        summary = DetectionSummary(
            total_frames_processed=len(frames_with_regions),
            total_regions_processed=total_regions,
            total_detections_found=total_detections,
            processing_start_time=start_time.isoformat(),
            processing_end_time=end_time.isoformat(),
            detection_model=self.detection_model,
            success_rate=(len(frames_with_regions) - error_count) / len(frames_with_regions) if frames_with_regions else 0.0,
            error_count=error_count
        )
        
        print(f"✅ Detection processing completed!")
        print(f"📊 Processed {summary.total_frames_processed} frames")
        print(f"🎯 Found {summary.total_detections_found} total detections")
        print(f"📈 Success rate: {summary.success_rate:.1%}")
        
        return summary
    
    def _cleanup_processed_frames(self, frame_results: List[FrameDetectionResult]):
        """
        Clean up full-resolution images after they've been processed.
        This saves disk space while keeping thumbnails for quick preview.
        
        Args:
            frame_results: List of processed frame results
        """
        try:
            from mongodb.frame_database_v2 import FrameDatabaseV2
            frame_db_v2 = FrameDatabaseV2(self.frame_db.db_manager)
            
            cleanup_count = 0
            for frame_result in frame_results:
                if frame_result.processing_success:
                    cleanup_success = frame_db_v2.cleanup_after_processing(
                        frame_result.frame_uuid, keep_thumbnails=True
                    )
                    if cleanup_success:
                        cleanup_count += 1
            
            if cleanup_count > 0:
                print(f"🧹 Cleaned up full-resolution images for {cleanup_count} frames (thumbnails kept)")
            
        except Exception as e:
            print(f"⚠️  Warning: Failed to cleanup processed frames: {e}")
    
    def process_single_frame(self, frame_data: Dict[str, Any]) -> FrameDetectionResult:
        """
        Process a single frame for object detection
        
        Args:
            frame_data: Frame data dictionary with image and metadata
            
        Returns:
            Frame detection result
        """
        frame_uuid = frame_data.get('uuid', 'unknown')
        metadata = frame_data.get('metadata', {})
        regions = metadata.get('consolidated_regions', [])
        frame_image = frame_data.get('image')
        
        if frame_image is None:
            return FrameDetectionResult(
                frame_uuid=frame_uuid,
                original_frame_metadata=metadata,
                regions_processed=0,
                total_detections=0,
                region_results=[],
                processing_success=False,
                processing_error="No image data"
            )
        
        if not isinstance(frame_image, np.ndarray) or frame_image.size == 0:
            return FrameDetectionResult(
                frame_uuid=frame_uuid,
                original_frame_metadata=metadata,
                regions_processed=0,
                total_detections=0,
                region_results=[],
                processing_success=False,
                processing_error="Invalid image data"
            )
        
        # Process regions with detector
        region_results = self.detector.detect_objects_in_regions(frame_image, regions)
        
        # Calculate totals
        total_detections = sum(result.detection_count for result in region_results)
        regions_processed = len([r for r in region_results if r.processed])
        
        return FrameDetectionResult(
            frame_uuid=frame_uuid,
            original_frame_metadata=metadata,
            regions_processed=regions_processed,
            total_detections=total_detections,
            region_results=region_results,
            detection_model=self.detection_model
        )
    
    
    def _create_empty_summary(self, start_time: datetime) -> DetectionSummary:
        """Create an empty summary when no frames are found"""
        end_time = datetime.now(timezone.utc)
        return DetectionSummary(
            total_frames_processed=0,
            total_regions_processed=0,
            total_detections_found=0,
            processing_start_time=start_time.isoformat(),
            processing_end_time=end_time.isoformat(),
            detection_model=self.detection_model,
            success_rate=0.0,
            error_count=0
        )
