"""
Birds of Play - Image Detection Module
=====================================

This module provides image detection capabilities for the Birds of Play system.
It processes consolidated motion regions from MongoDB and runs various detection
algorithms (YOLO11, etc.) to identify objects within those regions.

Main Components:
- ImageDetectionProcessor: Main orchestrator class
- ObjectDetector: Object detection implementation
- DetectionResult: Data structures for detection results
- MongoDB integration for storing detection results
"""

from .detectors.object_detector import ObjectDetector
from .models.detection_processor import ImageDetectionProcessor
from .models.detection_result import DetectionMetadata, RegionDetectionResult, FrameDetectionResult, DetectionSummary

__all__ = [
    'ImageDetectionProcessor',
    'ObjectDetector', 
    'DetectionMetadata',
    'RegionDetectionResult',
    'FrameDetectionResult',
    'DetectionSummary'
]

__version__ = "1.0.0"
