"""
Detection Result Data Structures
===============================

This module defines the data structures used to represent detection results
from various image detection algorithms.
"""

from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional
from datetime import datetime, timezone


@dataclass
class DetectionMetadata:
    """Metadata for a single object detection"""
    class_id: int
    class_name: str
    confidence: float
    bbox_x1: int
    bbox_y1: int
    bbox_x2: int
    bbox_y2: int
    region_x: int  # Position within the consolidated region
    region_y: int
    region_w: int
    region_h: int
    detection_timestamp: str = field(default_factory=lambda: datetime.now(timezone.utc).isoformat())


@dataclass
class RegionDetectionResult:
    """Results for a single consolidated region"""
    region_id: int
    region_bounds: Dict[str, Any]  # Original region bounds from motion detection
    detections: List[DetectionMetadata]
    processed: bool
    detection_count: int
    processing_error: Optional[str] = None
    processing_timestamp: str = field(default_factory=lambda: datetime.now(timezone.utc).isoformat())


@dataclass
class FrameDetectionResult:
    """Results for an entire frame"""
    frame_uuid: str
    original_frame_metadata: Dict[str, Any]
    regions_processed: int
    total_detections: int
    region_results: List[RegionDetectionResult]
    processing_timestamp: str = field(default_factory=lambda: datetime.now(timezone.utc).isoformat())
    detection_model: str = "yolo11n"
    processing_success: bool = True
    processing_error: Optional[str] = None


@dataclass
class DetectionSummary:
    """Summary statistics for detection processing"""
    total_frames_processed: int
    total_regions_processed: int
    total_detections_found: int
    processing_start_time: str
    processing_end_time: str
    detection_model: str
    success_rate: float
    error_count: int
