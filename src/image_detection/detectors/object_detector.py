"""
Object Detector Implementation
=============================

This module provides object detection for consolidated motion regions.
Currently supports YOLO models but designed to be model-agnostic.
"""

import numpy as np
from typing import List, Dict, Any, Optional
from ..models.detection_result import DetectionMetadata, RegionDetectionResult


class ObjectDetector:
    """Object detector for motion regions (supports multiple model types)"""
    
    def __init__(self, model_type: str = "yolo", model_path: str = "yolo11n.pt", confidence_threshold: float = 0.25):
        """
        Initialize object detector
        
        Args:
            model_type: Type of model (yolo, rcnn, ssd, etc.)
            model_path: Path to model file
            confidence_threshold: Minimum confidence for detections
        """
        self.model_type = model_type.lower()
        self.model_path = model_path
        self.confidence_threshold = confidence_threshold
        self.model = None
        self._validate_model_type()
        self._load_model()
    
    def _validate_model_type(self):
        """Validate the model type and check for required dependencies"""
        supported_models = ["yolo", "rcnn", "ssd", "retinanet", "efficientdet"]
        
        if self.model_type not in supported_models:
            raise ValueError(f"Unsupported model type: {self.model_type}. Supported types: {supported_models}")
        
        # Check for required dependencies based on model type
        if self.model_type == "yolo":
            try:
                import ultralytics
            except ImportError:
                raise ImportError("YOLO model requires ultralytics package. Install with: pip install ultralytics")
        elif self.model_type == "rcnn":
            try:
                import torchvision
            except ImportError:
                raise ImportError("RCNN model requires torchvision package. Install with: pip install torchvision")
        elif self.model_type in ["ssd", "retinanet", "efficientdet"]:
            try:
                import torch
                import torchvision
            except ImportError:
                raise ImportError(f"{self.model_type.upper()} model requires torch and torchvision packages. Install with: pip install torch torchvision")
    
    def _load_model(self):
        """Load the detection model based on model type"""
        try:
            if self.model_type == "yolo":
                self._load_yolo_model()
            elif self.model_type == "rcnn":
                self._load_rcnn_model()
            elif self.model_type == "ssd":
                self._load_ssd_model()
            elif self.model_type == "retinanet":
                self._load_retinanet_model()
            elif self.model_type == "efficientdet":
                self._load_efficientdet_model()
            else:
                raise ValueError(f"Model loading not implemented for type: {self.model_type}")
                
        except Exception as e:
            raise RuntimeError(f"Failed to load {self.model_type} model: {e}")
    
    def _load_yolo_model(self):
        """Load YOLO model"""
        from ultralytics import YOLO
        self.model = YOLO(self.model_path)
        print(f"✅ YOLO model loaded: {self.model_path}")
    
    def _load_rcnn_model(self):
        """Load RCNN model (placeholder for future implementation)"""
        raise NotImplementedError("RCNN model loading not yet implemented")
    
    def _load_ssd_model(self):
        """Load SSD model (placeholder for future implementation)"""
        raise NotImplementedError("SSD model loading not yet implemented")
    
    def _load_retinanet_model(self):
        """Load RetinaNet model (placeholder for future implementation)"""
        raise NotImplementedError("RetinaNet model loading not yet implemented")
    
    def _load_efficientdet_model(self):
        """Load EfficientDet model (placeholder for future implementation)"""
        raise NotImplementedError("EfficientDet model loading not yet implemented")
    
    def detect_objects_in_region(self, region_image: np.ndarray, region_bounds: Dict[str, Any]) -> List[DetectionMetadata]:
        """
        Detect objects in a single region image
        
        Args:
            region_image: Cropped image of the region
            region_bounds: Bounds of the region within the original frame
            
        Returns:
            List of detection metadata
        """
        if self.model is None:
            raise RuntimeError(f"{self.model_type.upper()} model not loaded")
        
        if region_image.size == 0:
            return []
        
        try:
            # Run model inference based on model type
            if self.model_type == "yolo":
                return self._detect_with_yolo(region_image, region_bounds)
            elif self.model_type == "rcnn":
                return self._detect_with_rcnn(region_image, region_bounds)
            elif self.model_type == "ssd":
                return self._detect_with_ssd(region_image, region_bounds)
            elif self.model_type == "retinanet":
                return self._detect_with_retinanet(region_image, region_bounds)
            elif self.model_type == "efficientdet":
                return self._detect_with_efficientdet(region_image, region_bounds)
            else:
                raise ValueError(f"Detection not implemented for model type: {self.model_type}")
                
        except Exception as e:
            print(f"❌ {self.model_type.upper()} detection failed: {e}")
            return []
    
    def _detect_with_yolo(self, region_image: np.ndarray, region_bounds: Dict[str, Any]) -> List[DetectionMetadata]:
        """Detect objects using YOLO model"""
        # Run YOLO inference
        results = self.model(region_image, verbose=False)
        
        detections = []
        region_x = region_bounds.get('x', 0)
        region_y = region_bounds.get('y', 0)
        
        for result in results:
            boxes = result.boxes
            if boxes is not None:
                for box in boxes:
                    # Get box coordinates (relative to region)
                    x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                    confidence = float(box.conf[0].cpu().numpy())
                    class_id = int(box.cls[0].cpu().numpy())
                    class_name = self.model.names[class_id]
                    
                    # Filter by confidence threshold
                    if confidence < self.confidence_threshold:
                        continue
                    
                    # Create detection metadata
                    detection = DetectionMetadata(
                        class_id=class_id,
                        class_name=class_name,
                        confidence=confidence,
                        bbox_x1=int(x1 + region_x),  # Convert to full frame coordinates
                        bbox_y1=int(y1 + region_y),
                        bbox_x2=int(x2 + region_x),
                        bbox_y2=int(y2 + region_y),
                        region_x=int(x1),  # Original region coordinates
                        region_y=int(y1),
                        region_w=int(x2 - x1),
                        region_h=int(y2 - y1)
                    )
                    detections.append(detection)
        
        return detections
    
    def _detect_with_rcnn(self, region_image: np.ndarray, region_bounds: Dict[str, Any]) -> List[DetectionMetadata]:
        """Detect objects using RCNN model (placeholder)"""
        raise NotImplementedError("RCNN detection not yet implemented")
    
    def _detect_with_ssd(self, region_image: np.ndarray, region_bounds: Dict[str, Any]) -> List[DetectionMetadata]:
        """Detect objects using SSD model (placeholder)"""
        raise NotImplementedError("SSD detection not yet implemented")
    
    def _detect_with_retinanet(self, region_image: np.ndarray, region_bounds: Dict[str, Any]) -> List[DetectionMetadata]:
        """Detect objects using RetinaNet model (placeholder)"""
        raise NotImplementedError("RetinaNet detection not yet implemented")
    
    def _detect_with_efficientdet(self, region_image: np.ndarray, region_bounds: Dict[str, Any]) -> List[DetectionMetadata]:
        """Detect objects using EfficientDet model (placeholder)"""
        raise NotImplementedError("EfficientDet detection not yet implemented")
    
    def detect_objects_in_regions(self, frame_image: np.ndarray, regions: List[Dict[str, Any]]) -> List[RegionDetectionResult]:
        """
        Detect objects in multiple regions of a frame
        
        Args:
            frame_image: Full frame image
            regions: List of region bounds dictionaries
            
        Returns:
            List of region detection results
        """
        region_results = []
        
        for i, region in enumerate(regions):
            x, y, w, h = region['x'], region['y'], region['width'], region['height']
            
            # Extract region from full frame
            region_image = frame_image[y:y+h, x:x+w]
            
            if region_image.size == 0:
                region_results.append(RegionDetectionResult(
                    region_id=i,
                    region_bounds=region,
                    detections=[],
                    processed=False,
                    detection_count=0,
                    processing_error="Empty region"
                ))
                continue
            
            try:
                # Detect objects in this region
                detections = self.detect_objects_in_region(region_image, region)
                
                region_results.append(RegionDetectionResult(
                    region_id=i,
                    region_bounds=region,
                    detections=detections,
                    processed=True,
                    detection_count=len(detections)
                ))
                
            except Exception as e:
                region_results.append(RegionDetectionResult(
                    region_id=i,
                    region_bounds=region,
                    detections=[],
                    processed=False,
                    detection_count=0,
                    processing_error=str(e)
                ))
        
        return region_results
