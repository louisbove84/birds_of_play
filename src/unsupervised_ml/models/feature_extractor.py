"""
Feature Extractor for bird images using pre-trained CNNs.
"""

import numpy as np
import torch
import torch.nn as nn
from torchvision import models, transforms
from typing import List, Tuple
import logging

class FeatureExtractor:
    """Extract deep features from bird images using pre-trained CNNs."""
    
    def __init__(self, model_name: str = 'resnet50', use_gpu: bool = True):
        self.model_name = model_name
        self.device = torch.device('cuda' if use_gpu and torch.cuda.is_available() else 'cpu')
        
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # Load model
        self.model = self._load_model()
        self.model.to(self.device)
        self.model.eval()
        
        # Image preprocessing
        self.transform = transforms.Compose([
            transforms.ToPILImage(),
            transforms.Resize((224, 224)),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406], 
                               std=[0.229, 0.224, 0.225])
        ])
        
        self.logger.info(f"Feature extractor initialized with {model_name} on {self.device}")
    
    def _load_model(self) -> nn.Module:
        """Load and modify pre-trained model for feature extraction."""
        if self.model_name == 'resnet50':
            model = models.resnet50(weights=models.ResNet50_Weights.IMAGENET1K_V2)
            model = nn.Sequential(*list(model.children())[:-1])
        else:
            raise ValueError(f"Unsupported model: {self.model_name}")
        
        return model
    
    def extract_features(self, images: np.ndarray) -> np.ndarray:
        """Extract features from a batch of images."""
        features = []
        
        with torch.no_grad():
            for image in images:
                if image.dtype != np.uint8:
                    image = (image * 255).astype(np.uint8)
                
                tensor = self.transform(image).unsqueeze(0).to(self.device)
                feature = self.model(tensor)
                
                if len(feature.shape) > 2:
                    feature = torch.flatten(feature, start_dim=1)
                
                features.append(feature.cpu().numpy())
        
        return np.vstack(features)
    
    def get_feature_dim(self) -> int:
        """Get the dimensionality of extracted features."""
        dummy_input = np.random.rand(1, 224, 224, 3).astype(np.float32)
        features = self.extract_features(dummy_input)
        return features.shape[1]


class FeaturePipeline:
    """Complete pipeline for feature extraction from bird objects."""
    
    def __init__(self, data_manager, feature_extractor: FeatureExtractor):
        self.data_manager = data_manager
        self.feature_extractor = feature_extractor
        self.logger = logging.getLogger(__name__)
    
    def extract_all_features(self, min_confidence: float = 0.7, 
                           batch_size: int = 32) -> Tuple[np.ndarray, List[dict]]:
        """Extract features from all bird objects."""
        df = self.data_manager.load_bird_objects(min_confidence=min_confidence)
        
        if len(df) == 0:
            self.logger.warning("No bird objects found")
            return np.array([]), []
        
        self.logger.info(f"Extracting features from {len(df)} bird objects")
        
        all_features = []
        metadata = []
        
        for i in range(0, len(df), batch_size):
            batch_df = df.iloc[i:i+batch_size]
            
            images = self.data_manager.load_batch_images(batch_df)
            batch_features = self.feature_extractor.extract_features(images)
            all_features.append(batch_features)
            
            for _, row in batch_df.iterrows():
                metadata.append({
                    'object_id': row['object_id'],
                    'confidence': row['confidence'],
                    'frame_id': row['frame_id'],
                    'region_id': row['region_id'],
                    'timestamp': row['timestamp'],
                    'image_path': row['image_path']
                })
            
            self.logger.info(f"Processed batch {i//batch_size + 1}/{(len(df)-1)//batch_size + 1}")
        
        features = np.vstack(all_features) if all_features else np.array([])
        self.logger.info(f"Extracted features of shape {features.shape}")
        
        return features, metadata
