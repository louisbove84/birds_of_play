"""
Object Data Manager for loading bird images and metadata.
"""

import sys
from pathlib import Path
from typing import List, Dict
import pandas as pd
import numpy as np
from pymongo import MongoClient
import cv2

# Add project root to path
project_root = Path(__file__).resolve().parent.parent.parent.parent
sys.path.insert(0, str(project_root))

class ObjectDataManager:
    """Manages loading bird objects with metadata from MongoDB."""
    
    def __init__(self, mongodb_uri: str = "mongodb://localhost:27017/", 
                 db_name: str = "birds_of_play"):
        self.mongodb_uri = mongodb_uri
        self.db_name = db_name
        self.project_root = project_root
        self.objects_dir = self.project_root / "data" / "objects"
        
        # Connect to MongoDB
        self.client = MongoClient(mongodb_uri)
        self.db = self.client[db_name]
        self.region_detections = self.db['region_detections']
        
    def load_bird_objects(self, min_confidence: float = 0.5) -> pd.DataFrame:
        """Load all bird objects with their metadata."""
        # Use high_confidence_detections collection instead
        high_conf_collection = self.db['high_confidence_detections']
        cursor = high_conf_collection.find({})
        
        objects_data = []
        
        for detection_doc in cursor:
            detection_info = detection_doc.get('detection_info', {})
            confidence = detection_info.get('confidence', 0.0)
            class_name = detection_info.get('class_name', 'unknown')
            
            if confidence >= min_confidence and class_name.lower() == 'bird':
                detection_id = detection_doc.get('detection_id')
                region_id = detection_doc.get('region_id')
                frame_id = detection_doc.get('frame_id')
                timestamp = detection_doc.get('timestamp')
                bbox = detection_info.get('bbox', [])
                
                # Convert detection_id to object_id format (det_0 -> obj_0)
                if detection_id:
                    object_id = detection_id.replace('_det_', '_obj_')
                    object_image_path = self.objects_dir / f"{object_id}.jpg"
                    
                    if object_image_path.exists():
                        objects_data.append({
                            'object_id': object_id,
                            'detection_id': detection_id,
                            'image_path': str(object_image_path),
                            'class_name': class_name,
                            'confidence': confidence,
                            'bbox': bbox,
                            'region_id': region_id,
                            'frame_id': frame_id,
                            'timestamp': timestamp
                        })
        
        return pd.DataFrame(objects_data)
    
    def load_object_image(self, image_path: str, target_size=(224, 224)) -> np.ndarray:
        """Load and preprocess an object image."""
        try:
            image = cv2.imread(image_path)
            if image is None:
                return np.zeros((*target_size, 3), dtype=np.float32)
            
            image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
            image = cv2.resize(image, target_size)
            image = image.astype(np.float32) / 255.0
            
            return image
        except Exception:
            return np.zeros((*target_size, 3), dtype=np.float32)
    
    def load_batch_images(self, df: pd.DataFrame, target_size=(224, 224)) -> np.ndarray:
        """Load a batch of images from DataFrame."""
        images = []
        for _, row in df.iterrows():
            image = self.load_object_image(row['image_path'], target_size)
            images.append(image)
        return np.array(images)
    
    def get_statistics(self) -> Dict:
        """Get statistics about the loaded data."""
        df = self.load_bird_objects()
        
        return {
            'total_objects': len(df),
            'confidence_stats': {
                'mean': df['confidence'].mean() if len(df) > 0 else 0,
                'std': df['confidence'].std() if len(df) > 0 else 0,
            },
            'unique_frames': df['frame_id'].nunique() if len(df) > 0 else 0,
            'unique_regions': df['region_id'].nunique() if len(df) > 0 else 0,
            'class_distribution': df['class_name'].value_counts().to_dict() if len(df) > 0 else {}
        }
    
    def close(self):
        """Close database connection."""
        if self.client:
            self.client.close()
            
    def __enter__(self):
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
